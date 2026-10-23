/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of the headless benchmark and validation (see BenchmarkRunner.h).
*/

#include "bench/BenchmarkRunner.h"

#include "bench/MetricsLogger.h"
#include "bench/Timer.h"
#include "sim/BackendFactory.h"
#include "sim/Metrics.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>

namespace swarm
{

SpawnConfig makeSpawn(std::size_t n, double antFraction, double predatorFraction, std::uint32_t seed)
{
    SpawnConfig spawn;
    spawn.seed = seed;
    spawn.ants = static_cast<std::size_t>(static_cast<double>(n) * antFraction);
    spawn.predators = static_cast<std::size_t>(static_cast<double>(n) * predatorFraction);
    spawn.boids = n - spawn.ants - spawn.predators;
    return spawn;
}

namespace
{

/// Prepares a backend with params, an empty world, and a freshly spawned population.
std::unique_ptr<ISimBackend> prepareBackend(const std::string& name, const SimParams& params,
                                            const AgentPopulation& initial)
{
    std::unique_ptr<ISimBackend> backend = makeBackend(name);
    if (!backend)
    {
        return nullptr;
    }
    backend->setParams(params);
    backend->setWorld(World{});
    backend->uploadState(initial);
    backend->synchronize();
    return backend;
}

void printAvailable()
{
    std::cerr << "available backends:";
    for (const std::string& n : availableBackends()) std::cerr << ' ' << n;
    std::cerr << '\n';
}

}  // namespace

int runBenchmark(const BenchConfig& config)
{
    MetricsLogger csv;
    const std::vector<std::string> header{
        "timestamp", "host", "backend", "device", "threads", "agents", "boids", "ants", "predators",
        "warmup_steps", "steps", "repeat",
        "step_ms_mean", "step_ms_std", "step_ms_min", "step_ms_p50", "step_ms_p95",
        "copy_ms_mean", "copy_ms_p95", "grid_ms_mean", "update_ms_mean",
        "steps_per_sec", "agent_updates_per_sec",
        "alignment", "cohesion", "nn_dist", "mean_speed"};
    if (!csv.open(config.outPath, header, config.append))
    {
        return 1;
    }

    std::cout << std::fixed << std::setprecision(3);
    int failures = 0;

    for (const std::string& backendName : config.backends)
    {
        if (!makeBackend(backendName))
        {
            std::cerr << "skipping unknown or not-compiled backend '" << backendName << "'\n";
            printAvailable();
            ++failures;
            continue;
        }

        for (std::size_t agents : config.agentCounts)
        {
            const SpawnConfig spawn = makeSpawn(agents, config.antFraction, config.predatorFraction, config.seed);
            const AgentPopulation initial = spawnPopulation(spawn, config.params);

            // Thread counts only matter for backends that use them.
            std::vector<unsigned> threads = config.threadCounts;
            {
                auto probe = makeBackend(backendName);
                if (!probe->supportsThreads() || threads.empty())
                {
                    threads = {0};
                }
            }

            for (unsigned t : threads)
            {
                for (int rep = 0; rep < config.repeats; ++rep)
                {
                    auto backend = prepareBackend(backendName, config.params, initial);
                    if (t > 0)
                    {
                        backend->setThreadCount(t);
                    }

                    RenderData render;
                    for (int s = 0; s < config.warmupSteps; ++s)
                    {
                        backend->step(config.dt);
                        if (config.measureCopy) backend->downloadRenderData(render);
                    }
                    backend->synchronize();

                    std::vector<double> stepMs;
                    std::vector<double> copyMs;
                    std::vector<double> gridMs;
                    std::vector<double> updateMs;
                    stepMs.reserve(static_cast<std::size_t>(config.measureSteps));

                    Stopwatch total;
                    for (int s = 0; s < config.measureSteps; ++s)
                    {
                        Stopwatch sw;
                        backend->step(config.dt);
                        backend->synchronize();
                        stepMs.push_back(sw.elapsedMs());

                        const StepTimings st = backend->lastTimings();
                        if (st.gridBuildMs >= 0.0) gridMs.push_back(st.gridBuildMs);
                        if (st.updateMs >= 0.0) updateMs.push_back(st.updateMs);

                        if (config.measureCopy)
                        {
                            sw.restart();
                            backend->downloadRenderData(render);
                            backend->synchronize();
                            copyMs.push_back(sw.elapsedMs());
                        }
                    }
                    const double totalMs = total.elapsedMs();

                    const SampleStats step = summarize(stepMs);
                    const SampleStats copy = summarize(copyMs);
                    const SampleStats grid = summarize(gridMs);
                    const SampleStats upd = summarize(updateMs);

                    SwarmMetrics metrics;
                    if (config.computeMetrics)
                    {
                        AgentPopulation finalState;
                        backend->downloadState(finalState);
                        metrics = computeMetrics(finalState, config.params);
                    }

                    const double stepsPerSec = step.mean > 0.0 ? 1000.0 / step.mean : 0.0;
                    const unsigned threadsUsed = backend->supportsThreads() ? backend->threadCount() : 0;

                    csv.add(timestampNow()).add(hostName()).add(backendName).add(backend->deviceName())
                        .add(threadsUsed).add(agents).add(spawn.boids).add(spawn.ants).add(spawn.predators)
                        .add(config.warmupSteps).add(config.measureSteps).add(rep)
                        .add(step.mean).add(step.stddev).add(step.min).add(step.p50).add(step.p95)
                        .add(config.measureCopy ? copy.mean : -1.0).add(config.measureCopy ? copy.p95 : -1.0)
                        .add(grid.count ? grid.mean : -1.0).add(upd.count ? upd.mean : -1.0)
                        .add(stepsPerSec, 2).add(stepsPerSec * static_cast<double>(agents), 0)
                        .add(metrics.alignment).add(metrics.cohesion).add(metrics.nearestNeighborDist)
                        .add(metrics.meanSpeed);
                    csv.endRow();

                    std::cout << backendName << "  threads=" << threadsUsed << "  agents=" << agents
                              << "  rep=" << rep << "  step=" << step.mean << " ms (p95 " << step.p95
                              << ")  copy=" << (config.measureCopy ? copy.mean : 0.0) << " ms  ["
                              << totalMs / 1000.0 << " s]\n";
                }
            }
        }
    }

    std::cout << "wrote " << config.outPath << '\n';
    return failures == 0 ? 0 : 2;
}

int runValidation(const ValidateConfig& config)
{
    MetricsLogger csv;
    const std::vector<std::string> header{
        "timestamp", "host", "reference", "candidate", "agents", "steps", "metric",
        "reference_value", "candidate_value", "abs_diff", "rel_diff", "tolerance", "pass"};
    if (!csv.open(config.outPath, header, config.append))
    {
        return 1;
    }

    if (!makeBackend(config.reference))
    {
        std::cerr << "reference backend '" << config.reference << "' is not available\n";
        printAvailable();
        return 1;
    }

    bool allPass = true;
    std::cout << std::fixed << std::setprecision(4);

    for (std::size_t agents : config.agentCounts)
    {
        const SpawnConfig spawn = makeSpawn(agents, 0.0, 0.0, config.seed);
        const AgentPopulation initial = spawnPopulation(spawn, config.params);

        const auto runOne = [&](const std::string& name) {
            auto backend = prepareBackend(name, config.params, initial);
            for (int s = 0; s < config.steps; ++s)
            {
                backend->step(config.dt);
            }
            backend->synchronize();
            AgentPopulation out;
            backend->downloadState(out);
            return computeMetrics(out, config.params);
        };

        const SwarmMetrics ref = runOne(config.reference);

        for (const std::string& cand : config.candidates)
        {
            if (!makeBackend(cand))
            {
                std::cerr << "skipping candidate '" << cand << "' (not available)\n";
                continue;
            }
            const SwarmMetrics got = runOne(cand);

            struct Row
            {
                const char* name;
                double r;
                double c;
                double absTol;
            };
            const Row rows[] = {
                {"alignment", ref.alignment, got.alignment, config.alignmentAbsTolerance},
                {"cohesion", ref.cohesion, got.cohesion, 0.0},
                {"nn_dist", ref.nearestNeighborDist, got.nearestNeighborDist, 0.0},
                {"mean_speed", ref.meanSpeed, got.meanSpeed, 0.0},
            };

            for (const Row& row : rows)
            {
                const double absDiff = std::fabs(row.r - row.c);
                const double rel = relativeDifference(row.r, row.c);
                const bool pass = rel <= config.relTolerance || absDiff <= row.absTol;
                allPass = allPass && pass;

                csv.add(timestampNow()).add(hostName()).add(config.reference).add(cand).add(agents)
                    .add(config.steps).add(row.name).add(row.r, 6).add(row.c, 6).add(absDiff, 6)
                    .add(rel, 6).add(config.relTolerance, 4).add(pass ? "PASS" : "FAIL");
                csv.endRow();

                std::cout << config.reference << " vs " << cand << "  agents=" << agents << "  "
                          << row.name << ": " << row.r << " vs " << row.c << "  rel=" << rel
                          << (pass ? "  PASS" : "  FAIL") << '\n';
            }
        }
    }

    std::cout << "wrote " << config.outPath << '\n';
    return allPass ? 0 : 3;
}

}  // namespace swarm
