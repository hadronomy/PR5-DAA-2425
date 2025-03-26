#include "visualization/window_system.h"
#include "imgui_internal.h"
#include "rlImGui.h"
#include "visualization/imgui_theme.h"

WindowSystem::WindowSystem() : first_frame_(true), dockspace_id_(0) {}

WindowSystem::~WindowSystem() {}

bool WindowSystem::Initialize(int width, int height, const char* title) {
  // Set window configuration flags
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);

  // Initialize window with specified dimensions and title
  InitWindow(width, height, title);

  // Set target framerate
  SetTargetFPS(60);

  // Initialize ImGui
  rlImGuiSetup(true);

  // Apply our theme immediately after ImGui is set up
  ConfigureImGuiStyle();

  return true;
}

void WindowSystem::BeginFrame() {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  // Begin ImGui frame
  rlImGuiBegin();

  // Ensure theme is consistently applied at the start of each frame
  if (first_frame_ || ImGui::GetCurrentContext()->FrameCount == 1) {
    ConfigureImGuiStyle();  // Apply theme again on first real frame
  }

  // Setup docking on first frame
  if (first_frame_) {
    SetupDocking();
    first_frame_ = false;
  }
}

void WindowSystem::EndFrame() {
  // End ImGui frame
  rlImGuiEnd();

  EndDrawing();
}

bool WindowSystem::ShouldClose() const {
  return WindowShouldClose();
}

void WindowSystem::SetupDocking() {
#ifdef IMGUI_HAS_DOCK
  // Get window viewport information
  const ImGuiViewport* viewport = ImGui::GetMainViewport();

  // Setup dockspace
  dockspace_id_ = ImGui::GetID("MainWindowDockspace");
  ImGui::DockBuilderRemoveNode(dockspace_id_);
  ImGui::DockBuilderAddNode(dockspace_id_, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockspace_id_, viewport->Size);

  // Split the dockspace into sections
  ImGuiID dock_main_id = dockspace_id_;
  ImGuiID dock_left_id =
    ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
  ImGuiID dock_right_id =
    ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

  // Create additional splits for better organization
  ImGuiID dock_left_bottom_id =
    ImGui::DockBuilderSplitNode(dock_left_id, ImGuiDir_Down, 0.4f, nullptr, &dock_left_id);
  ImGuiID dock_right_bottom_id =
    ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.4f, nullptr, &dock_right_id);

  // Dock windows with specific layout
  ImGui::DockBuilderDockWindow("Left Panel", dock_left_id);
  ImGui::DockBuilderDockWindow("Right Panel", dock_right_id);
  ImGui::DockBuilderDockWindow("Canvas", dock_main_id);
  ImGui::DockBuilderDockWindow("Object Controls", dock_right_bottom_id);

  ImGui::DockBuilderFinish(dockspace_id_);
#endif
}

// Rename to avoid conflict with global function
void WindowSystem::ConfigureImGuiStyle() {
  // Use our theme manager to apply the Comfortable Dark Cyan theme
  g_ImGuiThemeManager.ApplyTheme(ImGuiThemeManager::ThemeType::ComfortableDarkCyan);
}
