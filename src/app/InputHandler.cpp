/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of the InputHandler (see InputHandler.h).
*/

#include "app/InputHandler.h"

#include "app/Renderer.h"
#include "app/SimulationController.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace swarm
{

namespace
{

float distance(sf::Vector2f a, sf::Vector2f b)
{
    return std::hypot(a.x - b.x, a.y - b.y);
}

constexpr std::size_t kMinAgents = 1000;
constexpr std::size_t kMaxAgents = 4000000;

}  // namespace

void InputHandler::handleEvent(const sf::Event& event, sf::RenderWindow& window, SimulationController& sim,
                               Renderer& renderer, ViewState& view)
{
    switch (event.type)
    {
    case sf::Event::Closed:
        view.quit = true;
        break;

    case sf::Event::KeyPressed:
        handleKey(event.key.code, sim, renderer, view);
        break;

    case sf::Event::MouseButtonPressed:
    {
        const sf::Vector2f pos = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        if (event.mouseButton.button == sf::Mouse::Left)
        {
            m_leftDown = true;
            m_dragging = false;
            m_pressPos = pos;
        }
        else if (event.mouseButton.button == sf::Mouse::Right)
        {
            sim.world().eraseNear(pos.x, pos.y, kEraseRadius);
        }
        break;
    }

    case sf::Event::MouseMoved:
    {
        if (!m_leftDown)
        {
            break;
        }
        const sf::Vector2f pos = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        if (!m_dragging && distance(pos, m_pressPos) > kDragThreshold)
        {
            m_dragging = true;
            sim.world().addObstacle(m_pressPos.x, m_pressPos.y, kObstacleRadius);
            m_lastObstacle = m_pressPos;
        }
        if (m_dragging)
        {
            placeObstaclesTo(sim, pos);
        }
        break;
    }

    case sf::Event::MouseButtonReleased:
        if (event.mouseButton.button == sf::Mouse::Left && m_leftDown)
        {
            if (!m_dragging)
            {
                sim.world().addFood(m_pressPos.x, m_pressPos.y);
            }
            m_leftDown = false;
            m_dragging = false;
        }
        break;

    default:
        break;
    }
}

void InputHandler::placeObstaclesTo(SimulationController& sim, sf::Vector2f target)
{
    // Place overlapping circles every radius along the drag path so the wall has no gaps.
    const float spacing = kObstacleRadius;
    float d = distance(target, m_lastObstacle);
    while (d >= spacing)
    {
        const sf::Vector2f dir((target.x - m_lastObstacle.x) / d, (target.y - m_lastObstacle.y) / d);
        m_lastObstacle += dir * spacing;
        sim.world().addObstacle(m_lastObstacle.x, m_lastObstacle.y, kObstacleRadius);
        d = distance(target, m_lastObstacle);
    }
}

void InputHandler::scaleAgents(SimulationController& sim, double factor)
{
    SpawnConfig spawn = sim.spawnConfig();
    const std::size_t total = spawn.total();
    const auto target = static_cast<std::size_t>(static_cast<double>(total) * factor);
    const std::size_t clamped = std::clamp(target, kMinAgents, kMaxAgents);
    if (clamped == total || total == 0)
    {
        return;
    }
    const double ratio = static_cast<double>(clamped) / static_cast<double>(total);
    spawn.ants = static_cast<std::size_t>(static_cast<double>(spawn.ants) * ratio);
    spawn.predators = static_cast<std::size_t>(static_cast<double>(spawn.predators) * ratio);
    spawn.boids = clamped - spawn.ants - spawn.predators;
    sim.reset(spawn);
    std::cout << "agents: " << sim.agentCount() << '\n';
}

void InputHandler::handleKey(sf::Keyboard::Key key, SimulationController& sim, Renderer& renderer,
                             ViewState& view)
{
    switch (key)
    {
    case sf::Keyboard::Escape:
        view.quit = true;
        break;
    case sf::Keyboard::Space:
        view.paused = !view.paused;
        break;
    case sf::Keyboard::R:
        sim.reset();
        break;
    case sf::Keyboard::B:
        sim.cycleBackend();
        break;
    case sf::Keyboard::P:
        renderer.toggleShape();
        break;
    case sf::Keyboard::O:
        view.showOverlay = !view.showOverlay;
        if (!view.showOverlay) renderer.clearPheromone();
        break;
    case sf::Keyboard::H:
        view.showHud = !view.showHud;
        break;
    case sf::Keyboard::C:
        sim.world().clear();
        break;
    case sf::Keyboard::Up:
        if (sim.supportsThreads()) sim.setThreadCount(sim.threadCount() + 1);
        break;
    case sf::Keyboard::Down:
        if (sim.supportsThreads() && sim.threadCount() > 1) sim.setThreadCount(sim.threadCount() - 1);
        break;
    case sf::Keyboard::Equal:
    case sf::Keyboard::Add:
    case sf::Keyboard::PageUp:
        scaleAgents(sim, 2.0);
        break;
    case sf::Keyboard::Hyphen:
    case sf::Keyboard::Subtract:
    case sf::Keyboard::PageDown:
        scaleAgents(sim, 0.5);
        break;
    default:
        break;
    }
}

}  // namespace swarm
