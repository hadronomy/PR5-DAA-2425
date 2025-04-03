#include "visualization/imgui_theme.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include "utils.h"

namespace daa {
namespace visualization {

// Theme implementations
void ComfortableDarkCyanTheme::Apply() const {
  // Comfortable Dark Cyan style by SouthCraftX from ImThemes
  ImGuiStyle& style = ImGui::GetStyle();

  style.Alpha = 1.0f;
  style.DisabledAlpha = 1.0f;

  // Consistent window padding for all windows and popups
  style.WindowPadding = ImVec2(16.0f, 16.0f);
  style.WindowRounding = 11.5f;
  style.WindowBorderSize = 0.0f;
  style.WindowMinSize = ImVec2(20.0f, 20.0f);
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
  style.WindowMenuButtonPosition = ImGuiDir_None;

  // Child windows padding
  style.ChildRounding = 12.0f;
  style.ChildBorderSize = 1.0f;

  // Popup padding and appearance
  style.PopupRounding = 10.0f;
  style.PopupBorderSize = 1.0f;

  // Frame padding for better control layout
  style.FramePadding = ImVec2(12.0f, 6.0f);  // More balanced padding for controls
  style.FrameRounding = 8.0f;
  style.FrameBorderSize = 0.0f;

  // Item spacing for better readability
  style.ItemSpacing = ImVec2(10.0f, 8.0f);  // Horizontal, Vertical
  style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
  style.CellPadding = ImVec2(10.0f, 8.0f);

  style.IndentSpacing = 20.0f;  // Increase indent spacing for better hierarchy visualization
  style.ColumnsMinSpacing = 8.699999809265137f;
  style.ScrollbarSize = 11.60000038146973f;
  style.ScrollbarRounding = 15.89999961853027f;
  style.GrabMinSize = 5.0f;  // Larger grab size for easier manipulation
  style.GrabRounding = 8.0f;
  style.TabRounding = 8.0f;
  style.TabBorderSize = 0.0f;
  style.ColorButtonPosition = ImGuiDir_Right;
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

  // Define a more refined color palette with darker hover/active states
  ImVec4 lightGray = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);   // Slightly dimmer
  ImVec4 mediumGray = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);  // Slightly darker
  ImVec4 darkGray = ImVec4(0.12f, 0.14f, 0.15f, 1.0f);    // Base dark color

  // New colors for hover and active states
  ImVec4 hoverColor = ImVec4(0.16f, 0.18f, 0.22f, 1.0f);    // Slightly lighter than base
  ImVec4 activeColor = ImVec4(0.20f, 0.22f, 0.27f, 1.0f);   // Even lighter for active state
  ImVec4 veryDarkGray = ImVec4(0.08f, 0.09f, 0.10f, 1.0f);  // Very dark for backgrounds

