#include "visualization/ui_components.h"
#include "imgui.h"
#include "visualization/imgui_theme.h"

namespace daa {
namespace visualization {

UIComponents::UIComponents(ObjectManager* object_manager, Canvas* canvas)
    : object_manager_(object_manager),
      canvas_(canvas),
      show_demo_window_(true),
      show_another_window_(false),
      show_object_window_(true),
      show_shader_debug_(false),
      clear_color_(DARKGRAY) {}

UIComponents::~UIComponents() {}

void UIComponents::RenderMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        CloseWindow();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Demo Window", nullptr, &show_demo_window_);
      ImGui::MenuItem("Another Window", nullptr, &show_another_window_);
      ImGui::MenuItem("Object Controls", nullptr, &show_object_window_);
      // Add shader debug menu item
      ImGui::MenuItem("Shader Debug", nullptr, &show_shader_debug_);
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

void UIComponents::RenderLeftPanel() {
  ImGui::Begin("Left Panel");
  ImGui::Text("Left Side Panel");
  ImGui::Separator();
  ImGui::TextWrapped("This panel could contain navigation, properties, or other controls.");

  if (ImGui::CollapsingHeader("Settings")) {
    float color[4] = {
      clear_color_.r / 255.0f,
      clear_color_.g / 255.0f,
      clear_color_.b / 255.0f,
      clear_color_.a / 255.0f
    };

    if (ImGui::ColorEdit3("Background Color", color)) {
      clear_color_ = Color{
        (unsigned char)(color[0] * 255),
        (unsigned char)(color[1] * 255),
        (unsigned char)(color[2] * 255),
        (unsigned char)(color[3] * 255)
      };
    }
  }

  ImGui::End();
}

void UIComponents::RenderRightPanel() {
  ImGui::Begin("Right Panel");

  // Title with styling
  ImGui::PushFont(GetThemeManager().GetFont("Geist"));
  ImGui::TextColored(ImVec4(0.65f, 0.65f, 1.0f, 1.0f), "Performance Monitor");
  ImGui::PopFont();

  ImGui::Separator();

  // Performance stats with better formatting
  ImGuiIO& io = ImGui::GetIO();

  // Collect frame time history
  static float frameTimes[120] = {};
  static float fpsTimes[120] = {};
  static int valuesOffset = 0;

  // Update values
  frameTimes[valuesOffset] = 1000.0f / io.Framerate;
  fpsTimes[valuesOffset] = io.Framerate;
  valuesOffset = (valuesOffset + 1) % IM_ARRAYSIZE(frameTimes);

  // Calculate stats
  float frameTimeAvg = 0.0f;
  float fpsAvg = 0.0f;
  for (int n = 0; n < IM_ARRAYSIZE(frameTimes); n++) {
    frameTimeAvg += frameTimes[n];
    fpsAvg += fpsTimes[n];
  }
  frameTimeAvg /= (float)IM_ARRAYSIZE(frameTimes);
  fpsAvg /= (float)IM_ARRAYSIZE(fpsTimes);

  // Display current values
  ImGui::BeginGroup();
  ImGui::TextColored(
    ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Current Frame Time: %.2f ms", 1000.0f / io.Framerate
  );
  ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Current FPS: %.1f", io.Framerate);
  ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Raylib FPS: %d", GetFPS());
  ImGui::TextColored(
    ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Average: %.2f ms/frame (%.1f FPS)", frameTimeAvg, fpsAvg
  );
  ImGui::EndGroup();

  ImGui::Spacing();

  // FPS Graph
  ImGui::Text("FPS History");
  char overlay[32];
  sprintf(overlay, "%.1f FPS", io.Framerate);
  ImGui::PlotLines(
    "##fps", fpsTimes, IM_ARRAYSIZE(fpsTimes), valuesOffset, overlay, 0.0f, 200.0f, ImVec2(0, 80)
  );

  // Frame time graph
  ImGui::Text("Frame Time History");
  sprintf(overlay, "%.2f ms", 1000.0f / io.Framerate);
  ImGui::PlotLines(
    "##frametime",
    frameTimes,
    IM_ARRAYSIZE(frameTimes),
    valuesOffset,
    overlay,
    0.0f,
    40.0f,
    ImVec2(0, 80)
  );

  // System info section
  if (ImGui::CollapsingHeader("System Information")) {
    ImGui::TextWrapped("Resolution: %dx%d", GetScreenWidth(), GetScreenHeight());
    ImGui::TextWrapped("GPU: %s", GetMonitorName(0));
  }

  // Theme selector
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Appearance");
  RenderThemeSelector();

  ImGui::End();
}

void UIComponents::RenderMainWindows() {
  if (show_shader_debug_ && canvas_) {
    ImGui::Begin("Shader Debug", &show_shader_debug_);

    bool debug_active = canvas_->IsShaderDebugActive();
    if (ImGui::Checkbox("Enable Debug Mode", &debug_active)) {
      canvas_->ToggleShaderDebug();
    }

    if (debug_active) {
      float param = canvas_->GetShaderDebugParam();
      if (ImGui::SliderFloat("Debug Mode", &param, 0.0f, 4.99f)) {
        canvas_->SetShaderDebugParam(param);
      }

      ImGui::Text("Modes:");
      ImGui::Text("0: World Position");
      ImGui::Text("1: Minor Grid");
      ImGui::Text("2: Medium Grid");
      ImGui::Text("3: Major Grid");
      ImGui::Text("4: Screen Derivatives");
    }

    ImGui::End();
  }
}

void UIComponents::RenderThemeSelector() {
  if (ImGui::BeginCombo("Theme", GetThemeManager().GetCurrentThemeName().c_str())) {
    auto themes = GetThemeManager().GetAvailableThemes();
    for (const auto& theme : themes) {
      bool is_selected = (GetThemeManager().GetCurrentThemeName() == theme);
      if (ImGui::Selectable(theme.c_str(), is_selected)) {
        GetThemeManager().ApplyTheme(theme);
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}

}  // namespace visualization
}  // namespace daa