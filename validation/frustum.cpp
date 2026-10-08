#include "native_slice.h"
#include "frustum.h"
#include <immintrin.h>
#include <intrin.h>
#include <atomic>
#include <bit>
#include <cstring>
#include <iostream>

using GetIndex = unsigned (*)();
using GetEpoch = int* (*)();
extern "C" __declspec(dllimport) unsigned get_index();
extern "C" __declspec(dllimport) int* get_epoch();
extern "C" __declspec(dllimport) unsigned get_template_offset();
struct Snapshot {
    std::array<unsigned char, 128> cache;
    std::array<unsigned char, 0x2a0c> tls;
    int cached_frame, global_epoch;
    bool operator==(const Snapshot&) const = default;
};
Snapshot capture() {
    auto& c = frustum_context;
    Snapshot s{};
    std::memcpy(s.cache.data(), c.base + 0x21A41E0, s.cache.size());
    std::memcpy(s.tls.data(), reinterpret_cast<unsigned char*>(c.get_epoch()) - 0x2a08, s.tls.size());
    s.cached_frame = c.at<int>(0x2079EA0); s.global_epoch = c.at<int>(0x20DA464);
    return s;
}
void restore(const Snapshot& s) {
    auto& c = frustum_context;
    std::memcpy(c.base + 0x21A41E0, s.cache.data(), s.cache.size());
    std::memcpy(reinterpret_cast<unsigned char*>(c.get_epoch()) - 0x2a08, s.tls.data(), s.tls.size());
    c.at<int>(0x2079EA0) = s.cached_frame; c.at<int>(0x20DA464) = s.global_epoch;
}
void cold() {
    auto& c = frustum_context;
    std::memset(c.base + 0x21A41E0, 0, 128);
    c.at<int>(0x2079EA0) = 0; c.at<int>(0x20DA464) = INT_MIN;
    c.at<int>(0x3275610) = 7; *c.get_epoch() = INT_MIN;
}

constexpr unsigned workers = 6;
HANDLE constructor_release{}, all_waiting{}, start_workers{};
std::atomic<unsigned> constructor_calls{}, waiting_threads{};
unsigned failing_constructor{}, exception_constructor_calls{};
thread_local bool marked_waiter{};
void (*cloned_constructor)(float*){};
// Explicit three-argument CRT callback adapter. It calls the actual OS function;
// this is not a successful-return substitute for synchronization.
void sleep_cv3(CONDITION_VARIABLE* cv, CRITICAL_SECTION* lock, DWORD milliseconds) {
    if (all_waiting && !marked_waiter) {
        marked_waiter = true;
        if (waiting_threads.fetch_add(1) + 1 == workers - 1) SetEvent(all_waiting);
    }
    SleepConditionVariableCS(cv, lock, milliseconds);
}
void instrumented_constructor(float* plane) {
    if (constructor_calls.fetch_add(1) == 0)
        require(WaitForSingleObject(constructor_release, INFINITE) == WAIT_OBJECT_0, "Constructor release failed");
    cloned_constructor(plane);
}
void throwing_constructor(float* plane) {
    if (++exception_constructor_calls == failing_constructor) throw 73;
    cloned_constructor(plane);
}
bool incorrect_frustum(const float*, const void*) { return false; }

struct Patch {
    unsigned char* entry;
    std::array<unsigned char, 14> original;
    Patch(unsigned char* p, void (*target)(float*)) : entry(p) {
        std::memcpy(original.data(), entry, original.size());
        std::array<unsigned char, 14> replacement{0xff, 0x25, 0, 0, 0, 0};
        std::memcpy(replacement.data() + 6, &target, sizeof(target));
        write(replacement.data());
    }
    void write(const unsigned char* data) {
        DWORD protection{}, ignored{};
        require(VirtualProtect(entry, original.size(), PAGE_READWRITE, &protection), "Patch protection failed");
        std::memcpy(entry, data, original.size());
        require(VirtualProtect(entry, original.size(), protection, &ignored), "Patch restore protection failed");
        require(FlushInstructionCache(GetCurrentProcess(), entry, original.size()), "Patch flush failed");
    }
    ~Patch() { write(original.data()); }
};

