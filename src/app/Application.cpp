/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of the interactive Application (see Application.h).
*/

#include "app/Application.h"

#include "bench/Timer.h"
#include "sim/BackendFactory.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace swarm
{

Application::Application(const AppConfig& config) : m_config(config)
{
    m_config.params.worldWidth = static_cast<float>(m_config.windowWidth);
    m_config.params.worldHeight = static_cast<float>(m_config.windowHeight);
    if (m_config.backend.empty())
    {
        m_config.backend = defaultBackend();
    }

    m_sim = std::make_unique<SimulationController>(m_config.params, m_config.spawn, m_config.backend);

    m_window.create(sf::VideoMode(m_config.windowWidth, m_config.windowHeight), "Swarm Simulator",
                    sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
    m_window.setVerticalSyncEnabled(m_config.vsync);
    m_window.setFramerateLimit(0);
    // The view always shows the whole world, whatever the window size.
    m_window.setView(sf::View(sf::FloatRect(0.0f, 0.0f, m_config.params.worldWidth, m_config.params.worldHeight)));

    if (!m_hud.loadFont())
    {
        std::cerr << "note: no font found (set SWARM_FONT=/path/to/font.ttf); HUD shown in window title\n";
    }
}

void Application::processEvents()
{
    sf::Event event;
    while (m_window.pollEvent(event))
    {
        m_input.handleEvent(event, m_window, *m_sim, m_renderer, m_view);
    }
}

void Application::renderFrame()
{
    FrameTimings& t = m_sim->timings();

    m_sim->fetchRenderData();
    t.fillMs = m_renderer.update(m_sim->renderData(), m_sim->params());

    if (m_view.showOverlay)
    {
        std::vector<float> field;
        int w = 0;
        int h = 0;
        if (m_sim->backend().downloadPheromone(field, w, h))
        {
            m_renderer.updatePheromone(field, w, h, m_sim->params());
        }
    }

    Stopwatch draw;
    m_window.clear(sf::Color(12, 14, 22));
    m_renderer.draw(m_window, m_sim->world(), m_view.showOverlay);
    if (m_view.showHud)
    {
        m_hud.draw(m_window, makeHudInfo());
    }
    m_window.display();
    t.drawMs = draw.elapsedMs();
    m_lastDrawMs = t.drawMs;
}

HudInfo Application::makeHudInfo() const
{
    const FrameTimings& t = m_sim->timings();
    HudInfo info;
    info.fps = m_fpsSmoothed;
    info.frameMs = m_frameMsSmoothed;
    info.stepMs = t.stepMs;
    info.stepsPerFrame = t.stepsThisFrame;
    info.copyMs = t.copyMs;
    info.fillMs = t.fillMs;
    info.drawMs = m_lastDrawMs;
    info.agents = m_sim->agentCount();
    info.backend = m_sim->backendName();
    info.device = m_sim->deviceName();
    info.threads = m_sim->threadCount();
    info.lastSwitchMs = m_sim->lastSwitchMs();
    info.paused = m_view.paused;
    info.shape = m_renderer.shapeName();
    info.food = m_sim->world().food.size();
    info.obstacles = m_sim->world().obstacles.size();
    return info;
}

int Application::run()
{
    std::cout << "backend: " << m_sim->backendName() << " (" << m_sim->deviceName() << "), agents: "
              << m_sim->agentCount() << '\n';

    const float dt = m_config.fixedDt;
    float accumulator = 0.0f;
    sf::Clock frameClock;
    Stopwatch titleTimer;

    while (m_window.isOpen() && !m_view.quit)
    {
        const float realDt = std::min(frameClock.restart().asSeconds(), 0.25f);
        m_sim->beginFrame();
        processEvents();
        if (m_view.quit)
        {
            break;
        }

        // Fixed-timestep simulation.
        if (m_view.paused)
        {
            accumulator = 0.0f;
        }
        else
        {
            accumulator += realDt;
            int steps = 0;
            while (accumulator >= dt && steps < m_config.maxStepsPerFrame)
            {
                m_sim->step(dt);
                accumulator -= dt;
                ++steps;
            }
            if (steps == m_config.maxStepsPerFrame)
            {
                accumulator = 0.0f;  // simulation is slower than real time: drop the backlog
            }
        }

        renderFrame();

        // Smoothed FPS for the HUD (exponential moving average).
        const double frameMs = static_cast<double>(realDt) * 1000.0;
        m_frameMsSmoothed = m_frameMsSmoothed == 0.0 ? frameMs : 0.9 * m_frameMsSmoothed + 0.1 * frameMs;
        m_fpsSmoothed = m_frameMsSmoothed > 0.0 ? 1000.0 / m_frameMsSmoothed : 0.0;

        if (titleTimer.elapsedMs() > 500.0)
        {
            m_window.setTitle("Swarm Simulator | " + Hud::summary(makeHudInfo()));
            titleTimer.restart();
        }
    }
    m_window.close();
    return 0;
}

}  // namespace swarm
