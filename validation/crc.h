#pragma once
#include <cstdint>
extern "C" void automatic_set_base(unsigned char*);
extern "C" std::uint32_t automatic_crc_seeded(std::uint32_t, const void*, std::uint32_t);
extern "C" void automatic_crc_buffer(std::uint32_t*, const void*, std::uint32_t);
extern "C" void automatic_crc_u32(std::uint32_t*, std::uint32_t);
extern "C" void automatic_crc_u64(std::uint32_t*, std::uint64_t);