struct Camera { std::array<unsigned char, 0x180> bytes{}; };
void initialize_camera(Camera& camera, bool ortho) {
    camera.bytes.fill(0);
    auto transform = reinterpret_cast<float*>(camera.bytes.data() + 0x7c);
    transform[6] = transform[2] = transform[4] = 1.f;
    auto frustum = reinterpret_cast<float*>(camera.bytes.data() + 0x150);
    frustum[0] = -1.f; frustum[1] = 1.f; frustum[2] = 1.f; frustum[3] = -1.f;
    frustum[4] = 1.f; frustum[5] = 100.f;
    camera.bytes[0x168] = ortho ? 1 : 0;
}
struct ThreadCall {
    Frustum function;
    const float* bound;
    const void* camera;
    bool result{};
    int epoch{};
    unsigned mxcsr{};
};
DWORD WINAPI thread_entry(void* parameter) {
    auto& call = *static_cast<ThreadCall*>(parameter);
    require(*frustum_context.get_epoch() == INT_MIN, "Loader did not initialize worker TLS epoch");
    require(WaitForSingleObject(start_workers, INFINITE) == WAIT_OBJECT_0, "Worker start failed");
    _mm_setcsr(0x1f80);
    call.result = call.function(call.bound, call.camera);
    call.epoch = *frustum_context.get_epoch(); call.mxcsr = _mm_getcsr();
    return 0;
}
std::array<ThreadCall, workers> concurrent(Frustum function, const float* bound, const void* camera) {
    cold();
    constructor_calls = 0; waiting_threads = 0;
    constructor_release = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    all_waiting = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    start_workers = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    require(constructor_release && all_waiting && start_workers, "Concurrency events failed");
    std::array<ThreadCall, workers> calls{};
    std::array<HANDLE, workers> threads{};
    for (unsigned i = 0; i < workers; ++i) {
        calls[i] = {function, bound, camera};
        threads[i] = CreateThread(nullptr, 0, thread_entry, &calls[i], 0, nullptr);
        require(threads[i], "CreateThread failed");
    }
    SetEvent(start_workers);
    require(WaitForSingleObject(all_waiting, INFINITE) == WAIT_OBJECT_0, "Waiter barrier failed");
    SetEvent(constructor_release);
    require(WaitForMultipleObjects(workers, threads.data(), TRUE, INFINITE) == WAIT_OBJECT_0, "Worker join failed");
    require(constructor_calls == 6 && waiting_threads == workers - 1, "Initialization count/contended wait mismatch");
    for (auto thread : threads) CloseHandle(thread);
    CloseHandle(constructor_release); CloseHandle(all_waiting); CloseHandle(start_workers);
    constructor_release = all_waiting = start_workers = nullptr;
    return calls;
}

