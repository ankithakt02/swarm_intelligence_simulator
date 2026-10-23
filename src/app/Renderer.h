/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Draws the swarm with a single sf::VertexArray (one draw call for all agents),
plus food, obstacles, and the optional pheromone overlay. Agents are drawn as
small triangles pointing along their velocity, or as single points (cheaper,
useful for very large counts). Colour encodes agent type and boid speed.
*/

#pragma once

#include "sim/SimTypes.h"

#include <SFML/Graphics.hpp>

#include <vector>

namespace swarm
{

class Renderer
{
public:
    enum class AgentShape
    {
        Triangle,
        Point
    };

    Renderer();

    /// Rebuilds the agent vertex array from the latest render data. Returns the time taken in ms.
    double update(const RenderData& data, const SimParams& params);
    /// Uploads a pheromone field (row-major, width x height) to the overlay texture.
    void updatePheromone(const std::vector<float>& field, int width, int height, const SimParams& params);
    void clearPheromone() { m_hasPheromone = false; }

    void draw(sf::RenderTarget& target, const World& world, bool showOverlay);

    void toggleShape();
    AgentShape shape() const { return m_shape; }
    const char* shapeName() const { return m_shape == AgentShape::Triangle ? "triangles" : "points"; }

private:
    void drawWorld(sf::RenderTarget& target, const World& world);

    AgentShape m_shape = AgentShape::Triangle;
    sf::VertexArray m_agents;
    sf::CircleShape m_circle;

    sf::Texture m_pheromoneTexture;
    sf::Sprite m_pheromoneSprite;
    std::vector<sf::Uint8> m_pheromonePixels;
    bool m_hasPheromone = false;
};

}  // namespace swarm
