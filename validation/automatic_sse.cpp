// Generic scalar-SSE semantic repairs for the pinned pcrecomp experiment.
// These functions are deliberately x64-specific, not a portability claim.
#include "cpu64.h"
#include <cstddef>
static_assert(sizeof(CPU) == 448 && offsetof(CPU, xmm) == 184);
#define BINARY(op) \
extern "C" float strict_##op(float a, float b) { \
    __asm__ __volatile__(#op " %1, %0" : "+x"(a) : "x"(b)); return a; \
}
BINARY(addss)
BINARY(subss)
BINARY(mulss)
BINARY(divss)
BINARY(minss)
BINARY(maxss)
#define COMPARE(op) \
extern "C" void strict_##op(CPU* c, float a, float b) { \
    unsigned char cf, zf, pf; \
    __asm__ __volatile__(#op " %4, %3; setb %0; sete %1; setp %2" \
        : "=qm"(cf), "=qm"(zf), "=qm"(pf) : "x"(a), "x"(b) : "cc"); \
    c->cf = cf; c->zf = zf; c->pf = pf; c->of = c->sf = c->af = 0; \
}
COMPARE(comiss)
COMPARE(ucomiss)
