#include "native_slice.h"
#include "crc.h"
#include <cstring>
#include <iostream>

struct Guarded {
    unsigned char* region;
    unsigned char* data;
    std::size_t size;
    explicit Guarded(std::size_t n) : size(n) {
        region=static_cast<unsigned char*>(VirtualAlloc(nullptr,n+8192,MEM_RESERVE,PAGE_NOACCESS));
        require(region,"CRC guard reserve failed");
        data=static_cast<unsigned char*>(VirtualAlloc(region+4096,n,MEM_COMMIT,PAGE_READWRITE));
        require(data,"CRC guard commit failed");
    }
    ~Guarded() { VirtualFree(region,0,MEM_RELEASE); }
};
int main(int argc,char** argv) {
    try {
        require(argc>=2,"Pack required"); NativeSlice slice(argv[1]); automatic_set_base(slice.base);
        auto seeded=reinterpret_cast<decltype(&automatic_crc_seeded)>(slice.base+0xA9AE90);
        auto buffer=reinterpret_cast<decltype(&automatic_crc_buffer)>(slice.base+0xCE2510);
        auto integer32=reinterpret_cast<decltype(&automatic_crc_u32)>(slice.base+0xCE2570);
        auto integer64=reinterpret_cast<decltype(&automatic_crc_u64)>(slice.base+0xCE25F0);
        bool negative=argc==3;
        Guarded input(65536), output(4096);
        std::vector<unsigned char> before(input.size);
        std::array<unsigned char,4096> out_before{}, out_after{};
        std::uint64_t trials=0, integers=0, random=0xfedcba9876543210ULL;
        for (unsigned pattern=0;pattern<5;++pattern) {
            for (std::size_t i=0;i<input.size;++i) {
                random^=random<<13; random^=random>>7; random^=random<<17;
                input.data[i]=pattern==0?0:pattern==1?255:pattern==2?static_cast<unsigned char>(i):pattern==3?(i&1?0x55:0xaa):static_cast<unsigned char>(random);
            }
            std::memcpy(before.data(),input.data,input.size);
            for (unsigned n : {0,1,2,3,4,7,8,15,16,31,32,33,63,64,65,255,256,257,4095,4096,4097,65535,65536})
            for (unsigned offset=0;offset<16;++offset)
            for (unsigned edge=0;edge<2;++edge) {
                if (n+offset>input.size) continue;
                auto p=input.data+(edge?input.size-n-offset:offset);
                for (std::uint32_t seed : {0u,1u,255u,0x80000000u,0xffffffffu,0x59b1a2c3u}) {
                    auto a=seeded(seed,p,n), b=automatic_crc_seeded(seed,p,n);
                    if (negative) b^=1;
                    if (a!=b) { std::cerr<<"CRC differential failure length="<<n<<" offset="<<offset<<" seed="<<seed<<"\n";return 1; }
                    std::memset(output.data,0xa5,output.size);
                    auto out=reinterpret_cast<std::uint32_t*>(output.data+(edge?4092:0));
                    std::memcpy(out_before.data(),output.data,4096);
                    buffer(out,p,n); std::memcpy(out_after.data(),output.data,4096);
                    std::memcpy(output.data,out_before.data(),4096); automatic_crc_buffer(out,p,n);
                    require(!std::memcmp(out_after.data(),output.data,4096),"CRC buffer output/state mismatch");
                    auto expected=out_before; std::memcpy(expected.data()+(edge?4092:0),out,4);
                    require(expected==out_after,"Native CRC wrote beyond output");
                    require(!std::memcmp(before.data(),input.data,input.size),"CRC changed input");
                    ++trials;
                }
            }
        }
        std::vector<std::uint64_t> values{0,1,0xff,0x80000000,0xffffffff,0x8000000000000000ULL,0xffffffffffffffffULL};
        for (unsigned bit=0;bit<64;++bit) { values.push_back(1ULL<<bit); values.push_back(~(1ULL<<bit)); }
        for (unsigned i=0;i<256;++i) { random^=random<<13;random^=random>>7;random^=random<<17;values.push_back(random); }
        for (auto value:values) for (unsigned edge=0;edge<2;++edge) {
            auto out=reinterpret_cast<std::uint32_t*>(output.data+(edge?4092:0));
            for (unsigned width : {32,64}) {
                std::memset(output.data,0x5a,4096);std::memcpy(out_before.data(),output.data,4096);
                if (width==32) integer32(out,static_cast<std::uint32_t>(value)); else integer64(out,value);
                std::memcpy(out_after.data(),output.data,4096);std::memcpy(output.data,out_before.data(),4096);
                if (width==32) automatic_crc_u32(out,static_cast<std::uint32_t>(value)); else automatic_crc_u64(out,value);
                require(!std::memcmp(out_after.data(),output.data,4096),"Integer CRC output/state mismatch");
                auto expected=out_before;std::memcpy(expected.data()+(edge?4092:0),out,4);
                require(expected==out_after,"Native integer CRC wrote beyond output");
                std::uint32_t from_buffer{};buffer(&from_buffer,&value,width/8);
                require(*out==from_buffer,"Integer CRC disagrees with native byte-buffer CRC");++integers;
            }
        }
        std::cout<<"{\"status\":\"pass\",\"buffer_trials\":"<<trials<<",\"integer_trials\":"<<integers
            <<",\"routines\":4,\"seeds\":6,\"alignments\":16,\"patterns\":5,\"guard_pages\":true,\"max_length\":65536}\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<"\n";return 2; }
}
