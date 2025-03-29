#include "visualization/application.h"

#include "imgui.h"
#include "rlImGui.h"

namespace daa {
namespace visualization {

VisApplication::VisApplication() {}

VisApplication::~VisApplication() {
  Shutdown();
}

bool VisApplication::Initialize(int width, int height, const char* title) {
  // Initialize window system
  window_system_ = std::make_unique<WindowSystem>();
  if (!window_system_->Initialize(width, height, title)) {
    return false;
  }

  // Initialize object manager
  object_manager_ = std::make_unique<ObjectManager>();
  object_manager_->Initialize();

  // Initialize canvas after object manager is ready
  canvas_ = std::make_unique<Canvas>(width, height, object_manager_.get());

  // Initialize problem manager (add this before scanning directory)
  problem_manager_ = std::make_unique<ProblemManager>();

  // Set up callback to automatically fit view when a problem is loaded
  problem_manager_->SetProblemLoadedCallback([this](const Rectangle& bounds) {
    if (canvas_) {
      canvas_->FitViewToBounds(bounds, 0.15f);  // 15% padding
    }
  });

  problem_manager_->scanDirectory("examples/");

  // Initialize UI components after object manager is ready
  ui_components_ = std::make_unique<UIComponents>(object_manager_.get());
  ui_components_->SetProblemManager(problem_manager_.get());
  ui_components_->Initialize();

  // Setup ImGui
  // SetupImGui();

  return true;
}

void VisApplication::Run() {
  while (!window_system_->ShouldClose()) {
    Update();
  }
}

void VisApplication::Update() {
  // Begin the frame
  window_system_->BeginFrame();

  // Render the UI components (including problem selector, inspector, etc.)
  ui_components_->RenderUI();

  // Render the canvas window
  canvas_->RenderWindow();

  // End the frame
  window_system_->EndFrame();
}

void VisApplication::Shutdown() {
  if (!window_system_->ShouldClose()) {

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