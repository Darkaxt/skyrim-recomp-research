#include "native_slice.h"
#include "dispatch.h"
#include <immintrin.h>
#include <bit>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <vector>

struct Info { std::uint32_t padding[24]; std::uint32_t flags; };
static_assert(offsetof(Info, flags) == 0x60);
struct Receiver {
    GetValue* vtable;
    std::uint32_t value_bits;
    unsigned calls;
    void* seen_receiver;
    int seen_index;
    unsigned mode;
    Info* selected;
    void** alternative_table;
};
struct Fixture {
    void* table_a[165];
    void* table_b[165];
    Info info_a, info_b;
    Receiver receiver;
};
static_assert(sizeof(Fixture) < 4096);

float callback(void* receiver, int index) {
    auto& r = *static_cast<Receiver*>(receiver);
    ++r.calls;
    r.seen_receiver = receiver;
    r.seen_index = index;
    if (r.mode == 1) r.selected->flags ^= 0x1c;
    if (r.mode == 2) *dispatch_table_slot = r.alternative_table;
    return std::bit_cast<float>(r.value_bits);
}
GetValue vtable[2]{nullptr, callback};
float incorrect(void*, int) { return 0.f; }

struct Arena {
    unsigned char* reservation;
    unsigned char* page;
    Arena() {
        reservation = static_cast<unsigned char*>(VirtualAlloc(nullptr, 12288, MEM_RESERVE, PAGE_NOACCESS));
        require(reservation, "Guard reservation failed");
        page = static_cast<unsigned char*>(VirtualAlloc(reservation + 4096, 4096, MEM_COMMIT, PAGE_READWRITE));
        require(page, "Guard commit failed");
    }
    ~Arena() { VirtualFree(reservation, 0, MEM_RELEASE); }
};

int main(int argc, char** argv) {
    const auto initial_mxcsr = _mm_getcsr();
    try {
        require(argc >= 2, "Slice pack required");
        NativeSlice slice(argv[1]);
        auto original = reinterpret_cast<GetValue>(slice.base + slice.function.BeginAddress);
        dispatch_table_slot = reinterpret_cast<void***>(slice.base + 0x219DEC8);
        const bool negative = argc == 3 && std::string(argv[2]) == "--negative-control";
        GetValue candidate = negative ? incorrect : reconstruct_dispatch;
        std::vector<std::uint32_t> values{0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x00800000,
            0x3f7fffff, 0x3f800000, 0x3f800001, 0x411fffff, 0x41200000, 0x41200001,
            0x42c7ffff, 0x42c80000, 0x42c80001, 0xbf800000, 0x7f7fffff, 0xff7fffff,
            0x7f800000, 0xff800000, 0x7fc12345, 0xffc54321, 0x7f812345, 0xff812345};
        std::uint64_t random = 0x8fe7a3b591;
        for (unsigned i = 0; i < 64; ++i) {
            random ^= random << 13; random ^= random >> 7; random ^= random << 17;
            values.push_back(static_cast<std::uint32_t>(random));
        }
        Arena arena;
        std::uint64_t trials = 0;
        for (unsigned rounding = 0; rounding < 4; ++rounding)
        for (unsigned denormals = 0; denormals < 4; ++denormals)
        for (unsigned flags = 0; flags < 32; ++flags)
        for (int index : {0, 1, 163})
        for (unsigned mode = 0; mode < 3; ++mode)
        for (auto bits : values) {
            std::memset(arena.page, 0xa5, 4096);
            auto fixture = reinterpret_cast<Fixture*>(arena.page + (trials % 2 ? 4096 - sizeof(Fixture) : 0));
            fixture->info_a.flags = flags | 0xa5a50000u;
            fixture->info_b.flags = (flags ^ 0x1c) | 0x5a5a0000u;
            for (auto& item : fixture->table_a) item = &fixture->info_b;
            for (auto& item : fixture->table_b) item = &fixture->info_a;
            fixture->table_a[0] = fixture->table_b[0] = nullptr;
            fixture->table_a[index + 1] = &fixture->info_a;
            fixture->table_b[index + 1] = &fixture->info_b;
            fixture->receiver = {vtable, bits, 0, nullptr, -1, mode, &fixture->info_a, fixture->table_b};
            std::array<unsigned char, 4096> before{}, after{};
            std::memcpy(before.data(), arena.page, 4096);
            *dispatch_table_slot = fixture->table_a;
            const auto control = 0x1f80u | (rounding << 13) | ((denormals & 1) << 6) | ((denormals >> 1) << 15);
            _mm_setcsr(control);
            auto result_a = std::bit_cast<std::uint32_t>(original(&fixture->receiver, index));
            auto mxcsr_a = _mm_getcsr();
            auto global_a = *dispatch_table_slot;
            _mm_setcsr(initial_mxcsr);
            std::memcpy(after.data(), arena.page, 4096);
            require(fixture->receiver.calls == 1 && fixture->receiver.seen_receiver == &fixture->receiver
                    && fixture->receiver.seen_index == index, "Original callback trace mismatch");
            require(global_a == (mode == 2 ? fixture->table_b : fixture->table_a), "Original global transition mismatch");
            std::memcpy(arena.page, before.data(), 4096);
            *dispatch_table_slot = fixture->table_a;
            _mm_setcsr(control);
            auto result_b = std::bit_cast<std::uint32_t>(candidate(&fixture->receiver, index));
            auto mxcsr_b = _mm_getcsr();
            _mm_setcsr(initial_mxcsr);
            if (result_a != result_b || mxcsr_a != mxcsr_b || (mxcsr_a & ~0x3fu) != control
                || global_a != *dispatch_table_slot || std::memcmp(after.data(), arena.page, 4096)) {
                std::cerr << "Dispatch differential failure flags=" << flags << " index=" << index
                    << " callback_mode=" << mode << " input_bits=" << bits << " result=" << result_a << "/" << result_b
                    << " mxcsr=" << mxcsr_a << "/" << mxcsr_b << "\n";
                return 1;
            }
            ++trials;
        }
        std::cout << "{\"status\":\"pass\",\"trials\":" << trials
            << ",\"value_cases\":" << values.size() << ",\"flag_combinations\":32,\"indices\":[0,1,163],"
            << "\"callback_modes\":[\"return_only\",\"mutate_flags\",\"replace_global_table\"],"
            << "\"fp_modes\":16,\"guard_pages\":true,\"fixture_bytes_and_callback_trace\":true,"
            << "\"real_engine_receiver\":false}\n";
    } catch (const std::exception& error) {
        _mm_setcsr(initial_mxcsr);
        std::cerr << error.what() << "\n"; return 2;
    }
}
