#include "visualization/canvas.h"
#include "imgui.h"
#include "raymath.h"
#include "rlImGui.h"

namespace daa {
namespace visualization {

Canvas::Canvas(int width, int height, ObjectManager* object_manager)
    : width_(width),
      height_(height),
      is_panning_(false),
      last_mouse_pos_({0, 0}),
      grid_size_(10.0f),  // Changed to 10.0f to match the new grid cell size
      needs_update_(true),
      object_manager_(object_manager) {

  // Initialize camera directly
  camera_.zoom = 1.0f;
  camera_.target = Vector2{0.0f, 0.0f};
  camera_.offset = Vector2{width_ / 2.0f, height_ / 2.0f};
  camera_.rotation = 0.0f;

  // Initialize render texture with MSAA
  render_texture_ = LoadRenderTexture(width, height);

  // Load grid shader with error handling
  const char* fragmentShaderPath = "resources/shaders/grid.fs";
  grid_shader_ = LoadShader(NULL, TextFormat("%s", fragmentShaderPath));

  // Check if shader is valid (id > 0 means shader is valid)
  shader_ready_ = (grid_shader_.id > 0);

  if (shader_ready_) {
    TraceLog(LOG_INFO, "Shader loaded successfully");
  } else {
    TraceLog(LOG_ERROR, "Failed to load shader: %s", fragmentShaderPath);
    // Fallback to default rendering without shader
  }

  // Get shader uniform locations
  grid_resolution_loc_ = GetShaderLocation(grid_shader_, "resolution");
  grid_scale_loc_ = GetShaderLocation(grid_shader_, "scale");
  grid_offset_loc_ = GetShaderLocation(grid_shader_, "offset");
  grid_color_loc_ = GetShaderLocation(grid_shader_, "gridColor");
  grid_bg_color_loc_ = GetShaderLocation(grid_shader_, "backgroundColor");

  // Get additional shader uniform locations for debugging
  shader_debug_mode_loc_ = GetShaderLocation(grid_shader_, "debugMode");
  shader_debug_param_loc_ = GetShaderLocation(grid_shader_, "debugParam");

  // Set initial shader values based on camera parameters
  float resolution[2] = {(float)width_, (float)height_};
  SetShaderValue(grid_shader_, grid_resolution_loc_, resolution, SHADER_UNIFORM_VEC2);

  // Use camera zoom directly for scale
  SetShaderValue(grid_shader_, grid_scale_loc_, &camera_.zoom, SHADER_UNIFORM_FLOAT);

  // Use camera target for grid offset
  float grid_offset[2] = {camera_.target.x, camera_.target.y};
  SetShaderValue(grid_shader_, grid_offset_loc_, grid_offset, SHADER_UNIFORM_VEC2);

  Color grid_color = ColorAlpha(LIGHTGRAY, 0.4f);
  float grid_color_normalized[4] = {
    (float)grid_color.r / 255.0f,
    (float)grid_color.g / 255.0f,
    (float)grid_color.b / 255.0f,
    (float)grid_color.a
  };
  SetShaderValue(grid_shader_, grid_color_loc_, grid_color_normalized, SHADER_UNIFORM_VEC4);

  Color bg_color = CLITERAL(Color){40, 40, 40, 255};
  float bg_color_normalized[4] = {
    (float)bg_color.r / 255.0f,
    (float)bg_color.g / 255.0f,
    (float)bg_color.b / 255.0f,
    (float)bg_color.a / 255.0f
  };
  SetShaderValue(grid_shader_, grid_bg_color_loc_, bg_color_normalized, SHADER_UNIFORM_VEC4);

  // Set initial debug values
  int debug_mode = 0;  // Off by default
  SetShaderValue(grid_shader_, shader_debug_mode_loc_, &debug_mode, SHADER_UNIFORM_INT);
  SetShaderValue(grid_shader_, shader_debug_param_loc_, &shader_debug_param_, SHADER_UNIFORM_FLOAT);

  // Force first update to ensure all values are properly synchronized
  Update();
}

Canvas::~Canvas() {
  UnloadRenderTexture(render_texture_);
  UnloadShader(grid_shader_);
}

void Canvas::Resize(int width, int height) {
  if (width != width_ || height != height_) {
    width_ = width;
    height_ = height;
    UnloadRenderTexture(render_texture_);
    render_texture_ = LoadRenderTexture(width, height);

    // Update camera offset when window is resized to maintain center positioning
    camera_.offset = Vector2{width_ / 2.0f, height_ / 2.0f};

    needs_update_ = true;
  }
}

void Canvas::Render() {
  // Always update the render texture every frame, like in the example
  UpdateRenderTexture();
}

Vector2 Canvas::ScreenToWorld(const Vector2& screen_pos) const {
  // Use GetScreenToWorld2D from raylib to convert using the camera
  return GetScreenToWorld2D(screen_pos, camera_);
}

Vector2 Canvas::WorldToScreen(const Vector2& world_pos) const {
  // Use GetWorldToScreen2D from raylib to convert using the camera
  return GetWorldToScreen2D(world_pos, camera_);
}

void Canvas::HandleInput(bool is_window_focused, bool is_window_hovered) {
  if (!is_window_focused || !is_window_hovered)
    return;

  // Get the ImGui window position and the content region min
  ImVec2 window_pos = ImGui::GetWindowPos();
  ImVec2 content_min = ImGui::GetWindowContentRegionMin();

  // Calculate canvas origin in screen space
  Vector2 canvas_origin = {window_pos.x + content_min.x, window_pos.y + content_min.y};

  // Get mouse position in screen coordinates
  Vector2 mouse_pos = GetMousePosition();

  // Convert mouse position to canvas-relative coordinates
  Vector2 canvas_mouse_pos = {mouse_pos.x - canvas_origin.x, mouse_pos.y - canvas_origin.y};

  // Handle zooming with mouse wheel
  float wheel = GetMouseWheelMove();
  if (wheel != 0) {
    HandleZooming(canvas_mouse_pos);
  }

  // Handle panning with middle mouse button
  if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
    is_panning_ = true;
    last_mouse_pos_ = canvas_mouse_pos;
  }

