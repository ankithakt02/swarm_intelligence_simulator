/*
Author: Ankitha Kottamthody (owner), initial draft by Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Shared simulation types used by every part of the project: the structure-of-arrays
agent storage, the slider-controlled parameters, the editable world (food and
obstacles), the minimal per-frame render data, and the ISimBackend interface that
both the CPU (std::thread) and CUDA backends implement.

DRAFT: the three of us must agree on this file before the backends are written.
Change it in one small pull request so everyone rebases onto the same interface.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace swarm
{

/// Kind of agent. Stored as one byte per agent in AgentPopulation::type.
enum class AgentType : std::uint8_t
{
    Boid = 0,      ///< flocking prey
    Ant = 1,       ///< forager following the pheromone field
    Predator = 2   ///< hunts boids
};

/// Structure-of-Arrays agent state. Every vector has exactly size() elements.
/// SoA keeps each field contiguous, which gives coalesced reads on the GPU and
/// cache-friendly loops on the CPU.
struct AgentPopulation
{
    std::vector<float> x;                ///< position x (world units = pixels)
    std::vector<float> y;                ///< position y
    std::vector<float> vx;               ///< velocity x (world units per second)
    std::vector<float> vy;               ///< velocity y
    std::vector<std::uint8_t> type;      ///< AgentType as uint8_t
    std::vector<std::uint8_t> carrying;  ///< ants only: 1 if carrying food

    std::size_t size() const { return x.size(); }
    void resize(std::size_t n);
    void clear();
    /// True if every field has the same length.
    bool isConsistent() const;
};

/// Every value the UI sliders can change. Backends receive a copy through
/// ISimBackend::setParams() whenever the user edits a value.
struct SimParams
{
    // World size (the world wraps around at the edges, i.e. a torus).
    float worldWidth = 1600.0f;
    float worldHeight = 900.0f;

    // Boid flocking
    float perceptionRadius = 25.0f;   ///< neighbours closer than this influence alignment/cohesion
    float separationRadius = 8.0f;    ///< neighbours closer than this push apart
    float separationWeight = 1.6f;
    float alignmentWeight = 1.0f;
    float cohesionWeight = 0.9f;
    float maxSpeed = 140.0f;          ///< world units per second
    float minSpeed = 40.0f;
    float maxForce = 400.0f;          ///< steering acceleration limit

    // World interaction
    float obstacleWeight = 4.0f;
    float foodAttractionWeight = 1.2f;
    float foodSenseRadius = 120.0f;

    // Predators
    float predatorSpeedFactor = 1.25f;  ///< predator max speed = maxSpeed * factor
    float predatorPerception = 90.0f;
    float fleeWeight = 3.0f;
    float chaseWeight = 2.0f;

    // Ant foraging / pheromone field
    float pheromoneCellSize = 8.0f;     ///< field resolution in world units
    float pheromoneDeposit = 1.0f;      ///< amount per ant per second
    float pheromoneDecay = 0.5f;        ///< fraction lost per second (0..1)
    float pheromoneDiffusion = 0.2f;    ///< fraction spread to neighbours per second
    float pheromoneFollowWeight = 1.5f;

    // CPU backend only
    unsigned threadCount = 0;           ///< 0 = std::thread::hardware_concurrency()
};

/// How many agents of each type to create.
struct SpawnConfig
{
    std::size_t boids = 20000;
    std::size_t ants = 0;
    std::size_t predators = 0;
    std::uint32_t seed = 42;

    std::size_t total() const { return boids + ants + predators; }
};

/// Fills a population with random positions and headings. Deterministic for a
/// given seed, so the CPU and CUDA backends can start from identical state.
AgentPopulation spawnPopulation(const SpawnConfig& config, const SimParams& params);

struct FoodItem
{
    float x = 0.0f;
    float y = 0.0f;
    float amount = 100.0f;
    float radius = 10.0f;
};

struct Obstacle
{
    float x = 0.0f;
    float y = 0.0f;
    float radius = 12.0f;
};

/// User-editable world. Obstacles are circles so the kernels stay simple.
struct World
{
    std::vector<FoodItem> food;
    std::vector<Obstacle> obstacles;
    std::uint64_t version = 0;  ///< bumped on every edit; backends re-upload only when it changes

    void addFood(float px, float py);
    void addObstacle(float px, float py, float radius);
    /// Removes food and obstacles within radius of (px, py). Returns the number removed.
    std::size_t eraseNear(float px, float py, float radius);
    void clear();
};

/// The minimum the renderer needs each frame. Kept separate from the full state
/// so the GPU -> host copy per frame is as small as possible (velocity is
/// included because agents are drawn as triangles pointing along their heading).
struct RenderData
{
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> vx;
    std::vector<float> vy;
    std::vector<std::uint8_t> type;

    std::size_t size() const { return x.size(); }
    void resize(std::size_t n);
};

/// Optional per-phase timings a backend can report for the last step().
/// Use a negative value for phases a backend does not measure.
struct StepTimings
{
    double gridBuildMs = -1.0;   ///< spatial grid build (CPU) / cell ids + sort + cell ranges (GPU)
    double updateMs = -1.0;      ///< behaviour update
    double pheromoneMs = -1.0;   ///< pheromone deposit / diffusion / decay
};

/// Common interface for the CPU and CUDA backends. The application and the
/// benchmark only ever talk to a backend through this interface.
class ISimBackend
{
public:
    virtual ~ISimBackend() = default;

    /// Short identifier used on the command line and in CSV files: "cpu", "cuda", "stub".
    virtual std::string name() const = 0;
    /// Human-readable device, e.g. "24-core Xeon Gold 6226" or "Tesla V100-PCIE-16GB".
    virtual std::string deviceName() const { return "host"; }

    /// Replace the full agent state (host -> backend). Used at reset and when switching backends.
    virtual void uploadState(const AgentPopulation& population) = 0;
    /// Copy the full agent state back (backend -> host). Used when switching backends.
    virtual void downloadState(AgentPopulation& population) = 0;
    /// Copy only what the renderer needs. Called once per rendered frame; must be cheap.
    virtual void downloadRenderData(RenderData& out) = 0;

    virtual void setParams(const SimParams& params) = 0;
    virtual void setWorld(const World& world) = 0;

    /// Advance the simulation by dt seconds. May return before GPU work finishes;
    /// call synchronize() before reading timings on the host.
    virtual void step(float dt) = 0;
    /// Block until all queued work is finished (no-op on the CPU).
    virtual void synchronize() {}

    virtual std::size_t agentCount() const = 0;

    /// Threading (only the CPU backend supports it).
    virtual bool supportsThreads() const { return false; }
    virtual void setThreadCount(unsigned /*count*/) {}
    virtual unsigned threadCount() const { return 1; }

    virtual StepTimings lastTimings() const { return {}; }

    /// Pheromone field for the overlay (row-major, width * height cells). Returns
    /// false if this backend has no field yet.
    virtual bool downloadPheromone(std::vector<float>& /*field*/, int& /*width*/, int& /*height*/)
    {
        return false;
    }
};

}  // namespace swarm
