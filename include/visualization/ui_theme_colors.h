#pragma once

#include "imgui.h"
#include "visualization/imgui_theme.h"

namespace daa {
namespace visualization {
namespace theme {

// Function to get theme colors to avoid magic numbers in UI code
// These are based on the current theme's colors

// Text colors
inline ImVec4 GetPrimaryTextColor() {
    return ImGui::GetStyle().Colors[ImGuiCol_Text];
}

inline ImVec4 GetSecondaryTextColor() {
    return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
}

inline ImVec4 GetHeaderTextColor() {
    return ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
}

inline ImVec4 GetAccentTextColor() {
    return ImVec4(1.0f, 0.8f, 0.4f, 1.0f);
}

inline ImVec4 GetSuccessTextColor() {
    return ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
}

inline ImVec4 GetErrorTextColor() {
    return ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
}

inline ImVec4 GetWarningTextColor() {
    return ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
}

// Button colors
inline ImVec4 GetButtonColor() {
    return ImGui::GetStyle().Colors[ImGuiCol_Button];
}

inline ImVec4 GetButtonHoveredColor() {
    return ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered];
}

inline ImVec4 GetButtonActiveColor() {
    return ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
}

// Icon button colors - neutral gray tones
inline ImVec4 GetIconButtonColor() {
    return ImVec4(0.15f, 0.15f, 0.15f, 0.8f);
}

inline ImVec4 GetIconButtonHoveredColor() {
    return ImVec4(0.25f, 0.25f, 0.25f, 0.9f);
}

inline ImVec4 GetIconButtonActiveColor() {
    return ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
}

// Background colors
inline ImVec4 GetWindowBgColor() {
    return ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
}

inline ImVec4 GetFrameBgColor() {
    return ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
}

// Convert ImVec4 to ImU32 for use with drawing functions
inline ImU32 ColorToU32(const ImVec4& color) {
    return ImGui::ColorConvertFloat4ToU32(color);
}

// Predefined colors as ImU32 for drawing functions
inline ImU32 GetHeaderColorU32() {
    return ColorToU32(GetHeaderTextColor());
}

inline ImU32 GetAccentColorU32() {
    return ColorToU32(GetAccentTextColor());
}

inline ImU32 GetErrorColorU32() {
    return ColorToU32(GetErrorTextColor());
}

inline ImU32 GetWarningColorU32() {
    return ColorToU32(GetWarningTextColor());
}

inline ImU32 GetSuccessColorU32() {
    return ColorToU32(GetSuccessTextColor());
}

} // namespace theme
} // namespace visualization
} // namespace daa
