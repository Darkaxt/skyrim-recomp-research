#pragma once
#include <cstdint>
using Frustum = bool (*)(const float*, const void*);
struct FrustumContext {
    unsigned char* base;
    int* (*get_epoch)();
    void (*constructor_override)(float*);
    template<class T> T& at(std::uintptr_t rva) { return *reinterpret_cast<T*>(base + rva); }
    void helper(std::uintptr_t rva, int* guard) {
        reinterpret_cast<void (*)(int*)>(base + rva)(guard);
    }
};
extern FrustumContext frustum_context;
extern "C" bool reconstruct_frustum(const float*, const void*);