  // Colors remain the same except blue/cyan accents which we replace with gray tones
  style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  style.Colors[ImGuiCol_TextDisabled] =
    ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
  style.Colors[ImGuiCol_WindowBg] = veryDarkGray;
  style.Colors[ImGuiCol_ChildBg] =
    ImVec4(0.10f, 0.11f, 0.12f, 1.0f);  // Slightly lighter than window bg
  style.Colors[ImGuiCol_PopupBg] = veryDarkGray;
  style.Colors[ImGuiCol_Border] =
    ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
  style.Colors[ImGuiCol_BorderShadow] =
    ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
  style.Colors[ImGuiCol_FrameBg] =
    ImVec4(0.1137254908680916f, 0.125490203499794f, 0.1529411822557449f, 1.0f);
  style.Colors[ImGuiCol_FrameBgHovered] = hoverColor;
  style.Colors[ImGuiCol_FrameBgActive] = activeColor;
  style.Colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.06f, 0.07f, 1.0f);  // Very dark title bar
  style.Colors[ImGuiCol_TitleBgActive] = darkGray;  // Slightly lighter when active
  style.Colors[ImGuiCol_TitleBgCollapsed] = veryDarkGray;
  style.Colors[ImGuiCol_MenuBarBg] =
    ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarBg] =
    ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrab] = darkGray;
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = hoverColor;
  style.Colors[ImGuiCol_ScrollbarGrabActive] = activeColor;
  style.Colors[ImGuiCol_CheckMark] = lightGray;
  style.Colors[ImGuiCol_SliderGrab] = mediumGray;
  style.Colors[ImGuiCol_SliderGrabActive] = lightGray;
  style.Colors[ImGuiCol_Button] = darkGray;
  style.Colors[ImGuiCol_ButtonHovered] = hoverColor;
  style.Colors[ImGuiCol_ButtonActive] = activeColor;
  style.Colors[ImGuiCol_Header] = darkGray;
  style.Colors[ImGuiCol_HeaderHovered] = hoverColor;
  style.Colors[ImGuiCol_HeaderActive] = activeColor;
  style.Colors[ImGuiCol_Separator] =
    ImVec4(0.1294117718935013f, 0.1490196138620377f, 0.1921568661928177f, 1.0f);
  style.Colors[ImGuiCol_SeparatorHovered] = hoverColor;
  style.Colors[ImGuiCol_SeparatorActive] = activeColor;
  style.Colors[ImGuiCol_ResizeGrip] =
    ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
  style.Colors[ImGuiCol_ResizeGripHovered] = hoverColor;
  style.Colors[ImGuiCol_ResizeGripActive] = activeColor;
  style.Colors[ImGuiCol_Tab] =
    ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
  style.Colors[ImGuiCol_TabHovered] = hoverColor;
  style.Colors[ImGuiCol_TabActive] = activeColor;
  style.Colors[ImGuiCol_TabUnfocused] =
    ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
  style.Colors[ImGuiCol_TabUnfocusedActive] = darkGray;
  style.Colors[ImGuiCol_PlotLines] = mediumGray;
  style.Colors[ImGuiCol_PlotLinesHovered] = activeColor;  // More consistent hover color
  style.Colors[ImGuiCol_PlotHistogram] = mediumGray;
  style.Colors[ImGuiCol_PlotHistogramHovered] = activeColor;  // More consistent hover color
  style.Colors[ImGuiCol_TableHeaderBg] =
    ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
  style.Colors[ImGuiCol_TableBorderStrong] =
    ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
  style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  style.Colors[ImGuiCol_TableRowBg] = darkGray;
  style.Colors[ImGuiCol_TableRowBgAlt] =
    ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2f, 0.3f, 0.4f, 0.5f);  // Darker selection color
  style.Colors[ImGuiCol_DragDropTarget] = lightGray;
  style.Colors[ImGuiCol_NavHighlight] = mediumGray;
  style.Colors[ImGuiCol_NavWindowingHighlight] = lightGray;
  style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.2f, 0.2f, 0.501960813999176f);
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.2f, 0.2f, 0.501960813999176f);
}

void DefaultTheme::Apply() const {
  // Apply classic style first
  ImGui::StyleColorsClassic();

  // Then override with better padding settings
  ImGuiStyle& style = ImGui::GetStyle();

  // Apply consistent padding while keeping default colors
  style.WindowPadding = ImVec2(14.0f, 14.0f);
  style.FramePadding = ImVec2(10.0f, 5.0f);
  style.ItemSpacing = ImVec2(8.0f, 8.0f);
  style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
  style.CellPadding = ImVec2(8.0f, 6.0f);

  // Rounded corners for a modern look
  style.WindowRounding = 6.0f;
  style.FrameRounding = 5.0f;
  style.PopupRounding = 6.0f;
  style.ScrollbarRounding = 6.0f;
  style.GrabRounding = 5.0f;
  style.TabRounding = 5.0f;

  // Reasonable sizes
  style.GrabMinSize = 5.0f;
  style.ScrollbarSize = 12.0f;
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
}

// FontManager implementation
FontManager::FontManager() : currentFontName("Default"), initialized(false) {}

