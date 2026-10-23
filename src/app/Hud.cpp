/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of the HUD (see Hud.h).
*/

#include "app/Hud.h"

#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <vector>

#ifndef SWARM_ASSET_DIR
#define SWARM_ASSET_DIR "assets"
#endif

namespace swarm
{

bool Hud::loadFont()
{
    std::vector<std::string> candidates;
    if (const char* env = std::getenv("SWARM_FONT"))
    {
        candidates.emplace_back(env);
    }
    candidates.emplace_back(std::string(SWARM_ASSET_DIR) + "/fonts/DejaVuSansMono.ttf");
    candidates.emplace_back("assets/fonts/DejaVuSansMono.ttf");
    candidates.emplace_back("../assets/fonts/DejaVuSansMono.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");
    candidates.emplace_back("/usr/share/fonts/dejavu/DejaVuSansMono.ttf");
    candidates.emplace_back("/usr/share/fonts/dejavu-sans-mono-fonts/DejaVuSansMono.ttf");
    candidates.emplace_back("C:/Windows/Fonts/consola.ttf");
    candidates.emplace_back("/System/Library/Fonts/Menlo.ttc");

    for (const std::string& path : candidates)
    {
        if (m_font.loadFromFile(path))
        {
            m_hasFont = true;
            m_text.setFont(m_font);
            m_text.setCharacterSize(14);
            m_text.setFillColor(sf::Color(230, 235, 240));
            m_background.setFillColor(sf::Color(10, 12, 18, 190));
            return true;
        }
    }
    return false;
}

std::string Hud::summary(const HudInfo& info)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << info.fps << " FPS | " << info.agents << " agents | "
       << info.backend;
    if (info.threads > 0) ss << " x" << info.threads << " threads";
    ss << std::setprecision(2) << " | step " << info.stepMs << " ms | copy " << info.copyMs << " ms";
    if (info.paused) ss << " | PAUSED";
    return ss.str();
}

void Hud::draw(sf::RenderTarget& target, const HudInfo& info)
{
    if (!m_hasFont)
    {
        return;
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "FPS        " << info.fps << "   (" << std::setprecision(2) << info.frameMs << " ms/frame)\n";
    ss << "step       " << info.stepMs << " ms  (" << info.stepsPerFrame << " step"
       << (info.stepsPerFrame == 1 ? "" : "s") << " this frame)\n";
    ss << "copy       " << info.copyMs << " ms  (backend -> host)\n";
    ss << "fill       " << info.fillMs << " ms  (vertex array)\n";
    ss << "draw       " << info.drawMs << " ms  (draw + display)\n";
    ss << "agents     " << info.agents << "   [" << info.shape << "]\n";
    ss << "backend    " << info.backend << "  (" << info.device << ")\n";
    ss << "threads    ";
    if (info.threads > 0) ss << info.threads; else ss << "n/a";
    ss << '\n';
    if (info.lastSwitchMs > 0.0)
    {
        ss << "switch     " << info.lastSwitchMs << " ms (last)\n";
    }
    ss << "world      " << info.food << " food, " << info.obstacles << " obstacles\n";
    if (info.paused)
    {
        ss << "PAUSED\n";
    }
    ss << "\nSpace pause  R reset  B backend  P shape  O overlay  H hud\n"
          "Up/Down threads  +/- agents  C clear world  Esc quit\n"
          "L-click food  L-drag obstacles  R-click erase";

    m_text.setString(ss.str());
    m_text.setPosition(14.0f, 12.0f);

    // Draw in screen space regardless of the world view.
    const sf::View previous = target.getView();
    target.setView(target.getDefaultView());
    const sf::FloatRect bounds = m_text.getGlobalBounds();
    m_background.setPosition(bounds.left - 8.0f, bounds.top - 6.0f);
    m_background.setSize(sf::Vector2f(bounds.width + 16.0f, bounds.height + 12.0f));
    target.draw(m_background);
    target.draw(m_text);
    target.setView(previous);
}

}  // namespace swarm
