#pragma once

#include "imgui.h"

class WindowSystem {
 public:
  WindowSystem();
  ~WindowSystem();

  bool Initialize(int width, int height, const char* title);
  void BeginFrame();
  void EndFrame();
  bool ShouldClose() const;

 private:
  void SetupDocking();
  void ConfigureImGuiStyle();  // This is a member function now

  bool first_frame_;
  ImGuiID dockspace_id_;
};
