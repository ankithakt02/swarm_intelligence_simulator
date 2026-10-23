/*
Author: Ankitha Kottamthody (owner), initial draft by Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Swarm-level reference metrics, computed on the host from a downloaded
population. The same functions are applied to CPU and CUDA results, so the two
backends are compared on statistics rather than on individual trajectories
(floating-point summation order differs between backends, so exact positions
diverge after a few hundred steps even when both are correct).
*/

#pragma once

#include "sim/SimTypes.h"

#include <cstddef>
#include <string>

namespace swarm
{

struct SwarmMetrics
{
    /// Polarisation order parameter of the boids: |mean of unit velocity vectors|.
    /// 0 = random headings, 1 = everyone flying the same way.
    double alignment = 0.0;
    /// Mean number of other boids within the perception radius (local density).
    double cohesion = 0.0;
    /// Mean distance from a boid to its nearest boid neighbour.
    double nearestNeighborDist = 0.0;
    /// Mean speed of all agents.
    double meanSpeed = 0.0;
    /// Number of boids sampled for cohesion / nearest-neighbour (exact for alignment).
    std::size_t sampleCount = 0;
};

/// Computes the metrics. Cohesion and nearest-neighbour distance use up to
/// maxSamples boids (evenly spaced indices) against all boids, with wrap-around
/// distances. Cost is O(maxSamples * N).
SwarmMetrics computeMetrics(const AgentPopulation& pop, const SimParams& params,
                            std::size_t maxSamples = 512);

/// Relative difference |a - b| / max(|a|, |b|, eps).
double relativeDifference(double a, double b);

}  // namespace swarm
