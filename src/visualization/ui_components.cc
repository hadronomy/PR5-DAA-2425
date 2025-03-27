#include "visualization/ui_components.h"
#include "imgui.h"
#include "visualization/imgui_theme.h"

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
      clear_color_ = (Color){(unsigned char)(color[0] * 255),
                             (unsigned char)(color[1] * 255),
                             (unsigned char)(color[2] * 255),
                             (unsigned char)(color[3] * 255)};
    }
  }

  ImGui::End();
}

void UIComponents::RenderRightPanel() {
  ImGui::Begin("Right Panel");
  ImGui::Text("Right Side Panel");
  ImGui::Separator();
  ImGui::TextWrapped("This panel could show details, properties, or other information.");

  // Display some stats
  ImGuiIO& io = ImGui::GetIO();
  ImGui::Text("Performance");
  ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  ImGui::Text("Raylib FPS: %d", GetFPS());

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
