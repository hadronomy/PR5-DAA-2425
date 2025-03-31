#pragma once

#include <optional>
#include <string>

#include "imgui.h"

#include "visualization/canvas.h"
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

  // Set canvas reference for route focusing
  void SetCanvas(Canvas* canvas) { canvas_ = canvas; }

  void RenderUI();

 private:
  // Window components
  void RenderEmptyStateOverlay();
  void RenderProblemSelector();
  void RenderProblemInspector();
  void RenderAlgorithmSelector();
  void RenderWarningDialog(const char* title, const char* message, bool* p_open);
  void RenderSolutionStatsWindow();

  // File dialog helper
  std::string OpenFileDialog(const char* title, const char* const* filters, int num_filters);

  // Visualization
  void RenderProblemVisualization();
  void RenderDiagonalPattern(
    const ImVec2& min,
    const ImVec2& max,
    ImU32 color,
    float thickness,
    float spacing
  );

  // Route selection and focusing
  void SetSelectedRoute(const std::string& route_type, int route_index);
  void ClearSelectedRoute();
  void FocusOnSelectedRoute();

  struct RouteSelection {
    std::string type;  // "cv" or "tv"
    int index;         // Route index
  };
  std::optional<RouteSelection> selected_route_;
  Canvas* canvas_;

  ObjectManager* object_manager_;
  ProblemManager* problem_manager_;

  bool show_problem_selector_;
  bool show_problem_inspector_;
  bool show_algorithm_selector_;
  bool show_no_algorithm_warning_;
  bool show_solution_stats_;
  bool show_benchmark_results_;
};

}  // namespace visualization
}  // namespace daa