#pragma once

#include <memory>

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

  // Initialize the application with window dimensions and title
  bool Initialize(int width, int height, const char* title);

  // Check if the application should close
  bool ShouldClose() const;
  void Run();
  void Shutdown();

  void Update();

 private:
  std::unique_ptr<ObjectManager> object_manager_;
  std::unique_ptr<ProblemManager> problem_manager_;
  std::unique_ptr<UIComponents> ui_components_;
  std::unique_ptr<Canvas> canvas_;
  std::unique_ptr<WindowSystem> window_system_;
};

}  // namespace visualization
}  // namespace daa