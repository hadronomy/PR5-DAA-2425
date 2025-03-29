#pragma once

#include <string>

#include "imgui.h"

#include "visualization/object_manager.h"
#include "visualization/problem_manager.h"

namespace daa {
namespace visualization {

class UIComponents {
 public:
  UIComponents(ObjectManager* object_manager);
  ~UIComponents();

  void Initialize();
  void SetProblemManager(ProblemManager* problem_manager);
  void ScanProblemsDirectory(const std::string& dir_path = "examples");

  void RenderUI();

 private:
  // Window components
  void RenderEmptyStateOverlay();
  void RenderProblemSelector();
  void RenderProblemInspector();
  void RenderAlgorithmSelector();
  void RenderWarningDialog(const char* title, const char* message, bool* p_open);

  // Visualization
  void RenderProblemVisualization();
  void RenderDiagonalPattern(
    const ImVec2& min,
    const ImVec2& max,
    ImU32 color,
    float thickness,
    float spacing
  );

  ObjectManager* object_manager_;
  ProblemManager* problem_manager_;

  bool show_problem_selector_;
  bool show_problem_inspector_;
  bool show_algorithm_selector_;
  bool show_no_algorithm_warning_;
};

}  // namespace visualization
}  // namespace daa