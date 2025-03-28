#pragma once

#include <memory>

#include "raylib.h"
#include "visualization/canvas.h"
#include "visualization/object_manager.h"
#include "visualization/ui_components.h"
#include "visualization/window_system.h"

namespace daa {
namespace visualization {

class VisApplication {
 public:
  VisApplication();
  ~VisApplication();

  bool Initialize(int width = 1280, int height = 720, const char* title = "Visualization Window");
  void Run();
  void Shutdown();

 private:
  // Core components
  std::unique_ptr<WindowSystem> window_system_;
  std::unique_ptr<ObjectManager> object_manager_;
  std::unique_ptr<Canvas> canvas_;
  std::unique_ptr<UIComponents> ui_components_;

  // Application state
  int screen_width_;
  int screen_height_;
  Color clear_color_;
  bool running_;

  // ImGui setup
  void SetupImGui();
  void RenderFrame();
};

}  // namespace visualization
}  // namespace daa