bool FontManager::Initialize() {
  if (initialized)
    return true;

  ImGuiIO& io = ImGui::GetIO();

  // Store default font
  fonts["Default"] = io.Fonts->AddFontDefault();

  // Load additional fonts
  // Note: Paths are relative to the application directory
  ImFontConfig config;
  config.MergeMode = false;

  bool success = true;

  // Get the executable path to make font paths absolute
  std::filesystem::path exePath = daa::GetExecutablePath();
  std::filesystem::path basePath = exePath.parent_path();

  // Load a regular sans-serif font
  std::filesystem::path geistPath = basePath / "assets/fonts/Geist-Regular.ttf";
  ImFont* geistFont = io.Fonts->AddFontFromFileTTF(geistPath.string().c_str(), 16.0f, &config);
  fonts["Geist"] = geistFont ? geistFont : fonts["Default"];
  success = success && (geistFont != nullptr);

  if (!geistFont) {
    std::cerr << "Failed to load font: " << geistPath.string() << std::endl;
  }

  // Load a monospace font for code with Unicode icon support
  std::filesystem::path geistMonoPath = basePath / "assets/fonts/GeistMono-Regular.otf";

  // Create a glyph ranges builder to include Unicode icon ranges
  ImFontGlyphRangesBuilder builder;

  // Add default ranges
  builder.AddRanges(io.Fonts->GetGlyphRangesDefault());

  // Add Unicode icon ranges
  // Material Design Icons range (U+E000 - U+E999)
  for (ImWchar c = 0xE000; c <= 0xE999; c++)
    builder.AddChar(c);

  // Nerd Font icons range (U+E700 - U+F000)
  for (ImWchar c = 0xE700; c <= 0xF000; c++)
    builder.AddChar(c);

  // Font Awesome range
  for (ImWchar c = 0xF000; c <= 0xF8FF; c++)
    builder.AddChar(c);

  // Build the ranges
  ImVector<ImWchar> ranges;
  builder.BuildRanges(&ranges);

  // Configure font with icon ranges
  ImFontConfig iconConfig;
  iconConfig.MergeMode = true;          // Enable merge mode to avoid font switching
  iconConfig.GlyphMinAdvanceX = 13.0f;  // Set minimum width for consistent spacing
  iconConfig.PixelSnapH = true;

  // Load the font with the icon ranges
  ImFont* geistMonoFont =
    io.Fonts->AddFontFromFileTTF(geistMonoPath.string().c_str(), 16.0f, &iconConfig, ranges.Data);

  fonts["Geist Mono"] = geistMonoFont ? geistMonoFont : fonts["Default"];
  success = success && (geistMonoFont != nullptr);

  if (!geistMonoFont) {
    std::cerr << "Failed to load font: " << geistMonoPath.string() << std::endl;
  }

  // Build font atlas
  io.Fonts->Build();
  initialized = true;

  // Also load Geist Mono with icons
  std::filesystem::path geistMonoIconsPath = basePath / "assets/fonts/GeistMono-Regular.otf";

  // Create a separate font entry with icons
  if (geistMonoFont) {
    // We already have the font loaded, now load it again with icon support
    bool iconFontLoaded = LoadFontWithIcons("Geist Mono Icons", geistMonoIconsPath.string(), 16.0f);
    if (!iconFontLoaded) {
      std::cerr << "Failed to load font with icons: " << geistMonoIconsPath.string() << std::endl;
    }
  }

  // Set default font
  SetCurrentFont("Geist");

  return success;
}

bool FontManager::LoadFont(const std::string& name, const std::string& path, float size) {
  if (!initialized) {
    if (!Initialize())
      return false;
  }

  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig config;
  config.MergeMode = false;

  ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), size, &config);
  if (font == nullptr) {
    std::cerr << "Failed to load font: " << path << std::endl;
    return false;
  }

  fonts[name] = font;
  io.Fonts->Build();

  return true;
}

