#pragma once

#include <string>
#include <string_view>

#include "imgui.h"

namespace daa {
namespace visualization {

class WindowSystem {
 public:
  WindowSystem();
  ~WindowSystem();

  bool Initialize(int width, int height, const char* title);
  void BeginFrame();
  void EndFrame();
  bool ShouldClose() const;
  void FocusWindowByName(std::string_view window_name, bool force_focus = false);
  void CheckMouseButtonsForCanvas();

 private:
  void SetupDocking();
  void ConfigureImGuiStyle();
  void CreateDockspace();
  bool LoadDockingState();
  void SaveDockingState();

  bool first_frame_;
  ImGuiID dockspace_id_;
  std::string ini_filename_;
};

}  // namespace visualization
}  // namespace daa