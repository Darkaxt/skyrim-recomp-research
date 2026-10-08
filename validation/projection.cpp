#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <immintrin.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
#include "projection.h"

static_assert(offsetof(ProjectionArgs, result) == 52);
static_assert(offsetof(ProjectionArgs, mxcsr) == 56);
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

struct Slice {
    unsigned char* base{};
    RUNTIME_FUNCTION function{};
    bool registered{};
    Projection original{};
    explicit Slice(const char* path) {
        std::ifstream file(path, std::ios::binary);
        std::array<char, 8> magic{};
        std::array<std::uint32_t, 6> h{};
        file.read(magic.data(), 8);
        file.read(reinterpret_cast<char*>(h.data()), sizeof(h));
        require(file.good() && std::string(magic.data(), 8) == "SKYSLICE" && h[0] == 1,
                "Invalid isolated slice pack");
        base = static_cast<unsigned char*>(VirtualAlloc(nullptr, h[5], MEM_RESERVE, PAGE_NOACCESS));
        require(base, "Reserve failed");
        std::map<std::uint32_t, DWORD> pages;
        for (unsigned i = 0; i < h[1]; ++i) {
            std::array<std::uint32_t, 3> p{};
            file.read(reinterpret_cast<char*>(p.data()), sizeof(p));
            require(p[0] + p[1] <= h[5] && (p[2] == 1 || p[2] == 2), "Invalid piece bounds");
            for (auto page = p[0] & ~4095u; page < p[0] + p[1]; page += 4096) {
                if (!pages.contains(page))
                    require(VirtualAlloc(base + page, 4096, MEM_COMMIT, PAGE_READWRITE), "Commit failed");
                pages[page] = p[2] == 1 ? PAGE_EXECUTE_READ : PAGE_READONLY;
            }
            file.read(reinterpret_cast<char*>(base + p[0]), p[1]);
            require(file.good(), "Truncated piece");
        }
        for (auto [page, protection] : pages) {
            DWORD old{};
            require(VirtualProtect(base + page, 4096, protection, &old), "Protection failed");
        }
        require(FlushInstructionCache(GetCurrentProcess(), base + h[2], h[3] - h[2]), "Flush failed");
        function = {h[2], h[3], h[4]};
        require(RtlAddFunctionTable(&function, 1, reinterpret_cast<DWORD64>(base)), "Unwind registration failed");
        registered = true;
        original = reinterpret_cast<Projection>(base + h[2]);
    }
    void verify_unwind() {
        DWORD64 image{};
        auto found = RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(original), &image, nullptr);
        require(found && image == reinterpret_cast<DWORD64>(base) && found->BeginAddress == function.BeginAddress,
                "Original function not found by OS unwinder");
        alignas(16) std::array<std::uint64_t, 8> stack{0x12345678, 0x87654321, 0, 0x1122334455667788};
        CONTEXT context{};
        context.Rip = image + 0xEF11A3; // SUB rsp,18h and save XMM6 completed.
        context.Rsp = reinterpret_cast<DWORD64>(stack.data());
        void* handler_data{};
        DWORD64 establisher{};
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, image, context.Rip, found, &context,
                         &handler_data, &establisher, nullptr);
        require(context.Rip == stack[3] && context.Rsp == reinterpret_cast<DWORD64>(stack.data() + 4)
                && context.Xmm6.Low == stack[0] && static_cast<std::uint64_t>(context.Xmm6.High) == stack[1],
                "Original unwind stack/XMM6 recovery mismatch");
    }
    ~Slice() {
        if (registered) RtlDeleteFunctionTable(&function);
        if (base) VirtualFree(base, 0, MEM_RELEASE);
    }
};

struct Arena {
    unsigned char* reservation;
    unsigned char* page;
    Arena() {
        reservation = static_cast<unsigned char*>(VirtualAlloc(nullptr, 12288, MEM_RESERVE, PAGE_NOACCESS));
        require(reservation, "Guard reservation failed");
        page = static_cast<unsigned char*>(VirtualAlloc(reservation + 4096, 4096, MEM_COMMIT, PAGE_READWRITE));
        require(page, "Guard data commit failed");
    }
    ~Arena() { VirtualFree(reservation, 0, MEM_RELEASE); }
};

