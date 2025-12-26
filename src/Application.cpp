#include "Application.hpp"

#include <iostream>

Application::Application(unsigned int width, unsigned int height,
                         const std::string& title)
    : _window(sf::VideoMode(width, height), title),
      _selectedTool(ComponentType::NONE),
      _currentFilename(""),
      _isDirty(false),
      _isRunning(true) {
  setupWindow();
  createUI();
}

Application::~Application() = default;

void Application::setupWindow() {
  _window.setFramerateLimit(60);
  // Additional window setup can go here
}

void Application::createUI() {
  // TODO: Initialize UI components once they're implemented
  // _toolbar = std::make_unique<ToolBar>();
  // _controller = std::make_unique<CircuitController>();
  // _schematicEditor = std::make_unique<SchematicEditor>(_controller.get());
  // _resultsPanel = std::make_unique<ResultsPanel>();
  // _propertiesPanel = std::make_unique<PropertiesPanel>();
}

void Application::run() {
  while (_window.isOpen() && _isRunning) {
    float deltaTime = _clock.restart().asSeconds();

    processEvents();
    update(deltaTime);
    render();
  }
}

void Application::processEvents() {
  sf::Event event;
  while (_window.pollEvent(event)) {
    if (event.type == sf::Event::Closed) {
      if (promptSaveChanges()) {
        _window.close();
      }
    }

    if (event.type == sf::Event::MouseButtonPressed) {
      sf::Vector2f mousePos =
          _window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
      handleMouseClick(event.mouseButton.button, mousePos);
    }

    if (event.type == sf::Event::MouseMoved) {
      sf::Vector2f mousePos =
          _window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
      handleMouseMove(mousePos);
    }

    if (event.type == sf::Event::MouseButtonReleased) {
      sf::Vector2f mousePos =
          _window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
      handleMouseRelease(event.mouseButton.button, mousePos);
    }

    if (event.type == sf::Event::KeyPressed) {
      handleKeyPress(event.key.code);
    }

    // TODO: Forward events to UI components
    // _toolbar->handleEvent(event);
    // _schematicEditor->handleEvent(event, _window);
  }
}

void Application::update(float deltaTime) {
  // TODO: Update UI components
  // _schematicEditor->update(deltaTime);
  // _resultsPanel->update(deltaTime);
}

void Application::render() {
  _window.clear(sf::Color(240, 240, 240));  // Light gray background

  // TODO: Draw UI components in order (back to front)
  // _schematicEditor->draw(_window);
  // _toolbar->draw(_window);
  // _resultsPanel->draw(_window);
  // _propertiesPanel->draw(_window);

  // Temporary: Show welcome message until UI is implemented
  drawWelcomeMessage();

  _window.display();
}

// Event Handlers
void Application::handleMouseClick(sf::Mouse::Button button,
                                   sf::Vector2f position) {
  // TODO: Implement mouse click handling
  std::cout << "Mouse clicked at (" << position.x << ", " << position.y << ")"
            << std::endl;
}

void Application::handleMouseMove(sf::Vector2f position) {
  // TODO: Implement mouse move handling (e.g., dragging components)
}

void Application::handleKeyPress(sf::Keyboard::Key key) {
  // Keyboard shortcuts
  if (key == sf::Keyboard::S &&
      sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
    onSave();
  } else if (key == sf::Keyboard::N &&
             sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
    onNew();
  } else if (key == sf::Keyboard::O &&
             sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
    onOpen();
  } else if (key == sf::Keyboard::F5) {
    onSimulateDC();
  }
}

void Application::handleMouseRelease(sf::Mouse::Button button,
                                     sf::Vector2f position) {
  // TODO: Implement mouse release handling (e.g., end dragging)
}

// Menu/Action Handlers
void Application::onNew() {
  if (promptSaveChanges()) {
    // TODO: Clear the schematic editor
    _currentFilename = "";
    _isDirty = false;
    std::cout << "New circuit created" << std::endl;
  }
}

void Application::onOpen() {
  if (promptSaveChanges()) {
    // TODO: Open file dialog and load circuit
    std::cout << "Open circuit dialog (not yet implemented)" << std::endl;
  }
}

void Application::onSave() {
  if (_currentFilename.empty()) {
    onSaveAsCircuit();
  } else {
    // TODO: Save to current file
    _isDirty = false;
    std::cout << "Saved to " << _currentFilename << std::endl;
  }
}

void Application::onSaveAsCircuit() {
  // TODO: Open save dialog
  std::cout << "Save As dialog (not yet implemented)" << std::endl;
}

void Application::onSaveAsCCircuit() {
  // TODO: Export as C-style circuit file
  std::cout << "Save As C Circuit (not yet implemented)" << std::endl;
}

void Application::onSimulateDC() {
  // TODO: Run DC simulation
  std::cout << "Running DC simulation..." << std::endl;
  // _controller->simulate();
}

void Application::onSimulateAC(double frequency) {
  // TODO: Run AC simulation at specified frequency
  std::cout << "Running AC simulation at " << frequency << " Hz..."
            << std::endl;
}

void Application::onExportNetlist() {
  // TODO: Export SPICE netlist
  std::cout << "Export netlist (not yet implemented)" << std::endl;
}

void Application::onExit() {
  if (promptSaveChanges()) {
    _isRunning = false;
    _window.close();
  }
}

bool Application::promptSaveChanges() {
  if (_isDirty) {
    // TODO: Show dialog asking to save changes
    std::cout << "You have unsaved changes. Save? (not yet implemented)"
              << std::endl;
    // For now, just allow closing
    return true;
  }
  return true;
}

void Application::drawWelcomeMessage() {
  // Temporary function to show that the application is running
  sf::RectangleShape rect(sf::Vector2f(500, 200));
  rect.setPosition(350, 300);
  rect.setFillColor(sf::Color::White);
  rect.setOutlineColor(sf::Color::Black);
  rect.setOutlineThickness(2);
  _window.draw(rect);

  // Try to load a system font and display text
  sf::Font font;
  if (font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
    sf::Text title;
    title.setFont(font);
    title.setString("Circuit Simulator");
    title.setCharacterSize(32);
    title.setFillColor(sf::Color::Black);
    title.setPosition(420, 330);
    _window.draw(title);

    sf::Text subtitle;
    subtitle.setFont(font);
    subtitle.setString("Welcome, Application Framework Ready");
    subtitle.setCharacterSize(16);
    subtitle.setFillColor(sf::Color(100, 100, 100));
    subtitle.setPosition(420, 380);
    _window.draw(subtitle);

    sf::Text instructions;
    instructions.setFont(font);
    instructions.setString("Press Ctrl+N for New, F5 to Simulate");
    instructions.setCharacterSize(14);
    instructions.setFillColor(sf::Color(150, 150, 150));
    instructions.setPosition(380, 430);
    _window.draw(instructions);
  }
}
