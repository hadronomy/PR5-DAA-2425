#pragma once

#include "raylib.h"
#include "visualization/object_manager.h"

namespace daa {
namespace visualization {

// Define a constant for the scale: canvas units to meters conversion
// 1000 units = 1000 meters => 1 unit = 1 meters
constexpr float CANVAS_UNIT_TO_METERS = 10.0f;

class Canvas {
 public:
  Canvas(int width, int height, ObjectManager* object_manager);
  ~Canvas();

  void Resize(int width, int height);
  void Render();
  void RenderWindow();
  void HandleInput(bool is_window_focused, bool is_window_hovered);
  void Update();

  Vector2 ScreenToWorld(const Vector2& screen_pos) const;
  Vector2 WorldToScreen(const Vector2& world_pos) const;

  // Method to get current zoom level
  float GetScale() const { return camera_.zoom; }

  // Method to fit view to a rectangle in world coordinates
  void FitViewToBounds(const Rectangle& bounds, float padding = 0.1f);

  // Debug methods for the shader
  void ToggleShaderDebug() { shader_debug_mode_ = !shader_debug_mode_; }
  bool IsShaderDebugActive() const { return shader_debug_mode_; }
  void SetShaderDebugParam(float value) { shader_debug_param_ = value; }
  float GetShaderDebugParam() const { return shader_debug_param_; }

 private:
  // Canvas state
  int width_;
  int height_;
  bool is_panning_;
  Vector2 last_mouse_pos_;
  float grid_size_;
  bool needs_update_;  // Kept for compatibility, but we always render

  // Render resources
  RenderTexture2D render_texture_;
  Camera2D camera_;

  // Grid shader resources
  Shader grid_shader_;
  bool shader_ready_;  // Flag to track if shader is valid
  int grid_resolution_loc_;
  int grid_scale_loc_;
  int grid_offset_loc_;
  int grid_color_loc_;
  int grid_bg_color_loc_;

  // Shader debug variables
  bool shader_debug_mode_ = false;
  float shader_debug_param_ = 1.0f;
  int shader_debug_mode_loc_ = -1;
  int shader_debug_param_loc_ = -1;

  // New camera matrix uniform locations
  int view_matrix_loc_;
  int inv_view_matrix_loc_;

  // References
  ObjectManager* object_manager_;

  // Internal methods
  void UpdateRenderTexture();
  void DrawGrid();  // Kept for compatibility
  void HandleZooming(const Vector2& mouse_pos);
  void HandlePanning(const Vector2& mouse_pos);
  void DrawScaleLegend();  // New method for drawing the scale legend

  Vector2 CanvasToMeters(const Vector2& canvas_pos) const;
  Vector2 MetersToCanvas(const Vector2& meters_pos) const;

  // Helper method for setting matrix uniforms
  void SetShaderValueMatrix3x3(Shader shader, int locIndex, float* mat);
};

}  // namespace visualization
}  // namespace daa