using Fixture = std::array<std::uint32_t, 40>;
std::uint64_t random_state = 0xd17ec7a16329;
std::uint32_t random_bits() {
    random_state ^= random_state << 13; random_state ^= random_state >> 7; random_state ^= random_state << 17;
    return static_cast<std::uint32_t>(random_state);
}
std::vector<Fixture> corpus() {
    Fixture identity{};
    identity[0] = identity[5] = identity[10] = identity[15] = 0x3f800000;
    identity[16] = identity[19] = 0; identity[17] = identity[18] = 0x3f800000;
    identity[20] = 0x3f000000; identity[21] = 0xbf000000; identity[22] = 0x3e800000;
    identity[23] = 0x358637bd; // tolerance separate from permitted output locations
    identity[24] = 0x7fc12345; identity[25] = 0x80000000; identity[26] = 0xdeadbeef;
    std::vector<Fixture> cases{identity};
    constexpr std::uint32_t special[] = {0, 0x80000000, 1, 0x80000001, 0x007fffff,
        0x00800000, 0x3f7fffff, 0x3f800000, 0x3f800001, 0xbf800000, 0x7f7fffff,
        0xff7fffff, 0x7f800000, 0xff800000, 0x7fc12345, 0xffc54321, 0x7f812345};
    for (unsigned offset : {0u, 3u, 12u, 13u, 14u, 15u, 16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u})
        for (auto bits : special) { auto f = identity; f[offset] = bits; cases.push_back(f); }
    for (unsigned i = 0; i < 1536; ++i) {
        auto f = identity;
        for (unsigned j = 0; j < 23; ++j) {
            auto bits = random_bits();
            if (i % 2 == 0) bits = std::bit_cast<std::uint32_t>(static_cast<float>(static_cast<int>(bits % 200001) - 100000) / 1024.f);
            f[j] = bits;
        }
        f[23] = special[i % std::size(special)]; cases.push_back(f);
    }
    return cases;
}
ProjectionArgs arguments(Arena& a, const Fixture& f, const std::array<unsigned, 3>& offsets, bool at_end) {
    std::memset(a.page, 0xa5, 4096);
    auto data = a.page + (at_end ? 4096 - sizeof(Fixture) : 0);
    std::memcpy(data, f.data(), sizeof(f));
    auto floats = reinterpret_cast<float*>(data);
    return {floats, floats + 16, floats + 20, floats + offsets[0], floats + offsets[1], floats + offsets[2],
            std::bit_cast<float>(f[23]), 0, {}, 0};
}
extern "C" bool incorrect_projection(const float*, const float*, const float*, float*, float*, float*, float) { return false; }

double benchmark(Projection function, Arena& arena, const Fixture& fixture) {
    auto a = arguments(arena, fixture, {24, 25, 26}, false);
    LARGE_INTEGER frequency{}, begin{}, end{};
    QueryPerformanceFrequency(&frequency);
    std::array<double, 5> timings{};
    volatile unsigned consumed = 0;
    for (auto& t : timings) {
        QueryPerformanceCounter(&begin);
        unsigned value = 0;
        for (unsigned i = 0; i < 200000; ++i)
            value += function(a.matrix, a.port, a.point, a.x, a.y, a.z, a.tolerance);
        QueryPerformanceCounter(&end);
        consumed = value;
        t = static_cast<double>(end.QuadPart - begin.QuadPart) * 1e9 / frequency.QuadPart / 200000;
    }
    require(consumed == 200000, "Benchmark input unexpectedly rejected");
    std::sort(timings.begin(), timings.end());
    return timings[2];
}