  if (IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON)) {
    is_panning_ = false;
  }

  if (is_panning_) {
    HandlePanning(canvas_mouse_pos);
  }

  // Transform mouse position to world space for object interaction
  Vector2 world_mouse_pos = ScreenToWorld(canvas_mouse_pos);

  // Handle object interaction - no need to track if interaction occurred
  object_manager_->HandleObjectInteraction(
    world_mouse_pos,
    IsMouseButtonPressed(MOUSE_LEFT_BUTTON),
    IsMouseButtonReleased(MOUSE_LEFT_BUTTON)
  );
}

void Canvas::Update() {
  if (shader_ready_) {
    // Update shader values for correct grid rendering
    float resolution[2] = {(float)width_, (float)height_};
    SetShaderValue(grid_shader_, grid_resolution_loc_, resolution, SHADER_UNIFORM_VEC2);

    // Pass the zoom directly to the shader
    SetShaderValue(grid_shader_, grid_scale_loc_, &camera_.zoom, SHADER_UNIFORM_FLOAT);

    // Pass the camera target for grid offset
    float grid_offset[2] = {camera_.target.x, camera_.target.y};
    SetShaderValue(grid_shader_, grid_offset_loc_, grid_offset, SHADER_UNIFORM_VEC2);

    // Update debug uniforms
    int debug_mode = shader_debug_mode_ ? 1 : 0;
    SetShaderValue(grid_shader_, shader_debug_mode_loc_, &debug_mode, SHADER_UNIFORM_INT);
    SetShaderValue(
      grid_shader_, shader_debug_param_loc_, &shader_debug_param_, SHADER_UNIFORM_FLOAT
    );
  }
}

void Canvas::HandleZooming(const Vector2& mouse_pos) {
  float wheel = GetMouseWheelMove();

  // Get the world point that is under the mouse before zooming
  Vector2 mouse_world_before = ScreenToWorld(mouse_pos);

  // Apply zoom with logarithmic scaling for consistent zoom speed
  float scale = 0.2f * wheel;
  camera_.zoom = Clamp(expf(logf(camera_.zoom) + scale), 0.125f, 10.0f);

  // After changing the zoom, we need to adjust the target to maintain the position
  // under the cursor

  // Get the new world position after zoom (if we didn't adjust the target)
  Vector2 mouse_world_after = ScreenToWorld(mouse_pos);

  // Adjust target to keep the world point under the mouse stable
  camera_.target.x += (mouse_world_before.x - mouse_world_after.x);
  camera_.target.y += (mouse_world_before.y - mouse_world_after.y);

  // Make sure camera gets updated
  Update();
}

void Canvas::HandlePanning(const Vector2& mouse_pos) {
  Vector2 delta = {mouse_pos.x - last_mouse_pos_.x, mouse_pos.y - last_mouse_pos_.y};

  // Apply panning - adjust target in opposite direction of movement
  // We divide by zoom since we're in screen space
  camera_.target.x -= delta.x / camera_.zoom;
  camera_.target.y -= delta.y / camera_.zoom;

  last_mouse_pos_ = mouse_pos;

  // Make sure camera gets updated
  Update();
}

