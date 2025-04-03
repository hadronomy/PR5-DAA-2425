#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "imgui.h"

namespace daa::visualization {

/**
 * @brief Modern window system using C++23 design patterns
 *
 * Manages the application window and ImGui docking system.
 */
class WindowSystem {
 public:
  // Configuration for window initialization
  struct Config {
    int width = 1280;
    int height = 720;
    std::string title = "VRPT Visualization";
    std::optional<std::filesystem::path> layout_path = std::nullopt;
    bool vsync = true;
    bool resizable = true;
    bool msaa = true;
    int target_fps = 60;
  };

  // Error types
  enum class Error { InitializationFailed, DockingSetupFailed, LayoutLoadFailed };

  // Result type for operations that can fail
  template <typename T>
  using Result = std::expected<T, Error>;

  // Constructor with default configuration
  explicit WindowSystem();

  // Destructor saves docking state
  ~WindowSystem();

  // Move-only semantics
  WindowSystem(WindowSystem&&) noexcept = default;
  WindowSystem& operator=(WindowSystem&&) noexcept = default;

  // No copying
  WindowSystem(const WindowSystem&) = delete;
  WindowSystem& operator=(const WindowSystem&) = delete;

  /**
   * @brief Initialize the window system
   *
   * @param config Window configuration
   * @return true if initialization succeeded
   */
  bool Initialize(int width, int height, const char* title);

  /**
   * @brief Initialize with configuration object
   *
   * @param config Window configuration
   * @return Result<bool> True if successful, or an error
   */
  Result<bool> initialize(const Config& config);

  /**
   * @brief Begin a new frame
   */
  void BeginFrame();

  /**
   * @brief End the current frame
   */
  void EndFrame();

  /**
   * @brief Check if the window should close
   *
   * @return true if the window should close
   */
  [[nodiscard]] bool ShouldClose() const;

  /**
   * @brief Focus a specific ImGui window by name
   *
   * @param window_name Name of the window to focus
   * @param force_focus Force focus even if window is already focused
   */
  void FocusWindowByName(std::string_view window_name, bool force_focus = false);

  /**
   * @brief Get the current dockspace ID
   *
   * @return ImGuiID The dockspace ID
   */
  [[nodiscard]] ImGuiID getDockspaceId() const noexcept { return dockspace_id_; }

  /**
   * @brief Save the current docking layout
   *
   * @param path Optional path to save to (uses default if not provided)
   * @return true if saved successfully
   */
  bool saveLayout(const std::optional<std::filesystem::path>& path = std::nullopt);

  /**
   * @brief Load a docking layout
   *
   * @param path Optional path to load from (uses default if not provided)
   * @return true if loaded successfully
   */
  bool loadLayout(const std::optional<std::filesystem::path>& path = std::nullopt);

 private:
  // Setup methods
  void SetupDocking();
  void ConfigureImGuiStyle();
  void CreateDockspace();
  bool LoadDockingState();
  void SaveDockingState();

  // State
  bool first_frame_ = true;
  ImGuiID dockspace_id_ = 0;
  std::string ini_filename_;
  bool initialized_ = false;
  Config config_{};
};

// Error formatting for std::format
std::string to_string(WindowSystem::Error error);

}  // namespace daa::visualization