#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"

namespace daa {
namespace visualization {

class Theme {
 public:
  virtual ~Theme() = default;
  virtual void Apply() const = 0;
  virtual std::string GetName() const = 0;
};

class ComfortableDarkCyanTheme : public Theme {
 public:
  void Apply() const override;
  std::string GetName() const override { return "Comfortable Dark Cyan"; }
};

class DefaultTheme : public Theme {
 public:
  void Apply() const override;
  std::string GetName() const override { return "Default"; }
};

class FontManager {
 public:
  FontManager();
  ~FontManager() = default;

  bool Initialize();
  bool LoadFont(const std::string& name, const std::string& path, float size);
  void SetCurrentFont(const std::string& fontName);
  ImFont* GetFont(const std::string& fontName) const;
  bool IsInitialized() const { return initialized; }
  const std::string& GetCurrentFontName() const { return currentFontName; }

 private:
  std::unordered_map<std::string, ImFont*> fonts;
  std::string currentFontName;
  bool initialized;
};

class ImGuiThemeManager {
 public:
  static ImGuiThemeManager& GetInstance();

  // Delete copy and move constructors/assignments
  ImGuiThemeManager(const ImGuiThemeManager&) = delete;
  ImGuiThemeManager& operator=(const ImGuiThemeManager&) = delete;
  ImGuiThemeManager(ImGuiThemeManager&&) = delete;
  ImGuiThemeManager& operator=(ImGuiThemeManager&&) = delete;

  void RegisterTheme(std::unique_ptr<Theme> theme);
  bool ApplyTheme(const std::string& themeName);
  std::vector<std::string> GetAvailableThemes() const;
  std::string GetCurrentThemeName() const;

  // Font management
  void InitializeFonts();
  void SetFont(const std::string& fontName);
  void SetDefaultFont(float size = 16.0f);
  ImFont* GetFont(const std::string& fontName) const;

 private:
  // Private constructor for singleton
  ImGuiThemeManager();
  ~ImGuiThemeManager() = default;

  std::unordered_map<std::string, std::unique_ptr<Theme>> themes;
  std::string currentThemeName;
  FontManager fontManager;
};

// Global instance accessor function
inline ImGuiThemeManager& GetThemeManager() {
  return ImGuiThemeManager::GetInstance();
}

}  // namespace visualization
}  // namespace daa