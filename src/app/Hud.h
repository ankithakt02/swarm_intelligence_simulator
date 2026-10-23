/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
On-screen heads-up display: FPS, per-phase frame timings (step, GPU->host
copy, vertex fill, draw), agent count, active backend, and thread count.
If no font can be found, the same summary is shown in the window title.
*/

#pragma once

#include <SFML/Graphics.hpp>

#include <string>

namespace swarm
{

struct HudInfo
{
    double fps = 0.0;
    double frameMs = 0.0;
    double stepMs = 0.0;
    int stepsPerFrame = 0;
    double copyMs = 0.0;
    double fillMs = 0.0;
    double drawMs = 0.0;
    std::size_t agents = 0;
    std::string backend;
    std::string device;
    unsigned threads = 0;     ///< 0 = backend does not use threads
    double lastSwitchMs = 0.0;
    bool paused = false;
    std::string shape;
    std::size_t food = 0;
    std::size_t obstacles = 0;
};

class Hud
{
public:
    /// Tries SWARM_FONT, the project assets folder, then common system fonts.
    bool loadFont();
    bool hasFont() const { return m_hasFont; }

    void draw(sf::RenderTarget& target, const HudInfo& info);
    /// One-line summary (used for the window title and console output).
    static std::string summary(const HudInfo& info);

private:
    sf::Font m_font;
    sf::Text m_text;
    sf::RectangleShape m_background;
    bool m_hasFont = false;
};

}  // namespace swarm
