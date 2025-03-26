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
  void SetupDocking();

  int GetScreenWidth() const { return GetScreenWidth(); }
  int GetScreenHeight() const { return GetScreenHeight(); }

 private:
  bool first_frame_;
  ImGuiID dockspace_id_;

  void ConfigureImGuiStyle();
  void ConfigureDockSpace();
};
