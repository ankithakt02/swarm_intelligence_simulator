/*
Author: Ankitha Kottamthody (owner), initial draft by Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of the helper functions for the shared simulation types.
*/

#include "sim/SimTypes.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace swarm
{

void AgentPopulation::resize(std::size_t n)
{
    x.resize(n);
    y.resize(n);
    vx.resize(n);
    vy.resize(n);
    type.resize(n);
    carrying.resize(n);
}

void AgentPopulation::clear()
{
    resize(0);
}

bool AgentPopulation::isConsistent() const
{
    const std::size_t n = x.size();
    return y.size() == n && vx.size() == n && vy.size() == n && type.size() == n &&
           carrying.size() == n;
}

AgentPopulation spawnPopulation(const SpawnConfig& config, const SimParams& params)
{
    AgentPopulation pop;
    pop.resize(config.total());

    std::mt19937 rng(config.seed);
    std::uniform_real_distribution<float> px(0.0f, params.worldWidth);
    std::uniform_real_distribution<float> py(0.0f, params.worldHeight);
    std::uniform_real_distribution<float> angle(0.0f, 6.28318530718f);
    std::uniform_real_distribution<float> speed(params.minSpeed, params.maxSpeed);

    for (std::size_t i = 0; i < pop.size(); ++i)
    {
        AgentType kind = AgentType::Boid;
        if (i >= config.boids + config.ants)
        {
            kind = AgentType::Predator;
        }
        else if (i >= config.boids)
        {
            kind = AgentType::Ant;
        }

        const float a = angle(rng);
        const float s = speed(rng);
        pop.x[i] = px(rng);
        pop.y[i] = py(rng);
        pop.vx[i] = s * std::cos(a);
        pop.vy[i] = s * std::sin(a);
        pop.type[i] = static_cast<std::uint8_t>(kind);
        pop.carrying[i] = 0;
    }
    return pop;
}

void World::addFood(float px, float py)
{
    FoodItem item;
    item.x = px;
    item.y = py;
    food.push_back(item);
    ++version;
}

void World::addObstacle(float px, float py, float radius)
{
    Obstacle o;
    o.x = px;
    o.y = py;
    o.radius = radius;
    obstacles.push_back(o);
    ++version;
}

std::size_t World::eraseNear(float px, float py, float radius)
{
    const auto near = [px, py, radius](float qx, float qy, float r) {
        const float dx = qx - px;
        const float dy = qy - py;
        const float reach = radius + r;
        return dx * dx + dy * dy <= reach * reach;
    };

    const std::size_t before = food.size() + obstacles.size();
    food.erase(std::remove_if(food.begin(), food.end(),
                              [&](const FoodItem& f) { return near(f.x, f.y, f.radius); }),
               food.end());
    obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
                                   [&](const Obstacle& o) { return near(o.x, o.y, o.radius); }),
                    obstacles.end());
    const std::size_t removed = before - (food.size() + obstacles.size());
    if (removed > 0)
    {
        ++version;
    }
    return removed;
}

void World::clear()
{
    food.clear();
    obstacles.clear();
    ++version;
}

void RenderData::resize(std::size_t n)
{
    x.resize(n);
    y.resize(n);
    vx.resize(n);
    vy.resize(n);
    type.resize(n);
}

}  // namespace swarm
