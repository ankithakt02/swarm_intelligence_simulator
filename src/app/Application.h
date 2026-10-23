/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Top-level interactive application: owns the SFML window, the simulation
controller, the renderer, the HUD and the input handler, and runs the main
loop with a fixed simulation timestep (rendering runs as fast as it can; the
simulation advances in fixed dt increments, at most maxStepsPerFrame per frame).
Also provides the FPS sweep used to find the largest agent count that renders
at 30 and 60 FPS (see FpsSweep.cpp).
*/

#pragma once

#include "app/Hud.h"
#include "app/InputHandler.h"
#include "app/Renderer.h"
#include "app/SimulationController.h"

#include <SFML/Graphics.hpp>

#include <memory>
#include <string>
#include <vector>

namespace swarm
{

struct AppConfig
{
    unsigned windowWidth = 1600;
    unsigned windowHeight = 900;
    SimParams params;
    SpawnConfig spawn;
    std::string backend;          ///< empty = defaultBackend()
    float fixedDt = 1.0f / 60.0f;
    int maxStepsPerFrame = 4;     ///< prevents a spiral of death when steps are slow
    bool vsync = false;

    // FPS sweep (non-interactive measurement mode)
    std::vector<std::size_t> sweepCounts;     ///< empty = normal interactive run
    std::vector<std::string> sweepBackends;   ///< empty = the starting backend only
    int sweepWarmupFrames = 30;
    int sweepFrames = 300;
    std::string sweepOut = "results/fps_sweep.csv";
};

class Application
{
public:
    explicit Application(const AppConfig& config);

    /// Interactive main loop. Returns the process exit code.
    int run();
    /// Measures frame time at each agent count (one step per frame, uncapped). Returns exit code.
    int runFpsSweep();

private:
    void processEvents();
    /// Copies render data, rebuilds vertices, draws and displays one frame (timed).
    void renderFrame();
    HudInfo makeHudInfo() const;

    AppConfig m_config;
    sf::RenderWindow m_window;
    std::unique_ptr<SimulationController> m_sim;
    Renderer m_renderer;
    Hud m_hud;
    InputHandler m_input;
    ViewState m_view;
    double m_fpsSmoothed = 0.0;
    double m_frameMsSmoothed = 0.0;
    double m_lastDrawMs = 0.0;  ///< draw time of the previous frame (the HUD is drawn before this frame's is known)
};

}  // namespace swarm
