#pragma once
#include <SFML/Graphics.hpp>

/**
 * Terminal Type - defines the electrical role of a terminal
 */
enum class TerminalType {
  POSITIVE,  // Positive terminal (e.g., voltage source +, resistor terminal 1)
  NEGATIVE,  // Negative terminal (e.g., voltage source -, resistor terminal 2)
  IN,        // Input terminal
  OUT,       // Output terminal
  BIDIRECTIONAL  // Can be either input or output
};

/**
 * Terminal - represents a connection point on a GUI component
 *
 * Each terminal has:
 * - A visual position (where it appears on screen)
 * - An electrical type (POSITIVE, NEGATIVE, etc.)
 * - A node number (assigned during circuit analysis, -1 if unassigned)
 * - A parent component (which component owns this terminal)
 */
class Terminal {
 public:
  /**
   * Constructor
   * @param position The visual position of the terminal (relative to component)
   * @param type The electrical type of the terminal
   */
  Terminal(sf::Vector2f position, TerminalType type);

  // --- Getters ---
  sf::Vector2f getPosition() const { return _position; }
  TerminalType getType() const { return _type; }
  int getNodeNumber() const { return _nodeNumber; }
  bool isConnected() const { return _isConnected; }

  // --- Setters ---
  void setPosition(sf::Vector2f position) { _position = position; }
  void setNodeNumber(int nodeNumber) { _nodeNumber = nodeNumber; }
  void setConnected(bool connected) { _isConnected = connected; }

  /**
   * Check if a point is within the clickable area of this terminal
   * @param point The point to check (in world coordinates)
   * @return true if the point is inside the terminal's hit area
   */
  bool contains(sf::Vector2f point) const;

  /**
   * Draw the terminal on the window
   * @param window The SFML window to draw on
   */
  void draw(sf::RenderWindow& window) const;

  /**
   * Get the radius of the terminal (for hit detection and rendering)
   */
  static constexpr float getRadius() { return TERMINAL_RADIUS; }

 private:
  sf::Vector2f _position;  // Visual position (world coordinates)
  TerminalType _type;      // Electrical type
  int _nodeNumber;         // Circuit node number (-1 if unassigned)
  bool _isConnected;       // Whether a wire is connected to this terminal

  static constexpr float TERMINAL_RADIUS = 6.0f;  // Visual radius in pixels
};