void Canvas::DrawGrid() {
  // Instead of drawing lines, we'll use the shader when rendering
}

void Canvas::UpdateRenderTexture() {
  BeginTextureMode(render_texture_);
  ClearBackground(CLITERAL(Color){40, 40, 40, 255});

  // Only use the shader if it's ready
  if (shader_ready_) {
    BeginShaderMode(grid_shader_);
    // Draw a full-screen rectangle to apply the shader
    DrawRectangle(0, 0, width_, height_, WHITE);
    EndShaderMode();
  }

  // Begin 2D mode with the properly configured camera
  BeginMode2D(camera_);

  // Draw objects
  object_manager_->DrawObjects();

  // Convert mouse position to canvas-relative coordinates first
  Vector2 canvas_mouse_pos = {
    GetMousePosition().x - ImGui::GetWindowPos().x - ImGui::GetWindowContentRegionMin().x,
    GetMousePosition().y - ImGui::GetWindowPos().y - ImGui::GetWindowContentRegionMin().y
  };

  // Convert to world coordinates (accounting for pan and zoom)
  Vector2 world_mouse_pos = ScreenToWorld(canvas_mouse_pos);

  // Draw the cursor indicator and coordinates text
  DrawCircleV(world_mouse_pos, 4, DARKGRAY);
  DrawTextEx(
    GetFontDefault(),
    TextFormat("[%i, %i]", (int)world_mouse_pos.x, (int)world_mouse_pos.y),
    Vector2Add(
      world_mouse_pos,
      Vector2{
        -44,
        -24,
      }
    ),
    20,
    2,
    BLACK
  );

  // Draw the scale legend
  DrawScaleLegend();

  EndMode2D();
  EndTextureMode();
}

void Canvas::DrawScaleLegend() {
  // Define constants for the scale legend
  const int legendWidth = 100;  // Fixed screen-space width of the legend
  const int padding = 20;       // Padding from the edge of the screen
  const int thickness = 2;      // Line thickness
  const int tickHeight = 5;     // Height of tick marks
  const int textPadding = 5;    // Padding between line and text
  const Color lineColor = WHITE;
  const Color textColor = WHITE;

  // Calculate a nice round scale value based on zoom level
  float worldUnitsPerLegend = legendWidth / camera_.zoom;

  // Round to a "nice" number (1, 2, 5, 10, 20, 50, 100, etc.)
  float magnitude = powf(10.0f, floorf(log10f(worldUnitsPerLegend)));
  float normalized = worldUnitsPerLegend / magnitude;

  // Choose a nice multiplier: 1, 2, 5
  float multiplier = 1.0f;
  if (normalized >= 1.0f && normalized < 2.0f)
    multiplier = 1.0f;
  else if (normalized >= 2.0f && normalized < 5.0f)
    multiplier = 2.0f;
  else if (normalized >= 5.0f)
    multiplier = 5.0f;

  float niceScale = multiplier * magnitude;

  // Calculate the actual pixel length based on the nice scale
  float actualLength = niceScale * camera_.zoom;

  // Calculate top-right corner position in screen space (regardless of camera)
  Vector2 screenPos = {(float)width_ - padding - actualLength, (float)padding};

  // We need to draw this in screen space, outside of the camera's transformation
  EndMode2D();  // End the camera transformation

  // Draw the main scale line
  DrawLineEx(screenPos, Vector2{screenPos.x + actualLength, screenPos.y}, thickness, lineColor);

  // Draw tick marks at both ends
  DrawLineEx(
    screenPos,
    Vector2{
      screenPos.x,
      screenPos.y + tickHeight,
    },
    thickness,
    lineColor
  );
  DrawLineEx(
    Vector2{
      screenPos.x + actualLength,
      screenPos.y,
    },
    Vector2{
      screenPos.x + actualLength,
      screenPos.y + tickHeight,
    },
    thickness,
    lineColor
  );

  // Draw scale text
  const char* scaleText;
  if (niceScale >= 1000) {
    scaleText = TextFormat("%.1f km", niceScale / 1000.0f);
  } else if (niceScale >= 1) {
    scaleText = TextFormat("%.0f m", niceScale);
  } else {
    scaleText = TextFormat("%.0f cm", niceScale * 100.0f);
  }

  // Calculate text position centered below the line
  Vector2 textSize = MeasureTextEx(GetFontDefault(), scaleText, 10, 1);
  Vector2 textPos = {
    screenPos.x + (actualLength - textSize.x) / 2, screenPos.y + tickHeight + textPadding
  };

  // Draw the text
  DrawTextEx(GetFontDefault(), scaleText, textPos, 10, 1, textColor);

  // Resume camera mode for subsequent drawing
  BeginMode2D(camera_);
}

