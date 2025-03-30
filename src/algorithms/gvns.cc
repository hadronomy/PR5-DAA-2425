#include "algorithms/gvns.h"

#include "imgui.h"

namespace daa {
namespace algorithm {

void GVNS::renderConfigurationUI() {
  ImGui::SliderInt("Max Iterations", &max_iterations_, 1, 100);

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

  ImGui::Text("Neighborhood Structures:");

  // Get all available searches
  auto searches = AlgorithmRegistry::getAvailableSearches();

  // For each search, check if it's in our selections
  ImGui::BeginChild("Neighborhoods", ImVec2(0, 120), true);
  for (const auto& search : searches) {
    bool is_selected =
      std::find(neighborhood_names_.begin(), neighborhood_names_.end(), search) !=
      neighborhood_names_.end();

    if (ImGui::Checkbox(search.c_str(), &is_selected)) {
      if (is_selected) {
        // Add to selections
        neighborhood_names_.push_back(search);
      } else {
        // Remove from selections
        neighborhood_names_.erase(
          std::remove(neighborhood_names_.begin(), neighborhood_names_.end(), search),
          neighborhood_names_.end()
        );
      }
    }
  }
  ImGui::EndChild();
}
  
}
}  // namespace daa