#ifndef THEME_HPP
#define THEME_HPP

#include "imgui.h"

/**
 * Theme management for the Circuit Simulator application.
 * Provides professional dark theme configuration for ImGui.
 */
class Theme {
 public:
  /**
   * Applies the professional dark theme to ImGui.
   * Sets up colors, styles, and visual properties.
   */
  static void setProfessionalTheme();

 private:
  /**
   * Private constructor - Theme is a utility class with static methods only.
   */
  Theme() = default;
};

#endif  // THEME_HPP
