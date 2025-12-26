#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

// Forward declarations
class SchematicEditor;
class CircuitController;
class ToolBar;
class ResultsPanel;
class PropertiesPanel;

/**
 * ComponentType - enum for different component types that can be selected
 */
enum class ComponentType {
  RESISTOR,
  CAPACITOR,
  INDUCTOR,
  VOLTAGE_SOURCE,
  CURRENT_SOURCE,
  WIRE,
  NONE
};

/**
 * Application - Main SFML application class
 * Manages the window, UI panels, and coordinates all application functionality
 */
class Application {
 public:
  Application(unsigned int width, unsigned int height,
              const std::string& title);
  ~Application();

  // Main application loop
  void run();

 private:
  // Core game loop functions
  void processEvents();
  void update(float deltaTime);
  void render();

  // Event handlers
  void handleMouseClick(sf::Mouse::Button button, sf::Vector2f position);
  void handleMouseMove(sf::Vector2f position);
  void handleKeyPress(sf::Keyboard::Key key);
  void handleMouseRelease(sf::Mouse::Button button, sf::Vector2f position);

  // Menu/Action handlers
  void onNew();
  void onOpen();
  void onSave();
  void onSaveAsCircuit();
  void onSaveAsCCircuit();
  void onSimulateDC();
  void onSimulateAC(double frequency);
  void onExportNetlist();
  void onExit();

  // UI setup
  void setupWindow();
  void createUI();

  // Save/Load functionality
  bool promptSaveChanges();

  // Member variables
  sf::RenderWindow _window;
  sf::Clock _clock;

  // UI Components (TODO: Uncomment when these classes are implemented)
  // std::unique_ptr<ToolBar> _toolbar;
  // std::unique_ptr<SchematicEditor> _schematicEditor;
  // std::unique_ptr<ResultsPanel> _resultsPanel;
  // std::unique_ptr<PropertiesPanel> _propertiesPanel;

  // Circuit controller (TODO: Uncomment when CircuitController is implemented)
  // std::unique_ptr<CircuitController> _controller;

  // Application state
  ComponentType _selectedTool;
  std::string _currentFilename;
  bool _isDirty;    // Has unsaved changes
  bool _isRunning;  // Application running state

  // Temporary welcome message (remove when UI is implemented)
  void drawWelcomeMessage();
};
