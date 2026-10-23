/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Headless benchmark and validation. The sweep runs every combination of
backend x agent count x thread count, times each step (and, separately, the
per-frame render copy), and writes one CSV row per combination. The validation
mode runs two backends from the same initial state and compares the reference
metrics against a tolerance.
*/

#pragma once

#include "sim/SimTypes.h"

#include <cstddef>
#include <string>
#include <vector>

namespace swarm
{

struct BenchConfig
{
    std::vector<std::string> backends;
    std::vector<std::size_t> agentCounts{10000, 50000, 100000};
    std::vector<unsigned> threadCounts{1, 2, 4, 8};
    int warmupSteps = 20;
    int measureSteps = 200;
    int repeats = 1;               ///< repeat each configuration (rows are written per repeat)
    float dt = 1.0f / 60.0f;
    std::uint32_t seed = 42;
    double antFraction = 0.0;      ///< tier 1 benchmarks flocking only
    double predatorFraction = 0.0;
    bool measureCopy = true;       ///< also time downloadRenderData() after every step
    bool computeMetrics = true;    ///< reference metrics after the measured steps
    std::string outPath = "results/bench.csv";
    bool append = false;
    SimParams params;
};

struct ValidateConfig
{
    std::string reference = "cpu";            ///< backend treated as ground truth
    std::vector<std::string> candidates{"cuda"};
    std::vector<std::size_t> agentCounts{10000, 50000};
    int steps = 300;
    float dt = 1.0f / 60.0f;
    std::uint32_t seed = 42;
    double relTolerance = 0.05;               ///< 5 % relative difference allowed per metric
    double alignmentAbsTolerance = 0.02;      ///< alignment can be near 0, so also allow an absolute gap
    std::string outPath = "results/validation.csv";
    bool append = false;
    SimParams params;
};

/// Runs the sweep. Returns 0 on success.
int runBenchmark(const BenchConfig& config);

/// Runs the validation. Returns 0 if every candidate is within tolerance.
int runValidation(const ValidateConfig& config);

/// Splits n agents into boids / ants / predators by fraction.
SpawnConfig makeSpawn(std::size_t n, double antFraction, double predatorFraction, std::uint32_t seed);

}  // namespace swarm
