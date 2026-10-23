/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of the placeholder backend (see StubBackend.h).
*/

#include "sim/StubBackend.h"

#include <cmath>

namespace swarm
{

namespace
{

/// Cheap integer hash -> float in [-1, 1]; deterministic per (agent, step).
float hashToSignedUnit(std::uint64_t a, std::uint64_t b)
{
    std::uint64_t h = a * 0x9E3779B97F4A7C15ULL ^ (b + 0x632BE59BD9B4E019ULL);
    h ^= h >> 33;
    h *= 0xFF51AFD7ED558CCDULL;
    h ^= h >> 33;
    return static_cast<float>(h & 0xFFFFFF) / 8388607.5f - 1.0f;
}

}  // namespace

void StubBackend::uploadState(const AgentPopulation& population)
{
    m_pop = population;
}

void StubBackend::downloadState(AgentPopulation& population)
{
    population = m_pop;
}

void StubBackend::downloadRenderData(RenderData& out)
{
    out.x = m_pop.x;
    out.y = m_pop.y;
    out.vx = m_pop.vx;
    out.vy = m_pop.vy;
    out.type = m_pop.type;
}

void StubBackend::step(float dt)
{
    const float w = m_params.worldWidth;
    const float h = m_params.worldHeight;
    const float maxWander = 1.5f * dt;  // radians per step

    for (std::size_t i = 0; i < m_pop.size(); ++i)
    {
        float vx = m_pop.vx[i];
        float vy = m_pop.vy[i];

        // Random wander: rotate the velocity by a small angle.
        const float turn = maxWander * hashToSignedUnit(i, m_stepIndex);
        const float c = std::cos(turn);
        const float s = std::sin(turn);
        const float rvx = c * vx - s * vy;
        const float rvy = s * vx + c * vy;
        vx = rvx;
        vy = rvy;

        // Push away from obstacles.
        for (const Obstacle& o : m_world.obstacles)
        {
            const float dx = m_pop.x[i] - o.x;
            const float dy = m_pop.y[i] - o.y;
            const float d2 = dx * dx + dy * dy;
            const float reach = o.radius + 20.0f;
            if (d2 < reach * reach && d2 > 1e-6f)
            {
                const float d = std::sqrt(d2);
                const float push = m_params.obstacleWeight * m_params.maxSpeed * (reach - d) / reach;
                vx += dx / d * push * dt * 10.0f;
                vy += dy / d * push * dt * 10.0f;
            }
        }

        // Clamp speed.
        const float speed = std::sqrt(vx * vx + vy * vy);
        if (speed > m_params.maxSpeed && speed > 0.0f)
        {
            vx *= m_params.maxSpeed / speed;
            vy *= m_params.maxSpeed / speed;
        }

        float px = m_pop.x[i] + vx * dt;
        float py = m_pop.y[i] + vy * dt;

        // Wrap around (torus world).
        if (px < 0.0f) px += w;
        if (px >= w) px -= w;
        if (py < 0.0f) py += h;
        if (py >= h) py -= h;

        m_pop.x[i] = px;
        m_pop.y[i] = py;
        m_pop.vx[i] = vx;
        m_pop.vy[i] = vy;
    }
    ++m_stepIndex;
}

}  // namespace swarm
