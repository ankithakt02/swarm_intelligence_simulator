/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Placeholder backend so the application, HUD, and benchmark can be built and
tested before the real CPU and CUDA backends exist. Agents move with a slow
random wander, avoid obstacles, and wrap around the world edges. It does NOT
implement flocking and is not part of the measured results.
*/

#pragma once

#include "sim/SimTypes.h"

#include <cstdint>

namespace swarm
{

class StubBackend : public ISimBackend
{
public:
    std::string name() const override { return "stub"; }
    std::string deviceName() const override { return "host (single thread, placeholder)"; }

    void uploadState(const AgentPopulation& population) override;
    void downloadState(AgentPopulation& population) override;
    void downloadRenderData(RenderData& out) override;

    void setParams(const SimParams& params) override { m_params = params; }
    void setWorld(const World& world) override { m_world = world; }

    void step(float dt) override;
    std::size_t agentCount() const override { return m_pop.size(); }

private:
    AgentPopulation m_pop;
    SimParams m_params;
    World m_world;
    std::uint64_t m_stepIndex = 0;
};

}  // namespace swarm
