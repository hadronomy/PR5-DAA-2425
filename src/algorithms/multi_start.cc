#include "algorithms/multi_start.h"

#include "imgui.h"

namespace daa {
namespace algorithm {

void MultiStart::renderConfigurationUI() {
  ImGui::SliderInt("Number of Starts", &num_starts_, 1, 50);

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
  ImGui::TextDisabled("(Hover for info)");
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::Text("TaskExchangeWithinRouteSearch: Swaps tasks within the same route");
    ImGui::Text("TaskExchangeBetweenRoutesSearch: Swaps tasks between different routes");
    ImGui::Text("TaskReinsertionWithinRouteSearch: Moves tasks within the same route");
    ImGui::Text("TaskReinsertionBetweenRoutesSearch: Moves tasks between different routes");
    ImGui::Text("TwoOptSearch: Reverses segments within routes");
    ImGui::EndTooltip();
  }

  // Get all available searches
  const auto& available_searches = AlgorithmRegistry::getAvailableSearches();
  bool searches_changed = false;

  // For each search, check if it's in our selections
  ImGui::BeginChild("Neighborhoods", ImVec2(0, 120), true);
  for (const auto& search : available_searches) {
    bool is_selected =
      std::find(search_names_.begin(), search_names_.end(), search) != search_names_.end();

    bool previous_state = is_selected;
    if (ImGui::Checkbox(search.c_str(), &is_selected) && is_selected != previous_state) {
      if (is_selected) {
        // Add to selections
        search_names_.push_back(search);
      } else {
        // Remove from selections
        search_names_.erase(
          std::remove(search_names_.begin(), search_names_.end(), search), search_names_.end()
        );
      }
      searches_changed = true;
    }
  }
  ImGui::EndChild();

  // Update search components if changed
  if (searches_changed) {
    updateSearches();
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
  if (!search_names_.empty()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Local Search Configurations:");

    for (const auto& search_name : search_names_) {
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