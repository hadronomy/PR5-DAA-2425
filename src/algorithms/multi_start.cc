#include "algorithms/multi_start.h"

#include "imgui.h"

namespace daa {
namespace algorithm {

void MultiStart::renderConfigurationUI() {
  ImGui::SliderInt("Number of Starts", &num_starts_, 1, 50);

  // Generator selection
  if (ImGui::BeginCombo(
        "Generator", generator_name_.empty() ? "Select Generator" : generator_name_.c_str()
      )) {
    for (const auto& gen : AlgorithmRegistry::getAvailableGenerators()) {
      bool is_selected = (generator_name_ == gen);
      if (ImGui::Selectable(gen.c_str(), is_selected)) {
        generator_name_ = gen;
        // Create new generator instance when selection changes
        using MetaFactory = MetaHeuristicFactory<
          VRPTSolution,
          VRPTProblem,
          TypedAlgorithm<VRPTProblem, VRPTSolution>>;
        try {
          generator_ = MetaFactory::createGenerator(generator_name_);
        } catch (const std::exception&) {
          generator_.reset();
        }
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // Local search selection
  if (ImGui::BeginCombo(
        "Local Search", search_name_.empty() ? "Select Search" : search_name_.c_str()
      )) {
    for (const auto& search : AlgorithmRegistry::getAvailableSearches()) {
      bool is_selected = (search_name_ == search);
      if (ImGui::Selectable(search.c_str(), is_selected)) {
        search_name_ = search;
        // Create new local search instance when selection changes
        using MetaFactory = MetaHeuristicFactory<
          VRPTSolution,
          VRPTProblem,
          TypedAlgorithm<VRPTProblem, VRPTSolution>>;
        try {
          local_search_ = MetaFactory::createSearch(search_name_);
        } catch (const std::exception&) {
          local_search_.reset();
        }
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // Generator configuration
  if (generator_ && !generator_name_.empty()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Generator Configuration:");
    ImGui::Indent(10.0f);
    generator_->renderConfigurationUI();
    ImGui::Unindent(10.0f);
  }

  // Local search configuration
  if (local_search_ && !search_name_.empty()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Local Search Configuration:");
    ImGui::Indent(10.0f);
    local_search_->renderConfigurationUI();
    ImGui::Unindent(10.0f);
  }
}

}  // namespace algorithm
}  // namespace daa