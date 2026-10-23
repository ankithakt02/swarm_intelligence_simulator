/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of the Renderer (see Renderer.h).
*/

#include "app/Renderer.h"

#include "bench/Timer.h"

#include <algorithm>
#include <cmath>

namespace swarm
{

namespace
{

sf::Uint8 toByte(float v)
{
    return static_cast<sf::Uint8>(std::clamp(v, 0.0f, 255.0f));
}

/// Boids go from deep blue (slow) to cyan-white (fast); ants are amber; predators red.
sf::Color agentColor(std::uint8_t type, float speedFraction)
{
    switch (static_cast<AgentType>(type))
    {
    case AgentType::Ant:
        return sf::Color(240, 170, 60);
    case AgentType::Predator:
        return sf::Color(235, 60, 60);
    case AgentType::Boid:
    default:
    {
        const float t = std::clamp(speedFraction, 0.0f, 1.0f);
        return sf::Color(toByte(40.0f + 120.0f * t), toByte(100.0f + 140.0f * t), toByte(210.0f + 45.0f * t));
    }
    }
}

}  // namespace

Renderer::Renderer() : m_agents(sf::Triangles)
{
    m_circle.setPointCount(24);
}

void Renderer::toggleShape()
{
    m_shape = m_shape == AgentShape::Triangle ? AgentShape::Point : AgentShape::Triangle;
    m_agents.setPrimitiveType(m_shape == AgentShape::Triangle ? sf::Triangles : sf::Points);
}

double Renderer::update(const RenderData& data, const SimParams& params)
{
    Stopwatch sw;
    const std::size_t n = data.size();
    const float invMax = params.maxSpeed > 0.0f ? 1.0f / params.maxSpeed : 0.0f;

    if (m_shape == AgentShape::Point)
    {
        m_agents.resize(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            const float speed = std::sqrt(data.vx[i] * data.vx[i] + data.vy[i] * data.vy[i]);
            sf::Vertex& v = m_agents[i];
            v.position = sf::Vector2f(data.x[i], data.y[i]);
            v.color = agentColor(data.type[i], speed * invMax);
        }
        return sw.elapsedMs();
    }

    m_agents.resize(n * 3);
    for (std::size_t i = 0; i < n; ++i)
    {
        const float vx = data.vx[i];
        const float vy = data.vy[i];
        const float speed = std::sqrt(vx * vx + vy * vy);
        float dx = 1.0f;
        float dy = 0.0f;
        if (speed > 1e-4f)
        {
            dx = vx / speed;
            dy = vy / speed;
        }
        const bool predator = data.type[i] == static_cast<std::uint8_t>(AgentType::Predator);
        const float len = predator ? 7.0f : 3.5f;   // tip distance from centre
        const float half = predator ? 3.0f : 1.5f;  // half width of the base

        const sf::Color color = agentColor(data.type[i], speed * invMax);
        const float px = data.x[i];
        const float py = data.y[i];

        sf::Vertex* tri = &m_agents[i * 3];
        tri[0].position = sf::Vector2f(px + dx * len, py + dy * len);
        tri[1].position = sf::Vector2f(px - dx * len * 0.5f - dy * half, py - dy * len * 0.5f + dx * half);
        tri[2].position = sf::Vector2f(px - dx * len * 0.5f + dy * half, py - dy * len * 0.5f - dx * half);
        tri[0].color = color;
        tri[1].color = color;
        tri[2].color = color;
    }
    return sw.elapsedMs();
}

void Renderer::updatePheromone(const std::vector<float>& field, int width, int height, const SimParams& params)
{
    if (width <= 0 || height <= 0 || field.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
    {
        m_hasPheromone = false;
        return;
    }
    const sf::Vector2u size = m_pheromoneTexture.getSize();
    if (size.x != static_cast<unsigned>(width) || size.y != static_cast<unsigned>(height))
    {
        if (!m_pheromoneTexture.create(static_cast<unsigned>(width), static_cast<unsigned>(height)))
        {
            m_hasPheromone = false;
            return;
        }
        m_pheromoneTexture.setSmooth(true);
        m_pheromoneSprite.setTexture(m_pheromoneTexture, true);
    }

    // Normalise by the current maximum so the overlay stays visible as the field decays.
    float maxValue = 1e-6f;
    for (float v : field) maxValue = std::max(maxValue, v);

    m_pheromonePixels.resize(field.size() * 4);
    for (std::size_t i = 0; i < field.size(); ++i)
    {
        const float t = std::sqrt(std::clamp(field[i] / maxValue, 0.0f, 1.0f));
        m_pheromonePixels[i * 4 + 0] = 190;
        m_pheromonePixels[i * 4 + 1] = 80;
        m_pheromonePixels[i * 4 + 2] = 255;
        m_pheromonePixels[i * 4 + 3] = toByte(200.0f * t);
    }
    m_pheromoneTexture.update(m_pheromonePixels.data());
    m_pheromoneSprite.setScale(params.worldWidth / static_cast<float>(width),
                               params.worldHeight / static_cast<float>(height));
    m_hasPheromone = true;
}

void Renderer::drawWorld(sf::RenderTarget& target, const World& world)
{
    m_circle.setOutlineThickness(0.0f);
    m_circle.setFillColor(sf::Color(110, 115, 125));
    for (const Obstacle& o : world.obstacles)
    {
        m_circle.setRadius(o.radius);
        m_circle.setOrigin(o.radius, o.radius);
        m_circle.setPosition(o.x, o.y);
        target.draw(m_circle);
    }

    m_circle.setFillColor(sf::Color(80, 210, 110, 220));
    m_circle.setOutlineColor(sf::Color(170, 255, 190));
    m_circle.setOutlineThickness(1.5f);
    for (const FoodItem& f : world.food)
    {
        m_circle.setRadius(f.radius);
        m_circle.setOrigin(f.radius, f.radius);
        m_circle.setPosition(f.x, f.y);
        target.draw(m_circle);
    }
}

void Renderer::draw(sf::RenderTarget& target, const World& world, bool showOverlay)
{
    if (showOverlay && m_hasPheromone)
    {
        target.draw(m_pheromoneSprite);
    }
    drawWorld(target, world);
    target.draw(m_agents);
}

}  // namespace swarm
