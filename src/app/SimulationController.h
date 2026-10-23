/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Owns the active backend, the simulation parameters, and the world. It is the
only place that steps the simulation in the interactive app, it performs the
runtime CPU <-> GPU switch (download full state from the old backend, upload
it to the new one), and it records the per-phase timings shown in the HUD.
It has no SFML dependency so it can also be used headless.
*/

#pragma once

#include "sim/SimTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace swarm
{

/// Timings of the most recent frame, in milliseconds.
struct FrameTimings
{
    double stepMs = 0.0;      ///< all simulation steps run this frame (summed)
    int stepsThisFrame = 0;
    double copyMs = 0.0;      ///< downloadRenderData()
    double fillMs = 0.0;      ///< building the vertex array (set by the renderer)
    double drawMs = 0.0;      ///< draw + display (set by the application)
    double frameMs = 0.0;     ///< whole frame
};

class SimulationController
{
public:
    SimulationController(const SimParams& params, const SpawnConfig& spawn, const std::string& backendName);

    /// Respawns agents with the current spawn configuration (world is kept).
    void reset();
    void reset(const SpawnConfig& spawn);

    /// Switches backend, transferring the full agent state. Returns false and
    /// keeps the old backend if the name is unknown or not compiled in.
    bool switchBackend(const std::string& name);
    /// Cycles through the compiled backends (stub is skipped when a real one exists).
    void cycleBackend();

    /// Advances one fixed step of dt seconds; the time is added to timings().stepMs.
    void step(float dt);
    /// Copies render data from the backend; the time is stored in timings().copyMs.
    void fetchRenderData();

    const RenderData& renderData() const { return m_render; }

    /// Edit params()/world() then call apply*() so the backend sees the change.
    SimParams& params() { return m_params; }
    void applyParams();
    World& world() { return m_world; }
    void applyWorld();

    void setThreadCount(unsigned count);
    unsigned threadCount() const;
    bool supportsThreads() const;

    const SpawnConfig& spawnConfig() const { return m_spawn; }
    std::string backendName() const;
    std::string deviceName() const;
    std::size_t agentCount() const;
    /// Time the last backend switch took (download + create + upload), ms.
    double lastSwitchMs() const { return m_lastSwitchMs; }
    ISimBackend& backend() { return *m_backend; }

    FrameTimings& timings() { return m_timings; }
    const FrameTimings& timings() const { return m_timings; }
    /// Clears the per-frame timings; call at the start of every frame.
    void beginFrame();

private:
    std::unique_ptr<ISimBackend> createConfigured(const std::string& name);

    SimParams m_params;
    SpawnConfig m_spawn;
    World m_world;
    std::unique_ptr<ISimBackend> m_backend;
    RenderData m_render;
    FrameTimings m_timings;
    double m_lastSwitchMs = 0.0;
    std::uint64_t m_uploadedWorldVersion = 0;
};

}  // namespace swarm