bool FontManager::LoadFontWithIcons(const std::string& name, const std::string& path, float size) {
  if (!initialized) {
    if (!Initialize())
      return false;
  }

  ImGuiIO& io = ImGui::GetIO();

  // Create a glyph ranges builder to include Unicode icon ranges
  ImFontGlyphRangesBuilder builder;

  // Add default ranges
  builder.AddRanges(io.Fonts->GetGlyphRangesDefault());

  // Add Unicode icon ranges
  // Material Design Icons range (U+E000 - U+E999)
  for (ImWchar c = 0xE000; c <= 0xE999; c++)
    builder.AddChar(c);

  // Nerd Font icons range (U+E700 - U+F000)
  for (ImWchar c = 0xE700; c <= 0xF000; c++)
    builder.AddChar(c);

  // Font Awesome range
  for (ImWchar c = 0xF000; c <= 0xF8FF; c++)
    builder.AddChar(c);

  // Build the ranges
  ImVector<ImWchar> ranges;
  builder.BuildRanges(&ranges);

  // Configure font with icon ranges
  ImFontConfig iconConfig;
  iconConfig.MergeMode = true;          // Enable merge mode to avoid font switching
  iconConfig.GlyphMinAdvanceX = 13.0f;  // Set minimum width for consistent spacing
  iconConfig.PixelSnapH = true;

  ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), size, &iconConfig, ranges.Data);
  if (font == nullptr) {
    std::cerr << "Failed to load font with icons: " << path << std::endl;
    return false;
  }

  fonts[name] = font;
  io.Fonts->Build();

  return true;
}

void FontManager::SetCurrentFont(const std::string& fontName) {
  if (!initialized) {
    if (!Initialize())
      return;
  }

  auto it = fonts.find(fontName);
  if (it != fonts.end() && it->second != nullptr) {
    ImGuiIO& io = ImGui::GetIO();
    io.FontDefault = it->second;
    currentFontName = fontName;
  } else {
    std::cerr << "Font not found: " << fontName << std::endl;
  }
}

ImFont* FontManager::GetFont(const std::string& fontName) const {
  auto it = fonts.find(fontName);
  if (it != fonts.end()) {
    return it->second;
  }

  auto defaultIt = fonts.find("Default");
  if (defaultIt != fonts.end()) {
    return defaultIt->second;
  }

  return nullptr;
}

// ImGuiThemeManager implementation
ImGuiThemeManager& ImGuiThemeManager::GetInstance() {
  static ImGuiThemeManager instance;
  return instance;
}

ImGuiThemeManager::ImGuiThemeManager() : currentThemeName("Default") {
  // Register default themes
  RegisterTheme(std::make_unique<ComfortableDarkCyanTheme>());
  RegisterTheme(std::make_unique<DefaultTheme>());
}

void ImGuiThemeManager::RegisterTheme(std::unique_ptr<Theme> theme) {
  if (theme) {
    std::string name = theme->GetName();
    themes[name] = std::move(theme);
  }
}

bool ImGuiThemeManager::ApplyTheme(const std::string& themeName) {
  auto it = themes.find(themeName);
  if (it != themes.end()) {
    // Make sure ImGui context exists before trying to apply theme
    if (ImGui::GetCurrentContext() != nullptr) {
      it->second->Apply();
      currentThemeName = themeName;

      // Ensure the current font is applied
      if (fontManager.IsInitialized()) {
        SetFont(fontManager.GetCurrentFontName());
      }
      return true;
    }
  }
  return false;
}

std::vector<std::string> ImGuiThemeManager::GetAvailableThemes() const {
  std::vector<std::string> result;
  result.reserve(themes.size());
  for (const auto& pair : themes) {
    result.push_back(pair.first);
  }
  return result;
}

std::string ImGuiThemeManager::GetCurrentThemeName() const {
  return currentThemeName;
}

void ImGuiThemeManager::InitializeFonts() {
  fontManager.Initialize();
}

void ImGuiThemeManager::SetFont(const std::string& fontName) {
  fontManager.SetCurrentFont(fontName);
}

void ImGuiThemeManager::SetDefaultFont(float size) {
  fontManager.SetCurrentFont("Geist Mono");
  ImGuiIO& io = ImGui::GetIO();
  io.FontGlobalScale = size;
}

ImFont* ImGuiThemeManager::GetFont(const std::string& fontName) const {
  return fontManager.GetFont(fontName);
}

bool ImGuiThemeManager::LoadFontWithIcons(
  const std::string& name,
  const std::string& path,
  float size
) {
  return fontManager.LoadFontWithIcons(name, path, size);
}

}  // namespace visualization
}  // namespace daa