/*
Author: Ankitha Kottamthody (owner), initial draft by Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Per-agent rule math shared by the CPU backend and the CUDA kernels, so both
backends compute exactly the same thing. Every function here must be plain
arithmetic on floats (no std::vector, no exceptions, no I/O) and marked SWARM_HD
so nvcc compiles it for both host and device.

TODO (Ankitha, M0): add separation / alignment / cohesion accumulation,
obstacle avoidance, food attraction, chase / flee, pheromone gradient following.
*/

#pragma once

#include <cmath>

#if defined(__CUDACC__)
#define SWARM_HD __host__ __device__ inline
#else
#define SWARM_HD inline
#endif

namespace swarm
{
namespace rules
{

/// Shortest signed difference on a wrap-around axis (the world is a torus).
SWARM_HD float wrapDelta(float d, float length)
{
    if (d > 0.5f * length) return d - length;
    if (d < -0.5f * length) return d + length;
    return d;
}

/// Wraps a coordinate back into [0, length).
SWARM_HD float wrapCoord(float p, float length)
{
    if (p < 0.0f) return p + length;
    if (p >= length) return p - length;
    return p;
}

/// Scales (x, y) down so its length is at most maxLen.
SWARM_HD void clampLength(float& x, float& y, float maxLen)
{
    const float l2 = x * x + y * y;
    if (l2 > maxLen * maxLen && l2 > 0.0f)
    {
        const float s = maxLen / sqrtf(l2);
        x *= s;
        y *= s;
    }
}

}  // namespace rules
}  // namespace swarm
