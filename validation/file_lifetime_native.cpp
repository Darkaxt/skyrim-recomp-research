#include "native_slice.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <immintrin.h>

struct Call { std::uint64_t args[6]{}, result{}; std::uint32_t mxcsr{}; };
extern "C" std::uint64_t file_call_probe(void*,Call*);
extern "C" void file_bad_abi();
struct FileObject { std::uint32_t flags, padding; HANDLE handle; };
static_assert(sizeof(FileObject)==16 && offsetof(FileObject,handle)==8);
struct Guard {
    unsigned char* base{}; unsigned char* data{}; std::size_t size; unsigned char* value; std::size_t extent;
    Guard(std::size_t bytes,bool end):size((bytes+4095)&~std::size_t(4095)),extent(bytes) {
        base=static_cast<unsigned char*>(VirtualAlloc(nullptr,size+8192,MEM_RESERVE,PAGE_NOACCESS));
        require(base,"Guard reserve"); data=static_cast<unsigned char*>(VirtualAlloc(base+4096,size,MEM_COMMIT,PAGE_READWRITE));
        if(!data){VirtualFree(base,0,MEM_RELEASE);throw std::runtime_error("Guard commit");}
        std::memset(data,0xa5,size); value=data+(end?size-bytes:0);
    }
    ~Guard(){VirtualFree(base,0,MEM_RELEASE);}
    Guard(const Guard&)=delete;
    void check() const {
        for(auto p=data;p<value;++p) require(*p==0xa5,"Native oracle failure: leading canary");
        for(auto p=value+extent;p<data+size;++p) require(*p==0xa5,"Native oracle failure: trailing canary");
    }
};
enum Api { Create,Read,Seek,Size,LastError,Close };
static const char* api_names[]{"CreateFileA","ReadFile","SetFilePointerEx","GetFileSizeEx","GetLastError","CloseHandle"};
struct Event { Api api; std::uint64_t handle,a,b,c,d,e,f,result,output; DWORD error; };
struct Handle { HANDLE raw; std::uint64_t id; bool live; };
static std::vector<Event> events;
static std::vector<Handle> handles;
static std::uint64_t next_handle=0,api_calls=0,abi_calls=0,workflows=0,read_checks=0,failures=0,early_cleanups=0,mapper_checks=0;
static bool negative_read=false;
static const char* allowed_path=nullptr;
static std::ofstream trace;
static std::string case_name;
static unsigned variant=0;
static std::uint64_t token(HANDLE h) {
    if(h==INVALID_HANDLE_VALUE || h==nullptr)return 0;
    for(auto& item:handles)if(item.raw==h && item.live)return item.id;
    return std::numeric_limits<std::uint64_t>::max();
}
static void record(Event event,DWORD error) {
    event.error=error; events.push_back(event); ++api_calls; SetLastError(error);
}
static HANDLE WINAPI traced_create(LPCSTR name,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES security,
                                   DWORD disposition,DWORD flags,HANDLE template_file) {
    const auto incoming=GetLastError();
    require(name==allowed_path && access==GENERIC_READ && share==FILE_SHARE_READ && !security
            && disposition==OPEN_EXISTING && flags==FILE_FLAG_SEQUENTIAL_SCAN && !template_file,
            "Native oracle failure: open ABI/arguments");
    SetLastError(incoming);
    HANDLE result=CreateFileA(name,access,share,security,disposition,flags,template_file);const auto error=GetLastError();
    std::uint64_t id=0;
    if(result!=INVALID_HANDLE_VALUE){id=++next_handle;handles.push_back({result,id,true});}
    record({Create,id,reinterpret_cast<std::uint64_t>(name),access,share,disposition,flags,0,result!=INVALID_HANDLE_VALUE,0,0},error);
    return result;
}
static BOOL WINAPI traced_read(HANDLE h,LPVOID data,DWORD bytes,LPDWORD count,LPOVERLAPPED overlap) {
    const auto incoming=GetLastError(); require(!overlap && count,"Native oracle failure: synchronous read ABI");
    SetLastError(incoming);BOOL result=ReadFile(h,data,bytes,count,overlap);const auto error=GetLastError();
    if(negative_read && result && *count)static_cast<unsigned char*>(data)[0]^=0xff;
    record({Read,token(h),reinterpret_cast<std::uint64_t>(data),bytes,reinterpret_cast<std::uint64_t>(count),reinterpret_cast<std::uint64_t>(overlap),0,0,static_cast<std::uint64_t>(result),*count,0},error);return result;
}
static BOOL WINAPI traced_seek(HANDLE h,LARGE_INTEGER distance,PLARGE_INTEGER position,DWORD mode) {
    BOOL result=SetFilePointerEx(h,distance,position,mode);const auto error=GetLastError();
    record({Seek,token(h),static_cast<std::uint64_t>(distance.QuadPart),mode,reinterpret_cast<std::uint64_t>(position),0,0,0,static_cast<std::uint64_t>(result),result?static_cast<std::uint64_t>(position->QuadPart):0,0},error);return result;
}
static BOOL WINAPI traced_size(HANDLE h,PLARGE_INTEGER size) {
    BOOL result=GetFileSizeEx(h,size);const auto error=GetLastError();
    record({Size,token(h),reinterpret_cast<std::uint64_t>(size),0,0,0,0,0,static_cast<std::uint64_t>(result),result?static_cast<std::uint64_t>(size->QuadPart):0,0},error);return result;
}
static DWORD WINAPI traced_error() {
    const auto error=GetLastError();record({LastError,0,0,0,0,0,0,0,error,0,0},error);return error;
}
static BOOL WINAPI traced_close(HANDLE h) {
    const auto id=token(h);BOOL result=CloseHandle(h);const auto error=GetLastError();
    if(result)for(auto& item:handles)if(item.raw==h&&item.live)item.live=false;
    record({Close,id,0,0,0,0,0,0,static_cast<std::uint64_t>(result),0,0},error);return result;
}
struct Engine {
    NativeSlice slice;
    Engine(const char* path):slice(path) {
        require(reinterpret_cast<std::uintptr_t>(slice.base)!=0x140000000ULL,"Native mapping was not relocated");
        const std::pair<std::uint32_t,void*> imports[]={{0x17c83d8,reinterpret_cast<void*>(traced_create)},
            {0x17c8478,reinterpret_cast<void*>(traced_read)},{0x17c83a8,reinterpret_cast<void*>(traced_seek)},
            {0x17c83c8,reinterpret_cast<void*>(traced_size)},{0x17c8290,reinterpret_cast<void*>(traced_error)},
            {0x17c8270,reinterpret_cast<void*>(traced_close)}};
        for(auto [rva,hook]:imports)*reinterpret_cast<void**>(slice.base+rva)=hook;
        require(slice.functions.size()==7,"Unexpected native unwind count");
        for(auto rva:{0xebfd70u,0xec01b0u,0xec0410u,0xec0590u,0xec0670u,0xec0710u,0xec0960u}) {
            DWORD64 base=0;auto f=RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(slice.base+rva),&base,nullptr);
            require(f&&base==reinterpret_cast<DWORD64>(slice.base)&&f->BeginAddress==rva,"Native unwind lookup failed");
            const auto expected=std::find_if(slice.functions.begin(),slice.functions.end(),[rva](const auto& entry){return entry.BeginAddress==rva;});
            require(expected!=slice.functions.end()&&f->EndAddress==expected->EndAddress&&f->UnwindData==expected->UnwindData,"Native unwind record differs");
        }
        for(auto rva:{0xec0190u,0xebff00u,0xec0990u,0xec4de0u}) {
            DWORD64 base=0;require(!RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(slice.base+rva),&base,nullptr),"Invented leaf unwind entry");
        }
        MEMORY_BASIC_INFORMATION info{};require(VirtualQuery(slice.base+0xec4e40,&info,sizeof(info))&&info.Protect==PAGE_EXECUTE_READ,"Mapper tables/code page not RX");
    }
    std::uint64_t invoke(std::uint32_t rva,std::initializer_list<std::uint64_t> args,std::initializer_list<Api> calls) {
        auto start=events.size();Call state{};std::copy(args.begin(),args.end(),state.args);
        const auto control=_mm_getcsr()&0xffc0;
        const auto mask=file_call_probe(slice.base+rva,&state);++abi_calls;
        require(mask==0 && (state.mxcsr&0xffc0)==control,"Native oracle failure: nonvolatile ABI/stack/FP control");
        require(events.size()==start+calls.size(),"Native oracle failure: API call count/order");
        for(auto api:calls)require(events[start++].api==api,"Native oracle failure: API call order");
        return state.result;
    }
};
static std::uint64_t ptr(const void* p){return reinterpret_cast<std::uint64_t>(p);}
static void begin_case(const std::string& name) {
    for(auto& h:handles)require(!h.live,"Native oracle failure: previous handle leak");
    handles.clear(); events.clear();case_name=name;
}
static void end_case() {
    for(const auto& h:handles) {
        require(!h.live,"Native oracle failure: missing close");DWORD flags=0;
        require(!GetHandleInformation(h.raw,&flags)&&GetLastError()==ERROR_INVALID_HANDLE,"Native oracle failure: OS handle remains live");
    }
    for(const auto& e:events)trace<<variant<<','<<case_name<<','<<api_names[e.api]<<','<<e.handle<<','<<e.a<<','<<e.b<<','<<e.c<<','<<e.d<<','<<e.e<<','<<e.f<<','<<e.result<<','<<e.output<<','<<e.error<<'\n';
    require(trace.good(),"Trace write failed");
}
static FileObject* object(Guard& g){return reinterpret_cast<FileObject*>(g.value);}
static void check_object(Guard& g) {g.check();require(object(g)->padding==0xa5a5a5a5,"Native oracle failure: object padding changed");}
static void close_object(Engine& e,Guard& g) {
    const bool live=object(g)->handle!=INVALID_HANDLE_VALUE;const auto flags=object(g)->flags;
    if(live)e.invoke(0xec0960,{ptr(g.value)},{Close});else e.invoke(0xec0960,{ptr(g.value)},{});
    require(object(g)->handle==INVALID_HANDLE_VALUE&&object(g)->flags==flags,"Native oracle failure: close state");check_object(g);
}
static std::vector<unsigned char> content(std::size_t size,unsigned pattern) {
    std::vector<unsigned char> bytes(size);
    for(std::size_t i=0;i<size;++i)
        bytes[i]=pattern==0?0:pattern==1?(i&1?0x55:0xaa):static_cast<unsigned char>((i*73+19)^(i>>3)^((i*i)>>7));
    return bytes;
}
struct Fixture {
    Engine& engine; Guard source,destination,path,output,buffer; FileObject* active;
    std::vector<unsigned char> expected; std::size_t position=0; std::string filename;
    Fixture(Engine& e,const std::string& name,std::vector<unsigned char> bytes,bool end,bool parameterized)
        :engine(e),source(16,end),destination(16,!end),path(name.size()+1,end),output(8,!end),buffer(65537,end),active(object(source)),expected(std::move(bytes)),filename(name) {
        std::memcpy(path.value,name.c_str(),name.size()+1);allowed_path=reinterpret_cast<const char*>(path.value);
        if(parameterized)require(engine.invoke(0xebfd70,{ptr(source.value),ptr(path.value),0,0,0,0},{Create})==ptr(source.value),"Native oracle failure: constructor result");
        else {
            require(engine.invoke(0xec0190,{ptr(source.value)}, {})==ptr(source.value),"Native oracle failure: default constructor result");
            require(active->flags==1&&active->handle==INVALID_HANDLE_VALUE,"Native oracle failure: default initialization");
            require(engine.invoke(0xec0410,{ptr(source.value),ptr(path.value),0,0},{Create})==0,"Native oracle failure: open result");
        }
        require(!handles.empty()&&active->handle==handles.back().raw&&handles.back().live,"Native oracle failure: constructed handle");
        require(active->flags==(parameterized?0u:1u),"Native oracle failure: constructor flags");check();
    }
    std::array<unsigned char,32> objects() {
        std::array<unsigned char,32> bytes{};std::memcpy(bytes.data(),source.value,16);std::memcpy(bytes.data()+16,destination.value,16);return bytes;
    }
    std::vector<unsigned char> buffer_bytes() {return {buffer.value,buffer.value+buffer.extent};}
    void same_buffer(const std::vector<unsigned char>& before) {
        require(!std::memcmp(buffer.value,before.data(),before.size()),"Native oracle failure: unrelated buffer write");
    }
    void check() {check_object(source);check_object(destination);path.check();output.check();buffer.check();
        if(active==object(source))for(unsigned i=0;i<16;++i)require(destination.value[i]==0xa5,"Native oracle failure: unused destination changed");
        require(!std::memcmp(path.value,filename.c_str(),filename.size()+1),"Native oracle failure: path mutated");}
    void size() {
        const auto before=objects();const auto bytes=buffer_bytes();
        std::memset(output.value,0x77,8);require(engine.invoke(0xec0710,{ptr(active),ptr(output.value)},{Size})==0,"Native oracle failure: size status");
        require(*reinterpret_cast<std::uint64_t*>(output.value)==expected.size(),"Native oracle failure: size output");
        require(objects()==before,"Native oracle failure: size changed object");same_buffer(bytes);check();
    }
    void read(std::uint32_t requested) {
        std::memset(buffer.value,0xa5,buffer.extent);std::memset(output.value,0x77,8);
        const auto wanted=std::min<std::size_t>(requested,position<expected.size()?expected.size()-position:0);
        const auto start=events.size();const auto before=objects();
        require(engine.invoke(0xec0590,{ptr(active),ptr(buffer.value),requested,ptr(output.value)},{Read})==0,"Native oracle failure: read status");
        require(*reinterpret_cast<std::uint64_t*>(output.value)==wanted,"Native oracle failure: read count");
        require(events[start].handle==token(active->handle)&&events[start].a==ptr(buffer.value)&&events[start].b==requested&&events[start].output==wanted,"Native oracle failure: read trace arguments");
        if(wanted)require(!std::memcmp(buffer.value,expected.data()+position,wanted),"Native oracle failure: read bytes");
        for(std::size_t i=wanted;i<buffer.extent;++i)require(buffer.value[i]==0xa5,"Native oracle failure: read overrun");
        require(objects()==before,"Native oracle failure: read changed object");position+=wanted;check();++read_checks;
    }
    void seek(std::int64_t distance,unsigned mode) {
        const auto signed_target=static_cast<std::int64_t>(mode==FILE_BEGIN?0:mode==FILE_CURRENT?position:expected.size())+distance;
        require(signed_target>=0,"Invalid successful seek fixture");const auto target=static_cast<std::uint64_t>(signed_target);
        std::memset(output.value,0x77,8);const auto start=events.size();const auto before=objects();const auto bytes=buffer_bytes();
        require(engine.invoke(0xec0670,{ptr(active),static_cast<std::uint64_t>(distance),mode,ptr(output.value)},{Seek})==0,"Native oracle failure: seek status");
        require(*reinterpret_cast<std::uint64_t*>(output.value)==target,"Native oracle failure: seek output");
        require(events[start].handle==token(active->handle)&&events[start].a==static_cast<std::uint64_t>(distance)&&events[start].b==mode,"Native oracle failure: seek trace arguments");
        require(objects()==before,"Native oracle failure: seek changed object");same_buffer(bytes);position=target;check();
    }
    void transfer() {
        const auto h=active->handle;const auto flags=active->flags;const auto bytes=buffer_bytes();const auto out=*reinterpret_cast<std::uint64_t*>(output.value);
        require(engine.invoke(0xec01b0,{ptr(destination.value),ptr(source.value)}, {})==ptr(destination.value),"Native oracle failure: transfer return");
        require(object(source)->handle==INVALID_HANDLE_VALUE&&object(source)->flags==1&&object(destination)->handle==h&&object(destination)->flags==flags,"Native oracle failure: transfer ownership");
        require(*reinterpret_cast<std::uint64_t*>(output.value)==out,"Native oracle failure: transfer changed output");same_buffer(bytes);active=object(destination);check();
    }
    void cleanup() {
        const auto bytes=buffer_bytes();const auto out=*reinterpret_cast<std::uint64_t*>(output.value);
        close_object(engine,source);if(active==object(destination))close_object(engine,destination);
        require(*reinterpret_cast<std::uint64_t*>(output.value)==out,"Native oracle failure: close changed output");same_buffer(bytes);check();
    }
};
static void normal(Engine& engine,const std::string& name,std::size_t length,unsigned pattern,bool end,bool constructor) {
    begin_case("normal_"+std::to_string(length)+"_"+std::to_string(pattern)+"_"+std::to_string(end)+"_"+std::to_string(constructor));
    Fixture f(engine,name,content(length,pattern),end,constructor);f.size();
    for(auto n:{0u,1u,7u,static_cast<unsigned>(length+1),1u})f.read(n);
    f.seek(0,FILE_BEGIN);f.read(static_cast<unsigned>(length));
    f.seek(1,FILE_CURRENT);f.read(2);f.seek(0,FILE_END);f.read(1);
    f.seek(-static_cast<std::int64_t>(std::min<std::size_t>(length,5)),FILE_END);f.read(8);
    f.transfer();close_object(engine,f.source);f.seek(0,FILE_BEGIN);f.read(7);f.cleanup();f.cleanup();end_case();++workflows;
}
static std::uint32_t expected_map(const std::vector<std::uint32_t>& mapping,std::uint32_t error) {
    return mapping[error<=183?error:184];
}
static void open_failure(Engine& e,const std::string& filename,DWORD error,const std::vector<std::uint32_t>& mapping,bool end,bool parameterized) {
    begin_case("open_failure_"+std::to_string(error)+"_"+std::to_string(end)+"_"+std::to_string(parameterized));
    Guard object_guard(16,end),path(filename.size()+1,!end);std::memcpy(path.value,filename.c_str(),filename.size()+1);allowed_path=reinterpret_cast<const char*>(path.value);
    auto f=object(object_guard);const auto expected=expected_map(mapping,error);
    if(parameterized)require(e.invoke(0xebfd70,{ptr(f),ptr(path.value),0,0,0,0},{Create,LastError})==ptr(f)&&f->flags==expected,"Native oracle failure: failed constructor state");
    else {e.invoke(0xec0190,{ptr(f)},{});require(e.invoke(0xec0410,{ptr(f),ptr(path.value),0,0},{Create,LastError})==expected&&f->flags==1,"Native oracle failure: failed open status");}
    require(events[0].error==error&&events[1].result==error&&f->handle==INVALID_HANDLE_VALUE,"Native oracle failure: original failure mapping");
    close_object(e,object_guard);close_object(e,object_guard);path.check();
    require(!std::memcmp(path.value,filename.c_str(),filename.size()+1),"Native oracle failure: failed open changed path");end_case();++failures;
}
static void closed_failures(Engine& e,const std::string& name,const std::vector<std::uint32_t>& mapping,bool end) {
    begin_case("closed_and_seek_failure_"+std::to_string(end));Fixture f(e,name,content(4097,2),end,true);
    std::memset(f.output.value,0x77,8);const auto start=events.size();const auto objects=f.objects();const auto bytes=f.buffer_bytes();
    require(e.invoke(0xec0670,{ptr(f.active),UINT64_MAX,FILE_BEGIN,ptr(f.output.value)},{Seek,LastError})==expected_map(mapping,ERROR_NEGATIVE_SEEK),"Native oracle failure: failed seek status");
    require(events[start].error==ERROR_NEGATIVE_SEEK&&*reinterpret_cast<std::uint64_t*>(f.output.value)==0x7777777777777777ULL,"Native oracle failure: failed seek output/error");
    require(events[start+1].result==ERROR_NEGATIVE_SEEK&&f.objects()==objects,"Native oracle failure: failed seek state");f.same_buffer(bytes);
    f.check();f.read(3);f.cleanup();
    const auto before_buffer=std::vector<unsigned char>(f.buffer.value,f.buffer.value+f.buffer.extent);
    const auto before_object=*f.active;
    const auto closed_objects=f.objects();
    const auto read_start=events.size();
    std::memset(f.output.value,0x77,8);require(e.invoke(0xec0590,{ptr(f.active),ptr(f.buffer.value),3,ptr(f.output.value)},{Read,LastError})==expected_map(mapping,ERROR_INVALID_HANDLE),"Native oracle failure: closed read status");
    require(events[read_start].error==ERROR_INVALID_HANDLE&&events[read_start+1].result==ERROR_INVALID_HANDLE,"Native oracle failure: closed read error");
    require(*reinterpret_cast<std::uint64_t*>(f.output.value)==0,"Native oracle failure: closed read count");
    require(!std::memcmp(f.buffer.value,before_buffer.data(),before_buffer.size()),"Native oracle failure: failed read changed buffer");
    std::memset(f.output.value,0x77,8);require(e.invoke(0xec0710,{ptr(f.active),ptr(f.output.value)},{Size})==7&&*reinterpret_cast<std::uint64_t*>(f.output.value)==0,"Native oracle failure: closed size output");
    require(events.back().error==ERROR_INVALID_HANDLE,"Native oracle failure: closed size error");
    require(f.objects()==closed_objects,"Native oracle failure: closed size changed object");
    require(!std::memcmp(f.active,&before_object,sizeof(before_object)),"Native oracle failure: failed operation changed object");
    f.check();f.cleanup();end_case();++failures;
}
static void partial(Engine& e,const std::string& name,bool end,bool constructor,unsigned stop) {
    begin_case("partial_"+std::to_string(stop)+"_"+std::to_string(end)+"_"+std::to_string(constructor));
    {Fixture f(e,name,content(4097,2),end,constructor);
        if(stop>=1)f.size();
        if(stop>=2)f.read(9);
        if(stop>=3)f.seek(1,FILE_CURRENT);
        if(stop>=4)f.transfer();
        if(stop>=5)close_object(e,f.source);
        if(stop>=6)f.cleanup();
        f.cleanup();}
    {Fixture retry(e,name,content(4097,2),!end,constructor);retry.read(11);retry.cleanup();}
    end_case();++early_cleanups;
}
int main(int argc,char** argv) {
    try {
        require(argc==6||argc==7,"Two packs, fixture directory, mapper expectations and trace required");
        negative_read=argc==7;events.reserve(256);handles.reserve(16);
        trace.open(argv[5]);require(trace.good(),"Trace open failed");
        trace<<"variant,case,api,handle_id,a,b,c,d,e,f,result,output,last_error\n";
        Call abi{};require(file_call_probe(reinterpret_cast<void*>(file_bad_abi),&abi)==1,"ABI negative control ineffective");
        SetLastError(0x10203040);require(traced_error()==0x10203040&&GetLastError()==0x10203040,"Trace clobbered last error");events.clear();
        std::ifstream input(argv[4],std::ios::binary);std::vector<std::uint32_t> mapping(188);input.read(reinterpret_cast<char*>(mapping.data()),mapping.size()*4);require(input.good(),"Mapper expectations missing");
        Engine a(argv[1]),b(argv[2]);require(a.slice.base!=b.slice.base,"Independent native mappings alias");
        const std::string directory=argv[3];
        for(Engine* engine:{&a,&b}) {
            for(std::uint32_t i=0;i<188;++i) {
                const auto error=i<184?i:i==184?184u:i==185?255u:i==186?65535u:UINT32_MAX;
                require(engine->invoke(0xec4de0,{error},{})==mapping[i],"Native oracle failure: mapper/relocated tables");++mapper_checks;
            }
            for(auto length:{0u,1u,31u,4097u,65536u})for(unsigned pattern=0;pattern<3;++pattern)
                for(bool end:{false,true})for(bool ctor:{false,true})normal(*engine,directory+"/"+std::to_string(length)+"_"+std::to_string(pattern)+".dat",length,pattern,end,ctor);
            const auto retry=directory+"/4097_2.dat";
            for(bool end:{false,true})for(bool ctor:{false,true}) {
                open_failure(*engine,directory+"/missing.dat",ERROR_FILE_NOT_FOUND,mapping,end,ctor);
                open_failure(*engine,directory+"/missing-parent/missing.dat",ERROR_PATH_NOT_FOUND,mapping,end,ctor);
                HANDLE exclusive=CreateFileA(retry.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr);require(exclusive!=INVALID_HANDLE_VALUE,"Sharing fixture failed");
                try{open_failure(*engine,retry,ERROR_SHARING_VIOLATION,mapping,end,ctor);}catch(...){CloseHandle(exclusive);throw;}
                require(CloseHandle(exclusive),"Sharing fixture close failed");
                for(unsigned stop=0;stop<7;++stop)partial(*engine,retry,end,ctor,stop);
            }
            for(bool end:{false,true})closed_failures(*engine,retry,mapping,end);
            ++variant;
        }
        trace.close();require(!trace.fail(),"Trace close failed");
        std::cout<<"{\"status\":\"pass\",\"normal_workflows\":"<<workflows<<",\"failure_workflows\":"<<failures
            <<",\"partial_cleanup_retry_workflows\":"<<early_cleanups<<",\"read_checks\":"<<read_checks
            <<",\"mapper_checks\":"<<mapper_checks<<",\"abi_calls\":"<<abi_calls<<",\"api_calls\":"<<api_calls
            <<",\"real_native_apis\":true,\"guard_pages\":true,\"two_relocated_mappings\":true,\"reversed_pack_order\":true,\"game_launched\":false}\n";
    }catch(const std::exception& e) {
        for(auto& h:handles)if(h.live){CloseHandle(h.raw);h.live=false;}
        std::cerr<<e.what()<<"\n";return 1;
    }
}
