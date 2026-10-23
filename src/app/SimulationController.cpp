/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of SimulationController (see SimulationController.h).
*/

#include "app/SimulationController.h"

#include "bench/Timer.h"
#include "sim/BackendFactory.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace swarm
{

SimulationController::SimulationController(const SimParams& params, const SpawnConfig& spawn,
                                           const std::string& backendName)
    : m_params(params), m_spawn(spawn)
{
    m_backend = createConfigured(backendName);
    if (!m_backend)
    {
        throw std::runtime_error("backend '" + backendName + "' is not available in this build");
    }
    reset();
}

std::unique_ptr<ISimBackend> SimulationController::createConfigured(const std::string& name)
{
    std::unique_ptr<ISimBackend> b = makeBackend(name);
    if (b)
    {
        b->setParams(m_params);
        b->setWorld(m_world);
        if (b->supportsThreads() && m_params.threadCount > 0)
        {
            b->setThreadCount(m_params.threadCount);
        }
    }
    return b;
}

void SimulationController::reset()
{
    m_backend->uploadState(spawnPopulation(m_spawn, m_params));
    m_backend->synchronize();
    fetchRenderData();
}

void SimulationController::reset(const SpawnConfig& spawn)
{
    m_spawn = spawn;
    reset();
}

bool SimulationController::switchBackend(const std::string& name)
{
    if (name == m_backend->name())
    {
        return true;
    }
    Stopwatch sw;
    std::unique_ptr<ISimBackend> next = createConfigured(name);
    if (!next)
    {
        std::cerr << "backend '" << name << "' is not available in this build\n";
        return false;
    }

    // State transfer: the swarm continues exactly where it was.
    AgentPopulation state;
    m_backend->synchronize();
    m_backend->downloadState(state);
    next->uploadState(state);
    next->synchronize();

    m_backend = std::move(next);
    m_uploadedWorldVersion = m_world.version;
    m_lastSwitchMs = sw.elapsedMs();
    std::cout << "switched to " << m_backend->name() << " (" << m_backend->deviceName() << ") in "
              << m_lastSwitchMs << " ms\n";
    return true;
}

void SimulationController::cycleBackend()
{
    std::vector<std::string> names = availableBackends();
    if (names.size() > 1)
    {
        names.erase(std::remove(names.begin(), names.end(), "stub"), names.end());
    }
    if (names.size() <= 1)
    {
        std::cout << "only one backend is compiled in (" << m_backend->name() << ")\n";
        return;
    }
    auto it = std::find(names.begin(), names.end(), m_backend->name());
    const std::size_t idx = it == names.end() ? 0 : static_cast<std::size_t>(it - names.begin()) + 1;
    switchBackend(names[idx % names.size()]);
}

void SimulationController::beginFrame()
{
    m_timings = FrameTimings{};
}

void SimulationController::step(float dt)
{
    if (m_world.version != m_uploadedWorldVersion)
    {
        applyWorld();
    }
    Stopwatch sw;
    m_backend->step(dt);
    m_backend->synchronize();
    m_timings.stepMs += sw.elapsedMs();
    ++m_timings.stepsThisFrame;
}

void SimulationController::fetchRenderData()
{
    Stopwatch sw;
    m_backend->downloadRenderData(m_render);
    m_backend->synchronize();
    m_timings.copyMs = sw.elapsedMs();
}

void SimulationController::applyParams()
{
    m_backend->setParams(m_params);
}

void SimulationController::applyWorld()
{
    m_backend->setWorld(m_world);
    m_uploadedWorldVersion = m_world.version;
}

void SimulationController::setThreadCount(unsigned count)
{
    m_params.threadCount = count;
    if (m_backend->supportsThreads())
    {
        m_backend->setThreadCount(count);
    }
}

unsigned SimulationController::threadCount() const
{
    return m_backend->supportsThreads() ? m_backend->threadCount() : 0;
}

bool SimulationController::supportsThreads() const
{
    return m_backend->supportsThreads();
}

std::string SimulationController::backendName() const
{
    return m_backend->name();
}

std::string SimulationController::deviceName() const
{
    return m_backend->deviceName();
}

std::size_t SimulationController::agentCount() const
{
    return m_backend->agentCount();
}

}  // namespace swarm