void Canvas::RenderWindow() {
  // ImGui window flags for canvas window
  ImGuiWindowFlags window_flags =
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse;

  // Remove padding to ensure canvas fills the window
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  // Begin canvas window
  ImGui::Begin("Canvas", nullptr, window_flags);
  ImGui::PopStyleVar();

  // Get window content region info
  ImVec2 window_size = ImGui::GetContentRegionAvail();

  // Resize canvas if window size changed
  Resize(window_size.x, window_size.y);

  // Check if window is focused and hovered
  bool focused = ImGui::IsWindowFocused();
  bool hovered = ImGui::IsWindowHovered();

  // Handle input when canvas is active
  if (focused && hovered) {
    // Critical: Temporarily disable ImGui's mouse capture when over canvas area
    ImGui::SetWindowFocus();
    ImGui::GetIO().WantCaptureMouse = false;
    ImGui::GetIO().WantCaptureKeyboard = false;

    // Process input
    HandleInput(focused, hovered);
  } else {
    // Re-enable mouse capture for ImGui when not hovering canvas
    ImGui::GetIO().WantCaptureMouse = true;
    ImGui::GetIO().WantCaptureKeyboard = true;
  }

  // Always update and render the canvas every frame
  Update();
  Render();

  // Draw the render texture to fill the entire content area
  rlImGuiImageRenderTextureFit(&render_texture_, true);

  // Add shader debug controls if shader is ready
  if (shader_ready_ && ImGui::Begin("Canvas Debug", nullptr, ImGuiWindowFlags_NoCollapse)) {
    // Debug Shader Toggle
    if (ImGui::Checkbox("Debug Shader", &shader_debug_mode_)) {
      Update();
    }

    if (shader_debug_mode_) {
      // Debug parameter dropdown
      const char* items[] = {"0.1", "0.5", "1.0", "2.0", "5.0", "10.0"};
      static float values[] = {0.1f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
      static int current_item = 2;

      if (ImGui::Combo("Debug Parameter", &current_item, items, IM_ARRAYSIZE(items))) {
        shader_debug_param_ = values[current_item];
        Update();
      }

      // Help text section
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.78f, 0.78f, 0.5f, 1.0f), "Debug Controls:");
      ImGui::BulletText("Shows different grid aspects based on debug mode");
      ImGui::BulletText("Select parameter to fine-tune visualization");
      ImGui::Separator();
    }

    // Camera Information Section
    if (ImGui::CollapsingHeader("Camera Information", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.8f, 1.0f), "Camera Properties:");
      ImGui::Indent(10);
      ImGui::Text("Target: (%.2f, %.2f)", camera_.target.x, camera_.target.y);
      ImGui::Text("Zoom: %.4f", camera_.zoom);
      ImGui::Text("Grid Size: %.2f", grid_size_);
      ImGui::Unindent(10);

      ImGui::Spacing();

      // Mouse position information
      ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.8f, 1.0f), "Mouse Coordinates:");
      ImGui::Indent(10);
      ImVec2 window_pos = ImGui::GetWindowPos();
      ImVec2 content_min = ImGui::GetWindowContentRegionMin();
      Vector2 canvas_origin = {window_pos.x + content_min.x, window_pos.y + content_min.y};
      Vector2 mouse_pos = GetMousePosition();
      Vector2 canvas_mouse_pos = {mouse_pos.x - canvas_origin.x, mouse_pos.y - canvas_origin.y};
      Vector2 world_mouse_pos = ScreenToWorld(canvas_mouse_pos);

      ImGui::Text("Screen: (%.2f, %.2f)", canvas_mouse_pos.x, canvas_mouse_pos.y);
      ImGui::Text("World:  (%.2f, %.2f)", world_mouse_pos.x, world_mouse_pos.y);
      ImGui::Unindent(10);

      ImGui::Spacing();

      // Camera control buttons
      ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.8f, 1.0f), "Camera Controls:");
      ImGui::Indent(10);

      if (ImGui::Button("Reset Camera", ImVec2(120, 0))) {
        camera_.target = Vector2{0, 0};
        camera_.offset = Vector2{width_ / 2.0f, height_ / 2.0f};
        camera_.zoom = 1.0f;
        Update();
      }

      ImGui::SameLine();

      if (ImGui::Button("Center Origin", ImVec2(120, 0))) {
        camera_.target = Vector2{0, 0};
        camera_.offset = Vector2{width_ / 2.0f, height_ / 2.0f};
        Update();
      }
      ImGui::Unindent(10);
    }

    ImGui::End();  // End of "Canvas Debug" window
  }

  ImGui::End();  // End of "Canvas" window
}

}  // namespace visualization
}  // namespace daa