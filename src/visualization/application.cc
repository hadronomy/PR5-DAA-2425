#include "visualization/application.h"
#include "rlImGui.h"

namespace daa {
namespace visualization {

VisApplication::VisApplication()
    : screen_width_(1280), screen_height_(720), clear_color_(DARKGRAY), running_(false) {}

VisApplication::~VisApplication() {
  Shutdown();
}

bool VisApplication::Initialize(int width, int height, const char* title) {
  screen_width_ = width;
  screen_height_ = height;

  // Initialize window system
  window_system_ = std::make_unique<WindowSystem>();
  if (!window_system_->Initialize(width, height, title)) {
    return false;
  }

  // Initialize object manager
  object_manager_ = std::make_unique<ObjectManager>();
  object_manager_->Initialize();

  // Initialize canvas
  canvas_ = std::make_unique<Canvas>(width, height, object_manager_.get());

  // Initialize UI components
  ui_components_ = std::make_unique<UIComponents>(object_manager_.get(), canvas_.get());

  // Setup ImGui
  SetupImGui();

  running_ = true;
  return true;
}

void VisApplication::SetupImGui() {
  // Set up ImGui with antialiasing enabled
  rlImGuiSetup(true);

  // Enable docking
#ifdef IMGUI_HAS_DOCK
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui::GetIO().ConfigDockingWithShift = false;            // Allow docking without shift key
  ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;  // Enable dragging only from title bar
#endif
}

void VisApplication::Run() {
  while (running_ && !window_system_->ShouldClose()) {
    window_system_->BeginFrame();
    RenderFrame();
    window_system_->EndFrame();
  }
}

void VisApplication::RenderFrame() {
  // Create dockspace
#ifdef IMGUI_HAS_DOCK
  ImGui::DockSpaceOverViewport();
#endif

  // Menu bar
  ui_components_->RenderMenuBar();

  // Side panels
  ui_components_->RenderLeftPanel();
  ui_components_->RenderRightPanel();

  // Main windows
  ui_components_->RenderMainWindows();

  // Object controls window
  if (ui_components_->ShowObjectWindow()) {
    object_manager_->RenderControlWindow(&ui_components_->ShowObjectWindow());
  }

  // Canvas window
  canvas_->RenderWindow();
}

void VisApplication::Shutdown() {
  if (running_) {
    running_ = false;

    // Clean up in reverse order of initialization
    ui_components_.reset();
    canvas_.reset();
    object_manager_.reset();
    window_system_.reset();

    // Shutdown ImGui
    rlImGuiShutdown();

    // Close window (if not already closed)
    CloseWindow();
  }
}

}  // namespace visualization
}  // namespace daa