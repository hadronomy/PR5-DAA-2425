#include "visualization/application.h"

#include <format>
#include <stdexcept>

#include "raylib.h"
#include "rlImGui.h"

namespace daa::visualization {

// Error to string conversion for formatting
std::string to_string(VisApplication::Error error) {
  switch (error) {
    case VisApplication::Error::WindowInitFailed:
      return "Window initialization failed";
    case VisApplication::Error::ComponentInitFailed:
      return "Component initialization failed";
    case VisApplication::Error::InvalidState:
      return "Invalid application state";
    default:
      return "Unknown error";
  }
}

VisApplication::~VisApplication() {
  shutdown();
}

VisApplication::Result<bool> VisApplication::initialize(const Config& config) {
  // Prevent double initialization
  if (initialized_) {
    return std::unexpected(Error::InvalidState);
  }

  try {
    // Initialize window system
    window_system_ = std::make_unique<WindowSystem>();

    // Create window system configuration
    WindowSystem::Config ws_config;
    ws_config.width = config.width;
    ws_config.height = config.height;
    ws_config.title = std::string(config.title);

    // Initialize window system with configuration
    auto ws_result = window_system_->initialize(ws_config);
    if (!ws_result) {
      return std::unexpected(Error::WindowInitFailed);
    }

    // Initialize object manager
    object_manager_ = std::make_unique<ObjectManager>();
    object_manager_->Initialize();

    // Initialize canvas after object manager is ready
    canvas_ = std::make_unique<Canvas>(config.width, config.height, object_manager_.get());

    // Initialize problem manager
    problem_manager_ = std::make_unique<ProblemManager>();

    // Set up callback to automatically fit view when a problem is loaded
    if (config.auto_fit_view) {
      problem_manager_->SetProblemLoadedCallback(
        [this, padding = config.view_padding](const Rectangle& bounds) {
          if (canvas_) {
            canvas_->FitViewToBounds(bounds, padding);
          }
        }
      );
    }

    // Scan for problem files
    problem_manager_->scanDirectory(std::string(config.examples_dir));

    // Initialize UI components after object manager is ready
    ui_components_ = std::make_unique<UIComponents>(object_manager_.get());
    ui_components_->SetProblemManager(problem_manager_.get());
    ui_components_->SetCanvas(canvas_.get());
    ui_components_->Initialize();

    initialized_ = true;
    return true;
  } catch (const std::exception& e) {
    // Clean up any partially initialized components
    shutdown();
    return std::unexpected(Error::ComponentInitFailed);
  }
}

bool VisApplication::shouldClose() const {
  return !initialized_ || (window_system_ && window_system_->ShouldClose());
}

void VisApplication::run() {
  if (!initialized_ || !window_system_) {
    return;
  }

  while (!window_system_->ShouldClose()) {
    update();
  }
}

void VisApplication::update() {
  if (!initialized_ || !window_system_) {
    return;
  }

  // Begin the frame
  window_system_->BeginFrame();

  // Check for algorithm completion if one is running
  if (problem_manager_ && problem_manager_->isAlgorithmRunning()) {
    problem_manager_->checkAlgorithmCompletion();
  }

  // Render the UI components
  if (ui_components_) {
    ui_components_->RenderUI();
  }

  // Render the canvas window
  if (canvas_) {
    canvas_->RenderWindow();
  }

  // End the frame
  window_system_->EndFrame();
}

void VisApplication::shutdown() {
  // Only perform shutdown if we're initialized and window system exists
  if (initialized_ && window_system_) {
    // Clean up in reverse order of initialization
    ui_components_.reset();
    canvas_.reset();
    problem_manager_.reset();
    object_manager_.reset();

    // Shutdown ImGui if window is still open
    if (!window_system_->ShouldClose()) {
      rlImGuiShutdown();
      CloseWindow();
    }

    window_system_.reset();
  }

  initialized_ = false;
}

ProblemManager& VisApplication::getProblemManager() {
  if (!problem_manager_) {
    throw std::runtime_error("Problem manager not initialized");
  }
  return *problem_manager_;
}

Canvas& VisApplication::getCanvas() {
  if (!canvas_) {
    throw std::runtime_error("Canvas not initialized");
  }
  return *canvas_;
}

}  // namespace daa::visualization