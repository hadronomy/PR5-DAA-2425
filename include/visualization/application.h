#pragma once

#include <concepts>
#include <expected>
#include <format>
#include <functional>
#include <memory>
#include <string_view>

#include "visualization/canvas.h"
#include "visualization/object_manager.h"
#include "visualization/problem_manager.h"
#include "visualization/ui_components.h"
#include "visualization/window_system.h"

namespace daa::visualization {

/**
 * @brief Modern visualization application using C++23 design patterns
 *
 * This class manages the visualization components and their lifecycle.
 */
class VisApplication {
 public:
  // Configuration struct for application initialization
  struct Config {
    int width = 1280;
    int height = 720;
    std::string_view title = "VRPT Visualization";
    std::string_view examples_dir = "examples/";
    bool auto_fit_view = true;
    float view_padding = 0.15f;
  };

  // Error types that can be returned from operations
  enum class Error { WindowInitFailed, ComponentInitFailed, InvalidState };

  // Result type for operations that can fail
  template <typename T>
  using Result = std::expected<T, Error>;

  // Default constructor
  VisApplication() = default;

  // Move-only semantics
  VisApplication(VisApplication&&) noexcept = default;
  VisApplication& operator=(VisApplication&&) noexcept = default;

  // No copying
  VisApplication(const VisApplication&) = delete;
  VisApplication& operator=(const VisApplication&) = delete;

  // Destructor
  ~VisApplication();

  /**
   * @brief Initialize the application with the given configuration
   *
   * @param config Configuration parameters
   * @return Result<bool> True if initialization succeeded, or an error
   */
  Result<bool> initialize(const Config& config);

  /**
   * @brief Check if the application should close
   *
   * @return bool True if the application should close
   */
  [[nodiscard]] bool shouldClose() const;

  /**
   * @brief Run the application main loop
   */
  void run();

  /**
   * @brief Shutdown the application and release resources
   */
  void shutdown();

  /**
   * @brief Update the application state for one frame
   */
  void update();

  /**
   * @brief Get a reference to the problem manager
   *
   * @return ProblemManager& Reference to the problem manager
   * @throws std::runtime_error if problem manager is not initialized
   */
  [[nodiscard]] ProblemManager& getProblemManager();

  /**
   * @brief Get a reference to the canvas
   *
   * @return Canvas& Reference to the canvas
   * @throws std::runtime_error if canvas is not initialized
   */
  [[nodiscard]] Canvas& getCanvas();

 private:
  // Component ownership using unique_ptr for RAII
  std::unique_ptr<ObjectManager> object_manager_;
  std::unique_ptr<ProblemManager> problem_manager_;
  std::unique_ptr<UIComponents> ui_components_;
  std::unique_ptr<Canvas> canvas_;
  std::unique_ptr<WindowSystem> window_system_;

  // Application state
  bool initialized_ = false;
};

// Error formatting for std::format
std::string to_string(VisApplication::Error error);

}  // namespace daa::visualization