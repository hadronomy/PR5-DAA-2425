#pragma once

// Only include ImGui directly
#include "imgui.h"

#include "raylib.h"

/**
 * Class to manage the ImGui visualization window and rendering using raylib
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
  int screen_width_;
  int screen_height_;
  Color clear_color_;

  // UI state
  bool show_demo_window_;
  bool show_another_window_;
  bool first_time_;
  ImGuiID dockspace_id_;

  // Setup methods
  void setupImGui();
  void setupDocking();

  // Rendering methods
  void renderMenuBar();
  void renderLeftPanel();
  void renderRightPanel();
  void renderMainWindows();

  // Utility functions
  float scaleToDPI(float value);
  int scaleToDPI(int value);
};