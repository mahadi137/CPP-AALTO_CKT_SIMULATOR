#include <iostream>

#include "Application.hpp"

/**
 * Main entry point for the GUI application
 */
int main() {
  try {
    // Create the application with window size 1200x800
    Application app(1200, 800, "Circuit Simulator - Schematic Editor");

    // Start the application (enters the game loop)
    app.run();

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
