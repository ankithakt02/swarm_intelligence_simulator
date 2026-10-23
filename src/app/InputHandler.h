/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Translates SFML mouse and keyboard events into simulation actions:
left click drops food, left click-and-drag draws a line of circular obstacles,
right click erases nearby food/obstacles, and keys pause, reset, switch the
backend, change thread / agent counts, and toggle display options.
*/

#pragma once

#include <SFML/Graphics.hpp>

namespace swarm
{

class SimulationController;
class Renderer;

/// Display state owned by the application and changed by input.
struct ViewState
{
    bool paused = false;
    bool showHud = true;
    bool showOverlay = false;
    bool quit = false;
};

class InputHandler
{
public:
    void handleEvent(const sf::Event& event, sf::RenderWindow& window, SimulationController& sim,
                     Renderer& renderer, ViewState& view);

private:
    void handleKey(sf::Keyboard::Key key, SimulationController& sim, Renderer& renderer, ViewState& view);
    void scaleAgents(SimulationController& sim, double factor);
    void placeObstaclesTo(SimulationController& sim, sf::Vector2f target);

    bool m_leftDown = false;
    bool m_dragging = false;
    sf::Vector2f m_pressPos;
    sf::Vector2f m_lastObstacle;

    static constexpr float kDragThreshold = 6.0f;   ///< pixels before a click becomes a drag
    static constexpr float kObstacleRadius = 10.0f;
    static constexpr float kEraseRadius = 15.0f;
};

}  // namespace swarm
