#pragma once

#include "raylib.h"
#include "visualization/canvas.h"
#include "visualization/object_manager.h"

namespace daa {
namespace visualization {

class UIComponents {
 public:
  UIComponents(ObjectManager* object_manager, Canvas* canvas);
  ~UIComponents();

  void RenderMenuBar();
  void RenderLeftPanel();
  void RenderRightPanel();
  void RenderMainWindows();
  void RenderThemeSelector();

  // State getters/setters
  bool& ShowDemoWindow() { return show_demo_window_; }
  bool& ShowAnotherWindow() { return show_another_window_; }
  bool& ShowObjectWindow() { return show_object_window_; }
  Color& GetClearColor() { return clear_color_; }
  bool& ShowShaderDebug() { return show_shader_debug_; }

 private:
  // References to other components
  ObjectManager* object_manager_;
  Canvas* canvas_;

  // UI state
  bool show_demo_window_;
  bool show_another_window_;
  bool show_object_window_;
  bool show_shader_debug_ = false;
  Color clear_color_;
};

}  // namespace visualization
}  // namespace daa