/*
Author: Ankitha Kottamthody (owner), initial draft by Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of the swarm reference metrics (see Metrics.h).
*/

#include "sim/Metrics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace swarm
{

namespace
{

/// Shortest signed difference on a wrap-around axis of the given length.
float wrapDelta(float d, float length)
{
    if (d > 0.5f * length) return d - length;
    if (d < -0.5f * length) return d + length;
    return d;
}

}  // namespace

SwarmMetrics computeMetrics(const AgentPopulation& pop, const SimParams& params,
                            std::size_t maxSamples)
{
    SwarmMetrics m;
    if (pop.size() == 0)
    {
        return m;
    }

    // Use boids if there are any, otherwise every agent.
    std::vector<std::size_t> flock;
    flock.reserve(pop.size());
    for (std::size_t i = 0; i < pop.size(); ++i)
    {
        if (pop.type[i] == static_cast<std::uint8_t>(AgentType::Boid))
        {
            flock.push_back(i);
        }
    }
    if (flock.empty())
    {
        for (std::size_t i = 0; i < pop.size(); ++i)
        {
            flock.push_back(i);
        }
    }

    // Alignment (exact) and mean speed.
    double sumUx = 0.0;
    double sumUy = 0.0;
    for (std::size_t i : flock)
    {
        const double s = std::hypot(static_cast<double>(pop.vx[i]), static_cast<double>(pop.vy[i]));
        if (s > 1e-9)
        {
            sumUx += pop.vx[i] / s;
            sumUy += pop.vy[i] / s;
        }
    }
    m.alignment = std::hypot(sumUx, sumUy) / static_cast<double>(flock.size());

    double speedSum = 0.0;
    for (std::size_t i = 0; i < pop.size(); ++i)
    {
        speedSum += std::hypot(static_cast<double>(pop.vx[i]), static_cast<double>(pop.vy[i]));
    }
    m.meanSpeed = speedSum / static_cast<double>(pop.size());

    // Sampled cohesion and nearest-neighbour distance.
    const std::size_t samples = std::min(maxSamples, flock.size());
    const std::size_t stride = std::max<std::size_t>(1, flock.size() / std::max<std::size_t>(1, samples));
    const float r2 = params.perceptionRadius * params.perceptionRadius;
    double neighborSum = 0.0;
    double nnSum = 0.0;
    std::size_t used = 0;

    for (std::size_t s = 0; s < samples; ++s)
    {
        const std::size_t i = flock[s * stride];
        float best = std::numeric_limits<float>::max();
        std::size_t count = 0;
        for (std::size_t j : flock)
        {
            if (j == i) continue;
            const float dx = wrapDelta(pop.x[j] - pop.x[i], params.worldWidth);
            const float dy = wrapDelta(pop.y[j] - pop.y[i], params.worldHeight);
            const float d2 = dx * dx + dy * dy;
            if (d2 < best) best = d2;
            if (d2 < r2) ++count;
        }
        if (best < std::numeric_limits<float>::max())
        {
            nnSum += std::sqrt(static_cast<double>(best));
            neighborSum += static_cast<double>(count);
            ++used;
        }
    }

    if (used > 0)
    {
        m.cohesion = neighborSum / static_cast<double>(used);
        m.nearestNeighborDist = nnSum / static_cast<double>(used);
    }
    m.sampleCount = used;
    return m;
}

double relativeDifference(double a, double b)
{
    const double denom = std::max({std::fabs(a), std::fabs(b), 1e-12});
    return std::fabs(a - b) / denom;
}

}  // namespace swarm
