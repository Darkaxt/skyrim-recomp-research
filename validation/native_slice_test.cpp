#include "native_slice.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <new>

struct Piece { std::uint32_t rva, kind; std::vector<unsigned char> bytes; };
static void pack(const std::string& path, std::vector<Piece> pieces, bool truncate = false) {
    std::ofstream out(path, std::ios::binary);
    const char magic[8] = {'S','K','Y','S','L','I','C','E'};
    const std::array<std::uint32_t,6> header{1,static_cast<std::uint32_t>(pieces.size()),0x1000,0x1006,0x2000,0x4000};
    out.write(magic,8); out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));
    for (const auto& p : pieces) {
        const std::array<std::uint32_t,3> h{p.rva,static_cast<std::uint32_t>(p.bytes.size()),p.kind};
        out.write(reinterpret_cast<const char*>(h.data()),sizeof(h));
        if (!truncate || &p != &pieces.back()) out.write(reinterpret_cast<const char*>(p.bytes.data()),p.bytes.size());
    }
    require(out.good(),"Test pack write failed");
}
static std::vector<Piece> pieces(unsigned data_kind) {
    // Authored synthetic test: return 42. These are not game instructions.
    return {{0x1000,1,{0xb8,42,0,0,0,0xc3}}, {0x1008,data_kind,{9,8,7,6}}, {0x2000,2,{1,0,0,0}}};
}
static void failure_releases_mapping(const std::string& path, const char* expected) {
    alignas(NativeSlice) unsigned char storage[sizeof(NativeSlice)]{};
    bool rejected = false;
    try {
        auto p = new(storage) NativeSlice(path.c_str());
        p->~NativeSlice();
    } catch (const std::exception& e) {
        require(std::string(e.what()).find(expected) != std::string::npos,"Unexpected pack rejection");
        rejected = true;
    }
    require(rejected,"Required invalid-pack rejection missing");
    void* base{}; std::memcpy(&base,storage,sizeof(base));
    if (base) {
        MEMORY_BASIC_INFORMATION info{};
        require(VirtualQuery(base,&info,sizeof(info)) != 0,"VirtualQuery failed");
        if (info.State != MEM_FREE) {
            VirtualFree(base,0,MEM_RELEASE);
            throw std::runtime_error("Rejected construction leaked reserved image");
        }
    }
}
int main(int argc, char** argv) {
    try {
        require(argc==3,"Directory and test mode required");
        const std::string path=std::string(argv[1])+"/loader-"+argv[2]+".pack";
        const std::string mode=argv[2];
        if (mode=="mixed") {
            for (bool reverse : {false,true}) {
                auto p=pieces(2); if(reverse)std::reverse(p.begin(),p.end()); pack(path,p);
                NativeSlice mapped(path.c_str());
                MEMORY_BASIC_INFORMATION info{};
                require(VirtualQuery(mapped.base+0x1000,&info,sizeof(info))!=0,"Query failed");
                require(info.Protect==PAGE_EXECUTE_READ,"Mixed code/data page lost RX permission");
                require(mapped.base[0x1008]==9,"Embedded data missing");
                require(reinterpret_cast<unsigned(*)()>(mapped.base+0x1000)()==42,"Synthetic execution failed");
            }
        } else if (mode=="wx") {
            for(bool reverse:{false,true}) {
                auto p=pieces(3); if(reverse)std::reverse(p.begin(),p.end()); pack(path,p);
                failure_releases_mapping(path,"Writable executable page");
            }
        } else if (mode=="truncated") {
            pack(path,pieces(2),true); failure_releases_mapping(path,"Truncated piece");
        } else throw std::runtime_error("Unknown loader test");
        std::cout<<"{\"status\":\"pass\",\"test\":\""<<mode<<"\"}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
}