int main(int argc, char** argv) {
    const auto initial_mxcsr = _mm_getcsr();
    try {
        require(argc == 3 || (argc == 4 && std::string(argv[3]) == "--negative-control"),
                "Slice and owned TLS DLL paths required");
        const Frustum candidate_function = argc == 4 ? incorrect_frustum : reconstruct_frustum;
        auto dll = LoadLibraryExA(argv[2], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        require(dll, "Owned TLS DLL load failed");
        require(get_template_offset() == 16, "Compiler TLS template layout changed");
        if (*get_epoch() != INT_MIN) {
            auto image = reinterpret_cast<unsigned char*>(dll);
            auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
            auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image + dos->e_lfanew);
            auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
            auto tls = reinterpret_cast<IMAGE_TLS_DIRECTORY64*>(image + directory.VirtualAddress);
            std::cerr << "TLS diagnostic index=" << get_index() << " epoch=" << *get_epoch()
                << " epoch_pointer=" << get_epoch() << " module=" << dll
                << " runtime_directory_rva=" << directory.VirtualAddress << " size=" << directory.Size
                << " raw_start=" << reinterpret_cast<void*>(tls->StartAddressOfRawData)
                << " index_address=" << reinterpret_cast<void*>(tls->AddressOfIndex)
                << " index_contents=" << *reinterpret_cast<unsigned*>(tls->AddressOfIndex) << "\n";
        }
        require(*get_epoch() == INT_MIN, "Initial loader TLS epoch mismatch");
        NativeSlice slice(argv[1]), constructor_copy(argv[1]);
        frustum_context = {slice.base, get_epoch, nullptr};
        auto& c = frustum_context;
        c.at<unsigned>(0x369A278) = get_index();
        auto kernel = GetModuleHandleW(L"kernel32.dll");
        auto runtime = GetModuleHandleW(L"vcruntime140.dll");
        require(kernel && runtime, "Host OS/MSVC runtime missing");
        const std::pair<unsigned, const char*> imports[] = {
            {0x17C8250, "LeaveCriticalSection"}, {0x17C8258, "EnterCriticalSection"},
            {0x17C8128, "SetEvent"}, {0x17C8130, "ResetEvent"}, {0x17C8138, "WaitForSingleObjectEx"},
            {0x17C8A58, "__CxxFrameHandler3"}};
        for (auto [rva, name] : imports) {
            auto address = GetProcAddress(rva == 0x17C8A58 ? runtime : kernel, name);
            require(address, "Required host import missing"); c.at<FARPROC>(rva) = address;
        }
        c.at<void*>(0x17C93F8) = c.base + 0x1627B90;
        auto lock = reinterpret_cast<CRITICAL_SECTION*>(c.base + 0x369A230);
        require(InitializeCriticalSectionAndSpinCount(lock, 4000), "Critical section initialization failed");
        InitializeConditionVariable(reinterpret_cast<CONDITION_VARIABLE*>(c.base + 0x369A258));
        auto cookie = c.at<std::uint64_t>(0x20DA488);
        c.at<std::uint64_t>(0x369A268) = _rotl64(reinterpret_cast<std::uint64_t>(sleep_cv3), static_cast<int>(cookie & 63)) ^ cookie;
        c.at<std::uint64_t>(0x369A270) = _rotl64(reinterpret_cast<std::uint64_t>(WakeAllConditionVariable), static_cast<int>(cookie & 63)) ^ cookie;
        cloned_constructor = reinterpret_cast<void (*)(float*)>(constructor_copy.base + 0xEF7430);
        auto original = reinterpret_cast<Frustum>(c.base + 0x224660);
        Camera cameras[2];
        float bound[4]{0, 0, 5, 1};
        std::uint64_t trials = 0;
        std::uint64_t random = 0x9bc78de314;
        auto random_finite = [&random]() {
            random ^= random << 13; random ^= random >> 7; random ^= random << 17;
            return static_cast<float>(static_cast<int>(random % 20001) - 10000) / 1024.f;
        };
        constexpr std::uint32_t special[] = {0, 0x80000000, 1, 0x3f800000, 0xbf800000,
            0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345};
        for (unsigned fp = 0; fp < 16; ++fp)
        for (unsigned fixture = 0; fixture < 512; ++fixture) {
            initialize_camera(cameras[0], fixture % 2);
            initialize_camera(cameras[1], fixture % 2);
            reinterpret_cast<float*>(cameras[1].bytes.data() + 0x7c)[9] = 2.f;
            bound[0] = static_cast<float>(static_cast<int>(fixture % 13) - 6);
            bound[1] = static_cast<float>(static_cast<int>(fixture % 17) - 8);
            bound[2] = static_cast<float>(fixture % 121);
            bound[3] = 1.f;
            if (fixture >= 256) {
                for (auto& camera : cameras) {
                    auto transform = reinterpret_cast<float*>(camera.bytes.data() + 0x7c);
                    auto f = reinterpret_cast<float*>(camera.bytes.data() + 0x150);
                    for (unsigned i = 0; i < 12; ++i) transform[i] = random_finite();
                    for (unsigned i = 0; i < 6; ++i) f[i] = random_finite();
                }
            } else if (fixture >= 128) {
                if (fixture % 3 == 0) bound[(fixture / 3) % 4] = std::bit_cast<float>(special[fixture % 9]);
                else {
                    auto location = fixture % 3 == 1 ? 0x150 : 0x7c;
                    reinterpret_cast<float*>(cameras[0].bytes.data() + location)[fixture % 6] = std::bit_cast<float>(special[fixture % 9]);
                }
            }
            cold();
            const unsigned control = 0x1f80u | ((fp & 3) << 13) | (((fp >> 2) & 1) << 6) | ((fp >> 3) << 15);
            for (unsigned step = 0; step < 7; ++step) {
                const void* camera = step == 0 ? nullptr : cameras[step >= 3 ? 1 : 0].bytes.data();
                if (step == 4 || step == 6) ++c.at<int>(0x3275610);
                if (step == 5) reinterpret_cast<float*>(cameras[1].bytes.data() + 0x7c)[9] = -4.f;
                auto before = capture();
                std::array<unsigned char, sizeof(cameras)> camera_before{};
                std::array<unsigned char, sizeof(bound)> bound_before{};
                std::memcpy(camera_before.data(), cameras, sizeof(cameras));
                std::memcpy(bound_before.data(), bound, sizeof(bound));
                _mm_setcsr(control); auto result_a = original(bound, camera); auto mxcsr_a = _mm_getcsr();
                auto after_a = capture();
                require(std::memcmp(camera_before.data(), cameras, sizeof(cameras)) == 0
                    && std::memcmp(bound_before.data(), bound, sizeof(bound)) == 0, "Original wrote input fixture");
                restore(before);
                _mm_setcsr(control); auto result_b = candidate_function(bound, camera); auto mxcsr_b = _mm_getcsr();
                auto after_b = capture(); _mm_setcsr(initial_mxcsr);
                require(std::memcmp(camera_before.data(), cameras, sizeof(cameras)) == 0
                    && std::memcmp(bound_before.data(), bound, sizeof(bound)) == 0, "Reconstruction wrote input fixture");
                if (result_a != result_b || mxcsr_a != mxcsr_b || (mxcsr_a & ~0x3fu) != control || !(after_a == after_b)) {
                    std::cerr << "Frustum differential failure fixture=" << fixture << " step=" << step << " fp=" << fp
                        << " result=" << result_a << "/" << result_b << " mxcsr=" << mxcsr_a << "/" << mxcsr_b
                        << " state_equal=" << (after_a == after_b) << "\n"; return 1;
                }
                ++trials;
            }
        }
        initialize_camera(cameras[0], false); bound[0] = bound[1] = 0; bound[2] = 5; bound[3] = 1;
        _mm_setcsr(0x1f80);
        std::array<ThreadCall, workers> original_threads;
        Snapshot original_concurrent;
        {
            Patch instrumentation(c.base + 0xEF7430, instrumented_constructor);
            original_threads = concurrent(original, bound, cameras[0].bytes.data());
            original_concurrent = capture();
        }
        c.constructor_override = instrumented_constructor;
        auto reconstructed_threads = concurrent(candidate_function, bound, cameras[0].bytes.data());
        require(capture() == original_concurrent, "Concurrent final state mismatch");
        for (unsigned i = 0; i < workers; ++i)
            require(original_threads[i].result && reconstructed_threads[i].result
                && original_threads[i].epoch == INT_MIN + 1 && reconstructed_threads[i].epoch == INT_MIN + 1
                && original_threads[i].mxcsr == reconstructed_threads[i].mxcsr, "Concurrent per-thread mismatch");
        c.constructor_override = nullptr;
        for (unsigned candidate = 0; candidate < 2; ++candidate)
        for (unsigned failure = 1; failure <= 6; ++failure) {
            failing_constructor = failure; exception_constructor_calls = 0;
            cold(); bool caught = false;
            try {
                if (candidate == 0) {
                    Patch injection(c.base + 0xEF7430, throwing_constructor);
                    original(bound, cameras[0].bytes.data());
                } else {
                    c.constructor_override = throwing_constructor;
                    candidate_function(bound, cameras[0].bytes.data());
                }
            } catch (int value) { require(value == 73, "Wrong injected exception"); caught = true; }
            c.constructor_override = nullptr;
            require(caught && c.at<int>(0x21A4250) == 0 && c.at<int>(0x20DA464) == INT_MIN
                    && *c.get_epoch() == INT_MIN && lock->RecursionCount == 0, "Exception abort cleanup failed");
            auto function = candidate == 0 ? original : candidate_function;
            require(function(bound, cameras[0].bytes.data()) && c.at<int>(0x21A4250) == INT_MIN + 1,
                    "Initialization retry after exception failed");
        }
        _mm_setcsr(initial_mxcsr);
        DeleteCriticalSection(lock);
        std::cout << "{\"status\":\"pass\",\"serial_trials\":" << trials
            << ",\"camera_fixtures\":512,\"fp_modes\":16,\"sequence_steps\":7,"
            << "\"loader_tls\":true,\"contending_threads\":6,\"waiters\":5,\"constructors_per_initialization\":6,"
            << "\"original_msvc_cleanup_and_retry\":true,\"reconstruction_cleanup_and_retry\":true,\"injected_exception_positions\":6,"
            << "\"concurrency_scope\":\"one immutable camera, same frame\",\"game_launched\":false}\n";
        // TLS owner remains loaded until process termination so live TLS references
        // cannot outlast its module; the OS releases its allocation at exit.
    } catch (const std::exception& error) {
        _mm_setcsr(initial_mxcsr); std::cerr << error.what() << "\n"; return 2;
    }
}
