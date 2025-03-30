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
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}

}  // namespace algorithm
}  // namespace daa