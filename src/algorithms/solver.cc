#include "algorithms/solver.h"

#include "algorithm_registry.h"
#include "imgui.h"

namespace daa {
namespace algorithm {

void VRPTSolver::renderConfigurationUI() {
  // Step 1: Select algorithm type
  std::vector<std::string> meta_algorithms = {"GVNS", "MultiStart-RVND"};

  ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Step 1: Select Algorithm");

  // Get the current algorithm name
  std::string selected_algorithm = cv_algorithm_name_;

  if (ImGui::BeginCombo(
        "CV Algorithm Type",
        selected_algorithm.empty() ? "Select Algorithm" : selected_algorithm.c_str()
      )) {
    for (const auto& algo : meta_algorithms) {
      bool is_selected = (selected_algorithm == algo);
      if (ImGui::Selectable(algo.c_str(), is_selected)) {
        // Set the selected algorithm in the problem manager
        cv_algorithm_name_ = algo;
        cv_algorithm_ =
          AlgorithmRegistry::createTyped<VRPTProblem, VRPTSolution>(cv_algorithm_name_);
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // Step 2: Configure algorithm parameters
  if (!selected_algorithm.empty()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Step 2: Configure Parameters");

    // Render algorithm-specific configuration UI
    cv_algorithm_->renderConfigurationUI();

    ImGui::Separator();
  }

  selected_algorithm = tv_algorithm_name_;

  ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Step 3: Select TV Algorithm");

  std::vector<std::string> tv_meta_algorithms = {"GreedyTVScheduler"};

  // select the tv_algorithm
  if (ImGui::BeginCombo(
        "TV Algorithm Type",
        selected_algorithm.empty() ? "Select Algorithm" : selected_algorithm.c_str()
      )) {
    for (const auto& algo : tv_meta_algorithms) {
      bool is_selected = (selected_algorithm == algo);
      if (ImGui::Selectable(algo.c_str(), is_selected)) {
        // Set the selected algorithm in the problem manager
        tv_algorithm_name_ = algo;
        tv_algorithm_ =
          AlgorithmRegistry::createTyped<VRPTData, VRPTSolution>(tv_algorithm_name_);
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // Step 2: Configure algorithm parameters
  if (!selected_algorithm.empty()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Step 4: Configure Parameters");

    // Render algorithm-specific configuration UI
    tv_algorithm_->renderConfigurationUI();

    ImGui::Separator();
  }
}

}  // namespace algorithm
}  // namespace daa
