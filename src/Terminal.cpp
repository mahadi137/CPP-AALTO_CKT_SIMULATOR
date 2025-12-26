#include "Terminal.hpp"

#include <cmath>

Terminal::Terminal(sf::Vector2f position, TerminalType type)
    : _position(position),
      _type(type),
      _nodeNumber(-1),  // -1 means unassigned
      _isConnected(false) {}

bool Terminal::contains(sf::Vector2f point) const {
  // Calculate distance from point to terminal center
  float dx = point.x - _position.x;
  float dy = point.y - _position.y;
  float distanceSquared = dx * dx + dy * dy;

  // Check if distance is within radius (use squared to avoid sqrt)
  return distanceSquared <= (TERMINAL_RADIUS * TERMINAL_RADIUS);
}

void Terminal::draw(sf::RenderWindow& window) const {
  // Create a circle shape for the terminal
  sf::CircleShape circle(TERMINAL_RADIUS);
  circle.setPosition(_position.x - TERMINAL_RADIUS,
                     _position.y - TERMINAL_RADIUS);

  // Color based on connection state
  if (_isConnected) {
    circle.setFillColor(sf::Color::Green);  // Green when connected
  } else {
    circle.setFillColor(sf::Color::White);  // White when unconnected
  }

  // Black outline
  circle.setOutlineColor(sf::Color::Black);
  circle.setOutlineThickness(1.5f);

  window.draw(circle);

  // Optional: Draw a small indicator for terminal type
  // For example, + or - symbol for POSITIVE/NEGATIVE
  // This can be added later with text rendering
}
