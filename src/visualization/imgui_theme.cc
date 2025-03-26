#include "visualization/imgui_theme.h"
#include <vector>

// Initialize global instance
ImGuiThemeManager g_ImGuiThemeManager;

ImGuiThemeManager::ImGuiThemeManager() : currentTheme(ThemeType::Default) {
  // Initialize the theme function map
  themeMap[ThemeType::ComfortableDarkCyan] = &ApplyComfortableDarkCyanTheme;
  themeMap[ThemeType::Default] = &ApplyDefaultTheme;

  // Initialize theme name map
  themeNameMap["Comfortable Dark Cyan"] = ThemeType::ComfortableDarkCyan;
  themeNameMap["Default"] = ThemeType::Default;
}

void ImGuiThemeManager::ApplyTheme(ThemeType theme) {
  if (themeMap.find(theme) != themeMap.end()) {
    // Make sure ImGui context exists before trying to apply theme
    if (ImGui::GetCurrentContext() != nullptr) {
      themeMap[theme]();
      currentTheme = theme;
    }
  }
}

bool ImGuiThemeManager::ApplyThemeByName(const std::string& themeName) {
  auto it = themeNameMap.find(themeName);
  if (it != themeNameMap.end()) {
    ApplyTheme(it->second);
    return true;
  }
  return false;
}

std::vector<std::string> ImGuiThemeManager::GetAvailableThemes() const {
  std::vector<std::string> result;
  for (const auto& pair : themeNameMap) {
    result.push_back(pair.first);
  }
  return result;
}

std::string ImGuiThemeManager::GetCurrentThemeName() const {
  for (const auto& pair : themeNameMap) {
    if (pair.second == currentTheme) {
      return pair.first;
    }
  }
  return "Unknown";
}

void ImGuiThemeManager::ApplyComfortableDarkCyanTheme() {
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

  // Define gray accent colors instead of cyan
  ImVec4 lightGray = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
  ImVec4 mediumGray = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
  ImVec4 darkGray = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

  // Colors remain the same except blue/cyan accents which we replace with gray tones
  style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  style.Colors[ImGuiCol_TextDisabled] =
    ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
  style.Colors[ImGuiCol_WindowBg] =
    ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
  style.Colors[ImGuiCol_ChildBg] =
    ImVec4(0.09411764889955521f, 0.1019607856869698f, 0.1176470592617989f, 1.0f);
  style.Colors[ImGuiCol_PopupBg] =
    ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
  style.Colors[ImGuiCol_Border] =
    ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
  style.Colors[ImGuiCol_BorderShadow] =
    ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
  style.Colors[ImGuiCol_FrameBg] =
    ImVec4(0.1137254908680916f, 0.125490203499794f, 0.1529411822557449f, 1.0f);
  style.Colors[ImGuiCol_FrameBgHovered] =
    ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
  style.Colors[ImGuiCol_FrameBgActive] =
    ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
  style.Colors[ImGuiCol_TitleBg] =
    ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
  style.Colors[ImGuiCol_TitleBgActive] =
    ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
  style.Colors[ImGuiCol_TitleBgCollapsed] =
    ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
  style.Colors[ImGuiCol_MenuBarBg] =
    ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarBg] =
    ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrab] =
    ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] =
    ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] =
    ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
  style.Colors[ImGuiCol_CheckMark] = lightGray;
  style.Colors[ImGuiCol_SliderGrab] = mediumGray;
  style.Colors[ImGuiCol_SliderGrabActive] = lightGray;
  style.Colors[ImGuiCol_Button] =
    ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
  style.Colors[ImGuiCol_ButtonHovered] =
    ImVec4(0.1803921610116959f, 0.1882352977991104f, 0.196078434586525f, 1.0f);
  style.Colors[ImGuiCol_ButtonActive] =
    ImVec4(0.1529411822557449f, 0.1529411822557449f, 0.1529411822557449f, 1.0f);
  style.Colors[ImGuiCol_Header] = darkGray;
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
  style.Colors[ImGuiCol_Separator] =
    ImVec4(0.1294117718935013f, 0.1490196138620377f, 0.1921568661928177f, 1.0f);
  style.Colors[ImGuiCol_SeparatorHovered] =
    ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
  style.Colors[ImGuiCol_SeparatorActive] =
    ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
  style.Colors[ImGuiCol_ResizeGrip] =
    ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
  style.Colors[ImGuiCol_ResizeGripHovered] = mediumGray;
  style.Colors[ImGuiCol_ResizeGripActive] = lightGray;
  style.Colors[ImGuiCol_Tab] =
    ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
  style.Colors[ImGuiCol_TabHovered] =
    ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
  style.Colors[ImGuiCol_TabActive] =
    ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
  style.Colors[ImGuiCol_TabUnfocused] =
    ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
  style.Colors[ImGuiCol_TabUnfocusedActive] = darkGray;
  style.Colors[ImGuiCol_PlotLines] = mediumGray;
  style.Colors[ImGuiCol_PlotLinesHovered] = lightGray;
  style.Colors[ImGuiCol_PlotHistogram] = mediumGray;
  style.Colors[ImGuiCol_PlotHistogramHovered] = lightGray;
  style.Colors[ImGuiCol_TableHeaderBg] =
    ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
  style.Colors[ImGuiCol_TableBorderStrong] =
    ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
  style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  style.Colors[ImGuiCol_TableRowBg] =
    ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
  style.Colors[ImGuiCol_TableRowBgAlt] =
    ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
  style.Colors[ImGuiCol_TextSelectedBg] =
    ImVec4(0.9372549057006836f, 0.9372549057006836f, 0.9372549057006836f, 1.0f);
  style.Colors[ImGuiCol_DragDropTarget] = lightGray;
  style.Colors[ImGuiCol_NavHighlight] = mediumGray;
  style.Colors[ImGuiCol_NavWindowingHighlight] = lightGray;
  style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.2f, 0.2f, 0.501960813999176f);
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.2f, 0.2f, 0.501960813999176f);
}

void ImGuiThemeManager::ApplyDefaultTheme() {
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
