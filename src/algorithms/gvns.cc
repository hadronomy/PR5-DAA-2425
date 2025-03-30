#include "algorithms/gvns.h"

#include "imgui.h"

namespace daa {
namespace algorithm {

void GVNS::renderConfigurationUI() {
  ImGui::SliderInt("Max Iterations", &max_iterations_, 1, 100);

  // Generator selection
  bool generator_changed = false;
  if (ImGui::BeginCombo(
        "Generator", generator_name_.empty() ? "Select Generator" : generator_name_.c_str()
      )) {
    for (const auto& gen : AlgorithmRegistry::getAvailableGenerators()) {
      bool is_selected = (generator_name_ == gen);
      if (ImGui::Selectable(gen.c_str(), is_selected)) {
        generator_name_ = gen;
        generator_changed = true;
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // Update generator if changed
  if (generator_changed) {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;
    try {
      generator_ = MetaFactory::createGenerator(generator_name_);
    } catch (const std::exception&) {
      generator_.reset();
    }
  }

  ImGui::Text("Neighborhood Structures:");

  // Get all available searches
  auto searches = AlgorithmRegistry::getAvailableSearches();
  bool neighborhoods_changed = false;

  // For each search, check if it's in our selections
  ImGui::BeginChild("Neighborhoods", ImVec2(0, 120), true);
  for (const auto& search : searches) {
    bool is_selected = std::find(neighborhood_names_.begin(), neighborhood_names_.end(), search) !=
                       neighborhood_names_.end();

    bool previous_state = is_selected;
    if (ImGui::Checkbox(search.c_str(), &is_selected) && is_selected != previous_state) {
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
      neighborhoods_changed = true;
    }
  }
  ImGui::EndChild();

  // Update neighborhood components if changed
  if (neighborhoods_changed) {
    updateNeighborhoods();
  }

  // Generator configuration
  if (generator_ && !generator_name_.empty()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Generator Configuration:");
    ImGui::Indent(10.0f);
    generator_->renderConfigurationUI();
    ImGui::Unindent(10.0f);
  }

  // Local search configurations
  if (!neighborhood_names_.empty()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Local Search Configurations:");

    for (const auto& search_name : neighborhood_names_) {
      // Find the corresponding search in the search_map_
      auto search_it = search_map_.find(search_name);
      if (search_it != search_map_.end() && search_it->second) {
        ImGui::PushID(search_name.c_str());
        if (ImGui::CollapsingHeader(search_name.c_str())) {
          ImGui::Indent(10.0f);
          search_it->second->renderConfigurationUI();
          ImGui::Unindent(10.0f);
        }
        ImGui::PopID();
      }
    }
  }
}

}  // namespace algorithm
}  // namespace daa