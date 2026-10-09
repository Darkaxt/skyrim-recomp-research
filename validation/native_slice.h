#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

inline void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

// Maps only the explicitly listed pages. There is no PE loader, entry point,
// import resolution or game startup. Kind 3 is declared mutable fixture storage.
struct NativeSlice {
    unsigned char* base{};
    RUNTIME_FUNCTION function{};
    std::vector<RUNTIME_FUNCTION> functions;
    bool registered{};
    explicit NativeSlice(const char* path) {
        std::ifstream file(path, std::ios::binary);
        std::array<char, 8> magic{};
        std::array<std::uint32_t, 6> h{};
        file.read(magic.data(), 8);
        file.read(reinterpret_cast<char*>(h.data()), sizeof(h));
        require(file.good() && std::string(magic.data(), 8) == "SKYSLICE" && (h[0] == 1 || h[0] == 2),
                "Invalid isolated slice pack");
        if (h[0] == 2) {
            std::uint32_t count{};
            file.read(reinterpret_cast<char*>(&count), sizeof(count));
            require(count > 0 && count < 128, "Invalid unwind function count");
            functions.resize(count);
            file.read(reinterpret_cast<char*>(functions.data()), count * sizeof(RUNTIME_FUNCTION));
            require(file.good(), "Truncated unwind table");
        } else functions.push_back({h[2], h[3], h[4]});
        base = static_cast<unsigned char*>(VirtualAlloc(nullptr, h[5], MEM_RESERVE, PAGE_NOACCESS));
        require(base, "Reserve failed");
        try {
            std::map<std::uint32_t, DWORD> pages;
            for (unsigned i = 0; i < h[1]; ++i) {
                std::array<std::uint32_t, 3> p{};
                file.read(reinterpret_cast<char*>(p.data()), sizeof(p));
                require(p[0] <= h[5] && p[1] <= h[5] - p[0] && p[2] >= 1 && p[2] <= 3,
                        "Invalid piece bounds");
                for (auto page = p[0] & ~4095u; page < p[0] + p[1]; page += 4096) {
                    if (!pages.contains(page))
                        require(VirtualAlloc(base + page, 4096, MEM_COMMIT, PAGE_READWRITE), "Commit failed");
                    // Accumulate requirements for the complete page. Piece order
                    // must not remove execution or write permission from a peer.
                    pages[page] |= 1u << (p[2] - 1);
                    require((pages[page] & 5u) != 5u, "Writable executable page unsupported");
                }
                file.read(reinterpret_cast<char*>(base + p[0]), p[1]);
                require(file.good(), "Truncated piece");
            }
            for (auto [page, rights] : pages) {
                const DWORD protection = rights & 1u ? PAGE_EXECUTE_READ :
                                         rights & 4u ? PAGE_READWRITE : PAGE_READONLY;
                DWORD old{};
                require(VirtualProtect(base + page, 4096, protection, &old), "Protection failed");
                if (rights & 1u)
                    require(FlushInstructionCache(GetCurrentProcess(), base + page, 4096), "Flush failed");
            }
            function = {h[2], h[3], h[4]};
            require(RtlAddFunctionTable(functions.data(), static_cast<DWORD>(functions.size()),
                                        reinterpret_cast<DWORD64>(base)), "Unwind registration failed");
            registered = true;
        } catch (...) {
            if (registered) RtlDeleteFunctionTable(functions.data());
            VirtualFree(base, 0, MEM_RELEASE);
            throw;
        }
    }
    NativeSlice(const NativeSlice&) = delete;
    NativeSlice& operator=(const NativeSlice&) = delete;
    ~NativeSlice() {
        if (registered) RtlDeleteFunctionTable(functions.data());
        if (base) VirtualFree(base, 0, MEM_RELEASE);
    }
};
