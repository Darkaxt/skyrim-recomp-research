// Integration adapter for pcrecomp's explicit CPU-state calling convention.
// Generated instruction semantics are included separately, never copied here.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <immintrin.h>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include "projection.h"
#include "dispatch.h"
#include "frustum.h"
#include "cpu64.h"
static_assert(sizeof(CPU) == 448 && offsetof(CPU, xmm) == 184);

std::int64_t g_image_delta;
extern "C" void automatic_set_base(unsigned char* base) {
    g_image_delta = static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(base)) - 0x140000000LL;
}

#include "candidate-generated.cpp"

#if defined(AUTOMATIC_PROJECTION)
void dispatch(CPU*, std::uint64_t) { throw std::runtime_error("Unexpected generated call"); }
void dispatch_jmp(CPU*, std::uint64_t) { throw std::runtime_error("Unexpected generated tail call"); }
extern "C" bool reconstruct_projection(const float* matrix, const float* port,
                                        const float* point, float* x, float* y,
                                        float* z, float tolerance) {
    alignas(16) unsigned char stack[256]{};
    CPU c{};
    const auto entry_rsp = reinterpret_cast<std::uintptr_t>(stack + 136);
    c.rsp = entry_rsp;
    c.rcx = reinterpret_cast<std::uintptr_t>(matrix);
    c.rdx = reinterpret_cast<std::uintptr_t>(port);
    c.r8 = reinterpret_cast<std::uintptr_t>(point);
    c.r9 = reinterpret_cast<std::uintptr_t>(x);
    wr64(c.rsp + 0x28, reinterpret_cast<std::uintptr_t>(y));
    wr64(c.rsp + 0x30, reinterpret_cast<std::uintptr_t>(z));
    wrss(c.rsp + 0x38, tolerance);
    c.xmm[6].u64[0] = 0x123456789abcdef0ULL;
    c.xmm[6].u64[1] = 0xfedcba9876543210ULL;
    L_000140EF1180(&c);
    if (c.rsp != entry_rsp + 8 || c.xmm[6].u64[0] != 0x123456789abcdef0ULL
        || c.xmm[6].u64[1] != 0xfedcba9876543210ULL)
        throw std::runtime_error("Generated CPU stack/XMM6 preservation failure");
    return static_cast<std::uint8_t>(c.rax) != 0;
}
#elif defined(AUTOMATIC_DISPATCH)
void*** dispatch_table_slot;
void dispatch(CPU* c, std::uint64_t target) {
    auto function = reinterpret_cast<GetValue>(target);
    c->xmm[0].f32[0] = function(reinterpret_cast<void*>(c->rcx), static_cast<int>(c->rdx));
    c->rsp += 8; // The native callback consumed its own host return slot.
}
void dispatch_jmp(CPU*, std::uint64_t) { throw std::runtime_error("Unexpected generated tail call"); }
extern "C" float reconstruct_dispatch(void* receiver, int index) {
    alignas(16) unsigned char stack[256]{};
    CPU c{};
    const auto entry_rsp = reinterpret_cast<std::uintptr_t>(stack + 136);
    c.rsp = entry_rsp;
    c.rcx = reinterpret_cast<std::uintptr_t>(receiver);
    c.rdx = static_cast<std::uint32_t>(index);
    L_000140447180(&c);
    if (c.rsp != entry_rsp + 8)
        throw std::runtime_error("Generated dispatch stack preservation failure");
    return c.xmm[0].f32[0];
}
#elif defined(AUTOMATIC_FRUSTUM)
FrustumContext frustum_context;
void dispatch(CPU* c, std::uint64_t target) {
#if defined(AUTOMATIC_CLEANUP)
    if (target == 0x140EF7430ULL && frustum_context.constructor_override) {
        frustum_context.constructor_override(reinterpret_cast<float*>(c->rcx));
        c->rsp += 8;
        return;
    }
    if (automatic_generated_dispatch(c, target)) return;
#endif
    switch (target) {
    case 0x140EF7430ULL:
        if (frustum_context.constructor_override) {
            frustum_context.constructor_override(reinterpret_cast<float*>(c->rcx));
            c->rsp += 8;
        } else L_000140EF7430(c);
        break;
    case 0x140EF74A0ULL: L_000140EF74A0(c); break;
    case 0x140EF76F0ULL: L_000140EF76F0(c); break;
    case 0x140EF7710ULL: L_000140EF7710(c); break;
    case 0x1415A7D0CULL: // Original native CRT static-initialization header.
    case 0x1415A7CACULL: // Original native CRT footer.
    case 0x1415A7C7CULL: // Original native CRT abort, invoked by lifted cleanup.
        frustum_context.helper(static_cast<std::uintptr_t>(target - 0x140000000ULL),
                               reinterpret_cast<int*>(c->rcx));
        c->rsp += 8;
        break;
    default: throw std::runtime_error("Unsupported generated frustum call");
    }
}
void dispatch_jmp(CPU* c, std::uint64_t target) { dispatch(c, target); }
extern "C" bool reconstruct_frustum(const float* bound, const void* camera) {
    alignas(16) unsigned char stack[4096]{};
    CPU c{};
    const auto entry_rsp = reinterpret_cast<std::uintptr_t>(stack + 3960);
    c.rsp = entry_rsp;
    c.rcx = reinterpret_cast<std::uintptr_t>(bound);
    c.rdx = reinterpret_cast<std::uintptr_t>(camera);
    L_000140224660(&c);
    if (c.rsp != entry_rsp + 8)
        throw std::runtime_error("Generated frustum stack preservation failure");
    return static_cast<std::uint8_t>(c.rax) != 0;
}
#endif