int main(int argc, char** argv) {
    const auto initial_mxcsr = _mm_getcsr();
    try {
        require(argc >= 2, "Slice pack argument required");
        Slice slice(argv[1]); slice.verify_unwind();
        const bool negative = argc == 3 && std::string(argv[2]) == "--negative-control";
        Projection candidate = negative ? incorrect_projection : reconstruct_projection;
        auto cases = corpus();
        Arena original, replacement;
        constexpr std::array<std::array<unsigned, 3>, 9> aliases{{
            {24, 25, 26}, {24, 24, 26}, {24, 25, 24}, {24, 24, 24},
            {20, 21, 22}, {0, 5, 10}, {16, 17, 18}, {20, 20, 20}, {15, 15, 15}}};
        std::uint64_t trials = 0, accepted = 0, rejected = 0;
        for (unsigned rounding = 0; rounding < 4; ++rounding)
        for (unsigned denormals = 0; denormals < 4; ++denormals) {
            const unsigned control = 0x1f80 | (rounding << 13) | ((denormals & 1) << 6) | ((denormals >> 1) << 15);
            for (unsigned i = 0; i < cases.size(); ++i)
            for (unsigned alias = 0; alias < aliases.size(); ++alias) {
                auto a = arguments(original, cases[i], aliases[alias], i % 2);
                auto b = arguments(replacement, cases[i], aliases[alias], i % 2);
                std::array<unsigned char, 4096> before{};
                std::memcpy(before.data(), original.page, before.size());
                _mm_setcsr(control);
                auto abi_a = projection_probe(slice.original, &a);
                _mm_setcsr(control);
                auto abi_b = projection_probe(candidate, &b);
                _mm_setcsr(initial_mxcsr);
                bool permitted_writes = true;
                const auto fixture_offset = i % 2 ? 4096 - sizeof(Fixture) : 0;
                for (unsigned byte = 0; byte < 4096; ++byte) {
                    if (before[byte] == original.page[byte]) continue;
                    bool allowed = false;
                    for (auto output : aliases[alias])
                        allowed |= byte >= fixture_offset + output * 4 && byte < fixture_offset + output * 4 + 4;
                    permitted_writes &= allowed;
                }
                if (!permitted_writes || (a.mxcsr & ~0x3fu) != control
                    || abi_a || abi_b || a.result != b.result || a.mxcsr != b.mxcsr
                    || std::memcmp(original.page, replacement.page, 4096) != 0) {
                    unsigned changed = 4096;
                    for (unsigned j = 0; j < 4096; ++j) if (original.page[j] != replacement.page[j]) {changed = j; break;}
                    std::cerr << "Differential failure case=" << i << " alias=" << alias
                        << " rounding=" << rounding << " denormals=" << denormals
                        << " result=" << unsigned(a.result) << "/" << unsigned(b.result)
                        << " mxcsr=" << a.mxcsr << "/" << b.mxcsr
                        << " abi=" << abi_a << "/" << abi_b << " permitted_writes=" << permitted_writes
                        << " first_byte=" << changed << "\n";
                    return 1;
                }
                ++trials; if (a.result) ++accepted; else ++rejected;
            }
        }
        _mm_setcsr(0x1f80);
        const auto original_ns = benchmark(slice.original, original, cases[0]);
        const auto reconstructed_ns = benchmark(candidate, replacement, cases[0]);
        _mm_setcsr(initial_mxcsr);
        std::cout << "{\"status\":\"pass\",\"trials\":" << trials
            << ",\"accepted\":" << accepted << ",\"rejected\":" << rejected
            << ",\"fixture_cases\":" << cases.size() << ",\"alias_layouts\":9,\"fp_modes\":16,"
            << "\"abi_preservation\":true,\"guard_pages\":true,\"permitted_writes\":true,\"unwind_recovery\":true,"
            << "\"median_original_ns\":" << original_ns << ",\"median_reconstructed_ns\":" << reconstructed_ns << "}\n";
    } catch (const std::exception& error) {
        _mm_setcsr(initial_mxcsr);
        std::cerr << error.what() << "\n"; return 2;
    }
}
