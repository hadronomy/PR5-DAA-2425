#include <fstream>
#include <string>

#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "rlImGui.h"
#include "utils.h"

#include "visualization/imgui_theme.h"
#include "visualization/window_system.h"

namespace daa {
namespace visualization {

WindowSystem::WindowSystem() : first_frame_(true), dockspace_id_(0) {
  std::string exe_dir = GetExecutablePath().parent_path().string();
  ini_filename_ = exe_dir + "/resources/layouts/imgui.ini";
}

WindowSystem::~WindowSystem() {
  // Save docking state when application closes
  SaveDockingState();
}

bool WindowSystem::Initialize(int width, int height, const char* title) {
  // Set window configuration flags
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);

  const auto icon_path = GetExecutablePath().parent_path().concat("/assets/icon.png");

  // Initialize window with specified dimensions and title
  InitWindow(width, height, title);

  SetWindowIcon(LoadImage(icon_path.string().c_str()));

  // Set target framerate
  SetTargetFPS(60);

  // Initialize ImGui
  rlImGuiSetup(true);

  // Configure ImGui to save window positions and sizes
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = ini_filename_.c_str();

  // Make sure docking is properly enabled with necessary flags
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigDockingWithShift = true;
  io.ConfigWindowsResizeFromEdges = true;
  io.ConfigDockingAlwaysTabBar = true;

  // Initialize fonts before applying theme
  GetThemeManager().InitializeFonts();

  // Apply our theme immediately after ImGui is set up
  ConfigureImGuiStyle();

  // Attempt to load previous docking state
  LoadDockingState();

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

  // Check for mouse wheel activity and focus canvas if needed
  float wheel = GetMouseWheelMove();
  if (wheel != 0) {
    // Try to find and focus the Canvas window automatically when scrolling
    FocusWindowByName("Canvas", false);  // Use hover check for wheel activity
  }

  // Check for mouse buttons and focus canvas if needed
  CheckMouseButtonsForCanvas();

  // Create dockspace every frame
  CreateDockspace();

  // Setup default docking layout on first frame if no saved state
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

// Create the dockspace that will hold our windows
void WindowSystem::CreateDockspace() {
  // Get viewport and window metrics
  ImGuiViewport* viewport = ImGui::GetMainViewport();

  // Configure window flags for the dockspace host window
  ImGuiWindowFlags window_flags =
    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  // Set the window to cover the entire viewport
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  // Style adjustments for the outer window
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  // Begin the dockspace host window (unmovable, spans the entire viewport)
  ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  // Create the actual dockspace inside this window
  dockspace_id_ = ImGui::GetID("MainDockSpace");
  ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

  ImGui::DockSpace(dockspace_id_, ImVec2(0.0f, 0.0f), dockspace_flags);

  // End the dockspace host window
  ImGui::End();
}

void WindowSystem::SetupDocking() {
#ifdef IMGUI_HAS_DOCK
  // Check if the ini file exists before proceeding with manual docking setup
  std::ifstream file(ini_filename_);
  if (file.good()) {
    return;
  }

  // Setup dockspace
  ImGui::DockBuilderRemoveNode(dockspace_id_);
  ImGui::DockBuilderAddNode(
    dockspace_id_,
    static_cast<int>(ImGuiDockNodeFlags_DockSpace) | ImGuiDockNodeFlags_PassthruCentralNode
  );
  ImGui::DockBuilderSetNodeSize(dockspace_id_, ImGui::GetMainViewport()->Size);

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

  // Dock windows with specific layout - using more descriptive names
  ImGui::DockBuilderDockWindow("Algorithm Properties", dock_left_id);
  ImGui::DockBuilderDockWindow("Algorithm Output", dock_left_bottom_id);
  ImGui::DockBuilderDockWindow("Visualization", dock_main_id);
  ImGui::DockBuilderDockWindow("Controls", dock_right_id);
  ImGui::DockBuilderDockWindow("Statistics", dock_right_bottom_id);

  // Keep compatibility with old window names if needed
  ImGui::DockBuilderDockWindow("Left Panel", dock_left_id);
  ImGui::DockBuilderDockWindow("Right Panel", dock_right_id);
  ImGui::DockBuilderDockWindow("Canvas", dock_main_id);
  ImGui::DockBuilderDockWindow("Object Controls", dock_right_bottom_id);

  ImGui::DockBuilderFinish(dockspace_id_);
#endif
}

bool WindowSystem::LoadDockingState() {
  std::ifstream file(ini_filename_.c_str());
  if (file.good()) {
    file.close();
    ImGui::LoadIniSettingsFromDisk(ini_filename_.c_str());
    return true;
  }
  return false;
}

void WindowSystem::SaveDockingState() {
  // Force save the ini file
  ImGui::SaveIniSettingsToDisk(ini_filename_.c_str());
}

// Rename to avoid conflict with global function
void WindowSystem::ConfigureImGuiStyle() {
  // Use our theme manager to apply the Comfortable Dark Cyan theme
  GetThemeManager().ApplyTheme("Comfortable Dark Cyan");
}

void WindowSystem::FocusWindowByName(std::string_view window_name, bool force_focus) {
  // Find the window by name
  ImGuiWindow* window = ImGui::FindWindowByName(window_name.data());
  if (window && !window->Collapsed) {
    // Check if we should force focus or if the mouse is hovering over this window
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    bool is_hovering =
      mouse_pos.x >= window->Pos.x && mouse_pos.x <= window->Pos.x + window->Size.x &&
      mouse_pos.y >= window->Pos.y && mouse_pos.y <= window->Pos.y + window->Size.y;

    if (force_focus || is_hovering) {
      // Focus this window and handle scroll events
      ImGui::SetWindowFocus(window_name.data());
      // Temporarily disable capture for other windows
      ImGui::GetIO().WantCaptureMouse = false;
    }
  }
}

void WindowSystem::CheckMouseButtonsForCanvas() {
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON) ||
      IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
    // When any mouse button is pressed, try to focus the Canvas
    FocusWindowByName("Canvas", false);  // Use hover check
  }
}

}  // namespace visualization
}  // namespace daa