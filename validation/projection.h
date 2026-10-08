#pragma once
#include <cstdint>
using Projection = bool (*)(const float*, const float*, const float*, float*, float*, float*, float);
struct ProjectionArgs {
    const float *matrix, *port, *point;
    float *x, *y, *z;
    float tolerance;
    std::uint8_t result;
    std::uint8_t padding[3];
    std::uint32_t mxcsr;
};
extern "C" std::uint64_t projection_probe(Projection, ProjectionArgs*);
extern "C" bool reconstruct_projection(const float*, const float*, const float*,
                                        float*, float*, float*, float);
