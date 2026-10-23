/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Headless benchmark / validation entry point (no window, no SFML), meant for
PACE-ICE batch jobs.

Examples:
    swarm_bench --backends cpu --agents 50k --threads 1,2,4,8,16,24
    swarm_bench --backends cpu,cuda --agents 10k,50k,100k,250k --threads 24
    swarm_bench --validate --reference cpu --candidates cuda --agents 10k,50k
*/

#include "bench/BenchmarkRunner.h"
#include "common/Args.h"
#include "sim/BackendFactory.h"

#include <algorithm>
#include <iostream>
#include <thread>

namespace
{

void printUsage()
{
    std::cout <<
        "Usage: swarm_bench [options]\n"
        "\n"
        "Sweep mode (default): time every backend x agents x threads combination.\n"
        "  --backends LIST     comma list (default: every compiled backend except stub)\n"
        "  --agents LIST       agent counts, 'k' suffix allowed (default 10k,50k,100k)\n"
        "  --threads LIST      CPU thread counts (default 1,2,4,... up to all cores)\n"
        "  --steps N           measured steps per configuration (default 200)\n"
        "  --warmup N          untimed steps first (default 20)\n"
        "  --repeats N         repeat each configuration N times (default 1)\n"
        "  --ants-frac F       fraction of agents that are ants (default 0)\n"
        "  --pred-frac F       fraction of agents that are predators (default 0)\n"
        "  --no-copy           do not time the per-frame render copy\n"
        "  --no-metrics        skip the reference metrics (faster for huge counts)\n"
        "  --out PATH          CSV output (default results/bench.csv)\n"
        "  --append            append to the CSV instead of overwriting it\n"
        "\n"
        "Validation mode: compare backends on the reference metrics.\n"
        "  --validate\n"
        "  --reference NAME    ground-truth backend (default cpu)\n"
        "  --candidates LIST   backends to check (default cuda)\n"
        "  --tolerance F       allowed relative difference (default 0.05)\n"
        "  --steps N           steps before comparing (default 300)\n"
        "  --out PATH          CSV output (default results/validation.csv)\n"
        "\n"
        "Common:\n"
        "  --seed N            random seed for the initial state (default 42)\n"
        "  --world WxH         world size in units (default 1600x900)\n"
        "  --list-backends     print compiled backends and exit\n"
        "  --help\n";
}

std::vector<unsigned> defaultThreadCounts()
{
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    std::vector<unsigned> t;
    for (unsigned n = 1; n < hw; n *= 2)
    {
        t.push_back(n);
    }
    t.push_back(hw);
    return t;
}

}  // namespace

int main(int argc, char** argv)
{
    using namespace swarm;
    const Args args(argc, argv);

    const std::vector<std::string> known{
        "--backends", "--agents", "--threads", "--steps", "--warmup", "--repeats", "--ants-frac",
        "--pred-frac", "--no-copy", "--no-metrics", "--out", "--append", "--validate", "--reference",
        "--candidates", "--tolerance", "--seed", "--world", "--list-backends", "--help"};
    const std::vector<std::string> bad = args.unknown(known);
    if (!bad.empty())
    {
        std::cerr << "error: unknown option " << bad.front() << " (see --help)\n";
        return 1;
    }
    if (args.has("--help"))
    {
        printUsage();
        return 0;
    }
    if (args.has("--list-backends"))
    {
        for (const std::string& n : availableBackends()) std::cout << n << '\n';
        return 0;
    }

    SimParams params;
    const std::string world = args.get("--world", "");
    if (!world.empty())
    {
        const std::size_t xPos = world.find('x');
        if (xPos == std::string::npos)
        {
            std::cerr << "error: --world expects WxH, e.g. 1600x900\n";
            return 1;
        }
        params.worldWidth = static_cast<float>(parseReal(world.substr(0, xPos), "world width"));
        params.worldHeight = static_cast<float>(parseReal(world.substr(xPos + 1), "world height"));
    }
    const auto seed = static_cast<std::uint32_t>(parseCount(args.get("--seed", "42"), "seed"));

    std::vector<std::size_t> agents;
    const std::string agentText = args.get("--agents", args.has("--validate") ? "10k,50k" : "10k,50k,100k");
    for (const std::string& a : splitList(agentText))
    {
        agents.push_back(static_cast<std::size_t>(parseCount(a, "agent count")));
    }

    if (args.has("--validate"))
    {
        ValidateConfig cfg;
        cfg.params = params;
        cfg.seed = seed;
        cfg.agentCounts = agents;
        cfg.reference = args.get("--reference", "cpu");
        cfg.candidates = splitList(args.get("--candidates", "cuda"));
        cfg.relTolerance = parseReal(args.get("--tolerance", "0.05"), "tolerance");
        cfg.steps = static_cast<int>(parseCount(args.get("--steps", "300"), "steps"));
        cfg.outPath = args.get("--out", "results/validation.csv");
        cfg.append = args.has("--append");
        return runValidation(cfg);
    }

    BenchConfig cfg;
    cfg.params = params;
    cfg.seed = seed;
    cfg.agentCounts = agents;

    if (args.has("--backends"))
    {
        cfg.backends = splitList(args.get("--backends", ""));
    }
    else
    {
        for (const std::string& n : availableBackends())
        {
            if (n != "stub") cfg.backends.push_back(n);
        }
        if (cfg.backends.empty())
        {
            std::cout << "note: only the placeholder 'stub' backend is compiled in; benchmarking it\n";
            cfg.backends.push_back("stub");
        }
    }

    if (args.has("--threads"))
    {
        cfg.threadCounts.clear();
        for (const std::string& t : splitList(args.get("--threads", "")))
        {
            cfg.threadCounts.push_back(static_cast<unsigned>(parseCount(t, "thread count")));
        }
    }
    else
    {
        cfg.threadCounts = defaultThreadCounts();
    }

    cfg.measureSteps = static_cast<int>(parseCount(args.get("--steps", "200"), "steps"));
    cfg.warmupSteps = static_cast<int>(parseCount(args.get("--warmup", "20"), "warmup"));
    cfg.repeats = static_cast<int>(parseCount(args.get("--repeats", "1"), "repeats"));
    cfg.antFraction = parseReal(args.get("--ants-frac", "0"), "ant fraction");
    cfg.predatorFraction = parseReal(args.get("--pred-frac", "0"), "predator fraction");
    cfg.measureCopy = !args.has("--no-copy");
    cfg.computeMetrics = !args.has("--no-metrics");
    cfg.outPath = args.get("--out", "results/bench.csv");
    cfg.append = args.has("--append");

    if (cfg.antFraction < 0.0 || cfg.predatorFraction < 0.0 || cfg.antFraction + cfg.predatorFraction > 1.0)
    {
        std::cerr << "error: --ants-frac + --pred-frac must be between 0 and 1\n";
        return 1;
    }
    return runBenchmark(cfg);
}
