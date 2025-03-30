#include "algorithms/solver.h"

#include "imgui.h"

namespace daa {
namespace algorithm {

void VRPTSolver::renderConfigurationUI() {
  // CV Algorithm selection
  if (ImGui::BeginCombo(
        "CV Algorithm", cv_algorithm_.empty() ? "Select CV Algorithm" : cv_algorithm_.c_str()
      )) {
    std::vector<std::string> cv_options = {
      "MultiStart", "GVNS", "GreedyCVGenerator", "GRASPCVGenerator"
    };

    for (const auto& algo : cv_options) {
      bool is_selected = (cv_algorithm_ == algo);
      if (ImGui::Selectable(algo.c_str(), is_selected)) {
        cv_algorithm_ = algo;
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // TV Algorithm selection - usually stays as GreedyTVScheduler
  if (ImGui::BeginCombo(
        "TV Algorithm", tv_algorithm_.empty() ? "Select TV Algorithm" : tv_algorithm_.c_str()
      )) {
    std::vector<std::string> tv_options = {"GreedyTVScheduler"};

    for (const auto& algo : tv_options) {
      bool is_selected = (tv_algorithm_ == algo);
      if (ImGui::Selectable(algo.c_str(), is_selected)) {
        tv_algorithm_ = algo;
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  ImGui::TextWrapped(
    "VRPTSolver uses the specified algorithm for Phase 1 and GreedyTVScheduler for Phase 2"
  );
}

}  // namespace algorithm
}  // namespace daa
