#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdexcept>
#include "cpu64.h"
#include "lookup.h"
std::int64_t g_image_delta;
StringCompare lookup_compare;
extern "C" void automatic_set_base(unsigned char* base) {
    g_image_delta = reinterpret_cast<std::uintptr_t>(base) - 0x140000000ULL;
}
#include "candidate-generated.cpp"
void dispatch(CPU* c, std::uint64_t target) {
    if (target != reinterpret_cast<std::uintptr_t>(lookup_compare))
        throw std::runtime_error("Unexpected lookup import");
    c->rax = static_cast<std::uint32_t>(lookup_compare(
        reinterpret_cast<const char*>(c->rcx), reinterpret_cast<const char*>(c->rdx)));
    c->rsp += 8;
}
void dispatch_jmp(CPU*, std::uint64_t) { throw std::runtime_error("Unexpected lookup tail call"); }
extern "C" std::uint32_t automatic_lookup(const char* name) {
    alignas(16) unsigned char stack[1024]{};
    CPU c{};
    auto entry = reinterpret_cast<std::uintptr_t>(stack + 888);
    c.rsp = entry; c.rcx = reinterpret_cast<std::uintptr_t>(name);
    c.rbx = 0x1234567812345678ULL; c.rbp = 0xfedcba9876543210ULL;
    c.rsi = 0xabcdefab12345678ULL; c.rdi = 0x8765432187654321ULL;
    L_000140443320(&c);
    if (c.rsp != entry + 8 || c.rbx != 0x1234567812345678ULL || c.rbp != 0xfedcba9876543210ULL
            || c.rsi != 0xabcdefab12345678ULL || c.rdi != 0x8765432187654321ULL)
        throw std::runtime_error("Lookup virtual nonvolatile/stack corruption");
    return static_cast<std::uint32_t>(c.rax);
}
