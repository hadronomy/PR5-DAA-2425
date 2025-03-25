#pragma once

#include <string>
#include <thread>

// ImGui and OpenGL headers
#include <GLFW/glfw3.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "imgui_internal.h"

/**
 * Class to manage the ImGui visualization window and rendering
 */
class GuiManager {
 public:
  GuiManager();
  ~GuiManager();

  /**
   * Initialize the visualization window
   * @return true if initialization was successful
   */
  bool initialize();

  /**
   * Run the main loop for the visualization
   */
  void run();

  /**
   * Cleanup resources
   */
  void cleanup();

 private:
  // Window and rendering state
  GLFWwindow* window_;
  const char* glsl_version_;
  ImVec4 clear_color_;

  // UI state
  bool show_demo_window_;
  bool show_another_window_;
  bool first_time_;
  ImGuiID dockspace_id_;

  // Setup methods
  void setupImGui();
  void setupDocking(const ImGuiViewport* viewport);

  // Rendering methods
  void renderMenuBar();
  void renderLeftPanel();
  void renderRightPanel();
  void renderMainWindows();

  // Helper function for GLFW error callback
  static void glfw_error_callback(int error, const char* description);
};