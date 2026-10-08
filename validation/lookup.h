#pragma once
#include <cstdint>
extern "C" void automatic_set_base(unsigned char*);
extern "C" std::uint32_t automatic_lookup(const char*);
using StringCompare = int (*)(const char*, const char*);
extern StringCompare lookup_compare;
