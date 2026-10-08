#include <stdexcept>
#include "cpu64.h"
#include "crc.h"
std::int64_t g_image_delta;
extern "C" void automatic_set_base(unsigned char* base) {
    g_image_delta = reinterpret_cast<std::uintptr_t>(base) - 0x140000000ULL;
}
#include "candidate-generated.cpp"
void dispatch(CPU*, std::uint64_t) { throw std::runtime_error("Unexpected CRC dependency"); }
void dispatch_jmp(CPU*, std::uint64_t) { throw std::runtime_error("Unexpected CRC tail call"); }
static std::uint32_t invoke(void (*body)(CPU*), std::uint64_t a, std::uint64_t b, std::uint64_t d) {
    alignas(16) unsigned char stack[1024]{};
    CPU c{}; auto entry = reinterpret_cast<std::uintptr_t>(stack + 888);
    c.rsp=entry; c.rcx=a; c.rdx=b; c.r8=d;
    c.rbx=0x1234567812345678ULL; c.rdi=0xfedcba9876543210ULL;
    body(&c);
    if (c.rsp != entry+8 || c.rbx != 0x1234567812345678ULL || c.rdi != 0xfedcba9876543210ULL)
        throw std::runtime_error("CRC virtual stack/nonvolatile corruption");
    return static_cast<std::uint32_t>(c.rax);
}
extern "C" std::uint32_t automatic_crc_seeded(std::uint32_t seed, const void* p, std::uint32_t n) {
    return invoke(L_000140A9AE90,seed,reinterpret_cast<std::uintptr_t>(p),n);
}
extern "C" void automatic_crc_buffer(std::uint32_t* out, const void* p, std::uint32_t n) {
    invoke(L_000140CE2510,reinterpret_cast<std::uintptr_t>(out),reinterpret_cast<std::uintptr_t>(p),n);
}
extern "C" void automatic_crc_u32(std::uint32_t* out, std::uint32_t value) {
    invoke(L_000140CE2570,reinterpret_cast<std::uintptr_t>(out),value,0);
}
extern "C" void automatic_crc_u64(std::uint32_t* out, std::uint64_t value) {
    invoke(L_000140CE25F0,reinterpret_cast<std::uintptr_t>(out),value,0);
}
