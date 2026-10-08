#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <exception>

struct AutomaticAction { int next; std::uint64_t target; };
struct AutomaticState { std::uint32_t pc; int state; };
struct AutomaticCleanup {
    RUNTIME_FUNCTION function;
    const AutomaticAction* actions;
    unsigned action_count;
    const AutomaticState* states;
    unsigned state_count;
};

// Metadata is generated from pinned inputs. No engine-specific guard/reset logic.
inline void automatic_cleanup_unwind(const CPU& saved, const AutomaticCleanup& info,
                                     std::uint32_t call_pc) noexcept {
#if defined(AUTOMATIC_DISABLE_CLEANUP)
    (void)saved; (void)info; (void)call_pc;
#else
    int state = -1;
    for (unsigned i = 0; i < info.state_count; ++i) {
        if (info.states[i].pc > call_pc) break;
        state = info.states[i].state;
    }
    CONTEXT context{};
    context.Rip = GVA(0x140000000ULL + call_pc);
    context.Rsp = saved.rsp; context.Rbp = saved.rbp;
    context.Rbx = saved.rbx; context.Rsi = saved.rsi; context.Rdi = saved.rdi;
    context.R12 = saved.r12; context.R13 = saved.r13; context.R14 = saved.r14; context.R15 = saved.r15;
    void* data{};
    DWORD64 frame{};
    auto function = info.function;
    RtlVirtualUnwind(UNW_FLAG_NHANDLER, GVA(0x140000000ULL), context.Rip, &function,
                     &context, &data, &frame, nullptr);
    while (state >= 0) {
        if (static_cast<unsigned>(state) >= info.action_count) std::terminate();
        const auto action = info.actions[state];
        if (action.next >= state || action.next < -1) std::terminate();
        if (action.target) {
            CPU cleanup = saved;
            cleanup.rcx = 0; cleanup.rdx = frame;
            cleanup.rsp = (saved.rsp - 0x200) & ~std::uint64_t(15);
            push64(&cleanup, 0);
            dispatch(&cleanup, action.target);
        }
        state = action.next;
    }
#endif
}
