/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Entry point for the interactive SFML swarm simulator.

Examples:
    swarm                                    # default backend, 20k boids
    swarm --backend cuda --agents 100k
    swarm --agents 50k --ants-frac 0.2 --pred-frac 0.001
    swarm --fps-sweep 10k,25k,50k,100k,200k --sweep-backends cpu,cuda
*/

#include "app/Application.h"
#include "bench/BenchmarkRunner.h"
#include "common/Args.h"
#include "sim/BackendFactory.h"

#include <exception>
#include <iostream>

namespace
{

void printUsage()
{
    std::cout <<
        "Usage: swarm [options]\n"
        "  --backend NAME       cpu | cuda | stub (default: cpu if built, else stub)\n"
        "  --agents N           total agents, 'k' suffix allowed (default 20k)\n"
        "  --ants-frac F        fraction of agents that are ants (default 0)\n"
        "  --pred-frac F        fraction of agents that are predators (default 0)\n"
        "  --threads N          CPU worker threads (default: all cores)\n"
        "  --size WxH           window and world size (default 1600x900)\n"
        "  --seed N             random seed (default 42)\n"
        "  --vsync              enable vertical sync\n"
        "\n"
        "FPS sweep (non-interactive; writes CSV and exits):\n"
        "  --fps-sweep LIST     agent counts, e.g. 10k,25k,50k,100k,200k\n"
        "  --sweep-backends L   backends to sweep (default: --backend)\n"
        "  --sweep-frames N     measured frames per count (default 300)\n"
        "  --out PATH           CSV (appended; default results/fps_sweep.csv)\n"
        "  --list-backends\n"
        "  --help\n";
}

}  // namespace

int main(int argc, char** argv)
{
    using namespace swarm;
    const Args args(argc, argv);
    const std::vector<std::string> known{
        "--backend", "--agents", "--ants-frac", "--pred-frac", "--threads", "--size", "--seed", "--vsync",
        "--fps-sweep", "--sweep-backends", "--sweep-frames", "--out", "--list-backends", "--help"};
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

    AppConfig cfg;
    cfg.backend = args.get("--backend", "");

    const std::string size = args.get("--size", "1600x900");
    const std::size_t xPos = size.find('x');
    if (xPos == std::string::npos)
    {
        std::cerr << "error: --size expects WxH, e.g. 1600x900\n";
        return 1;
    }
    cfg.windowWidth = static_cast<unsigned>(parseCount(size.substr(0, xPos), "width"));
    cfg.windowHeight = static_cast<unsigned>(parseCount(size.substr(xPos + 1), "height"));

    const double antFrac = parseReal(args.get("--ants-frac", "0"), "ant fraction");
    const double predFrac = parseReal(args.get("--pred-frac", "0"), "predator fraction");
    if (antFrac < 0.0 || predFrac < 0.0 || antFrac + predFrac > 1.0)
    {
        std::cerr << "error: --ants-frac + --pred-frac must be between 0 and 1\n";
        return 1;
    }
    const auto agents = static_cast<std::size_t>(parseCount(args.get("--agents", "20k"), "agent count"));
    const auto seed = static_cast<std::uint32_t>(parseCount(args.get("--seed", "42"), "seed"));
    cfg.spawn = makeSpawn(agents, antFrac, predFrac, seed);
    cfg.params.threadCount = static_cast<unsigned>(parseCount(args.get("--threads", "0"), "thread count"));
    cfg.vsync = args.has("--vsync");

    for (const std::string& c : splitList(args.get("--fps-sweep", "")))
    {
        cfg.sweepCounts.push_back(static_cast<std::size_t>(parseCount(c, "agent count")));
    }
    cfg.sweepBackends = splitList(args.get("--sweep-backends", ""));
    cfg.sweepFrames = static_cast<int>(parseCount(args.get("--sweep-frames", "300"), "frames"));
    cfg.sweepOut = args.get("--out", "results/fps_sweep.csv");

    try
    {
        Application app(cfg);
        return cfg.sweepCounts.empty() ? app.run() : app.runFpsSweep();
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
