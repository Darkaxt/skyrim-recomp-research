#pragma once
#include <cstdint>
using GetValue = float (*)(void*, int);
extern void*** dispatch_table_slot;
extern "C" float reconstruct_dispatch(void*, int);
