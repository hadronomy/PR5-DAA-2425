#pragma once

#include <string>
#include "imgui.h"

class WindowSystem {
 public:
  WindowSystem();
  ~WindowSystem();

  bool Initialize(int width, int height, const char* title);
  void BeginFrame();
  void EndFrame();
  bool ShouldClose() const;
  void FocusWindowByName(const char* window_name);

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
