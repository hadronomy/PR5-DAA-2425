#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include "imgui.h"

class ImGuiThemeManager {
 public:
  enum class ThemeType { ComfortableDarkCyan, Default };

  ImGuiThemeManager();
  ~ImGuiThemeManager() = default;

  // Apply a specific theme by enum type
  void ApplyTheme(ThemeType theme);

  // Apply a theme by name
  bool ApplyThemeByName(const std::string& themeName);

  // Get list of available theme names
  std::vector<std::string> GetAvailableThemes() const;

  // Get the current theme type
  ThemeType GetCurrentTheme() const { return currentTheme; }

  // Get the current theme name
  std::string GetCurrentThemeName() const;

 private:
  // Theme functions
  static void ApplyComfortableDarkCyanTheme();
  static void ApplyDefaultTheme();

  // Map theme types to their implementation functions
  using ThemeFunc = std::function<void()>;
  std::unordered_map<ThemeType, ThemeFunc> themeMap;

  // Map theme names to their enum types for lookup
  std::unordered_map<std::string, ThemeType> themeNameMap;

  // Current theme
  ThemeType currentTheme;
};

// Global instance for easier access
extern ImGuiThemeManager g_ImGuiThemeManager;
