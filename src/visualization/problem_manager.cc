#include "visualization/problem_manager.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>

#include "algorithm_registry.h"
#include "imgui.h"

namespace fs = std::filesystem;

namespace daa {
namespace visualization {

ProblemManager::ProblemManager() : current_problem_filename_("") {}

bool ProblemManager::scanDirectory(const std::string& dir_path) {
  // Remember the currently selected problem if any
  std::string previously_selected = current_problem_filename_;

  // Clear the cache of loaded problems to force reload from files
  problems_.clear();
  current_problem_filename_ = "";
  // Clear any existing solution
  clearSolution();

  available_problems_.clear();

  try {
    // Check if the directory exists
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
      std::cerr << "Directory does not exist: " << dir_path << std::endl;
      return false;
    }

    // Iterate through the directory
    for (const auto& entry : fs::directory_iterator(dir_path)) {
      if (entry.is_regular_file()) {
        std::string filename = entry.path().filename().string();
        // Filter for .txt files or other appropriate extensions
        if (entry.path().extension() == ".txt") {
          available_problems_.push_back(entry.path().string());
        }
      }
    }

    // Sort alphabetically for better UI presentation
    std::sort(available_problems_.begin(), available_problems_.end());

    // If there was a previously selected problem and it still exists, reload it
    if (!previously_selected.empty()) {
      auto it =
        std::find(available_problems_.begin(), available_problems_.end(), previously_selected);
      if (it != available_problems_.end()) {
        loadProblem(previously_selected);
      }
    }

    return true;
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
    return false;
  } catch (const std::exception& e) {
    std::cerr << "Error scanning directory: " << e.what() << std::endl;
    return false;
  }
}

bool ProblemManager::loadProblem(const std::string& filename) {
  // Clear any existing solution when loading a new problem
  clearSolution();

  // Check if we already have this problem loaded
  if (problems_.find(filename) != problems_.end()) {
    // Use existing problem instance
    current_problem_filename_ = filename;
  } else {
    // Create and load new problem instance
    auto problem = std::make_unique<VRPTProblem>();
    if (!problem->loadFromFile(filename)) {
      std::cerr << "Failed to load problem from: " << filename << std::endl;
      return false;
    }

    // Store the loaded problem
    problems_[filename] = std::move(problem);
    current_problem_filename_ = filename;
  }

  // Calculate bounds and notify via callback
  if (problem_loaded_callback_ && isProblemLoaded()) {
    Rectangle bounds = GetCurrentProblemBounds();
    problem_loaded_callback_(bounds);
  }

  return true;
}

Rectangle ProblemManager::GetCurrentProblemBounds() const {
  if (!isProblemLoaded()) {
    return Rectangle{0, 0, 0, 0};
  }

  auto problem = problems_.at(current_problem_filename_).get();

  // Initialize with extreme values
  float minX = std::numeric_limits<float>::max();
  float minY = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float maxY = std::numeric_limits<float>::lowest();

  bool hasLocations = false;

  // Process all types of locations to find the bounds

  // Include depot
  try {
    const auto& depot = problem->getDepot();
    minX = std::min(minX, static_cast<float>(depot.x()));
    minY = std::min(minY, static_cast<float>(depot.y()));
    maxX = std::max(maxX, static_cast<float>(depot.x()));
    maxY = std::max(maxY, static_cast<float>(depot.y()));
    hasLocations = true;
  } catch (const std::exception&) {
    // Ignore if depot not found
  }

  // Include landfill
  try {
    const auto& landfill = problem->getLandfill();
    minX = std::min(minX, static_cast<float>(landfill.x()));
    minY = std::min(minY, static_cast<float>(landfill.y()));
    maxX = std::max(maxX, static_cast<float>(landfill.x()));
    maxY = std::max(maxY, static_cast<float>(landfill.y()));
    hasLocations = true;
  } catch (const std::exception&) {
    // Ignore if landfill not found
  }

  // Include SWTS locations
  try {
    const auto& swtsLocations = problem->getSWTS();
    for (const auto& swts : swtsLocations) {
      minX = std::min(minX, static_cast<float>(swts.x()));
      minY = std::min(minY, static_cast<float>(swts.y()));
      maxX = std::max(maxX, static_cast<float>(swts.x()));
      maxY = std::max(maxY, static_cast<float>(swts.y()));
      hasLocations = true;
    }
  } catch (const std::exception&) {
    // Ignore if SWTS not found
  }

  // Include zones
  try {
    const auto& zones = problem->getZones();
    for (const auto& zone : zones) {
      minX = std::min(minX, static_cast<float>(zone.x()));
      minY = std::min(minY, static_cast<float>(zone.y()));
      maxX = std::max(maxX, static_cast<float>(zone.x()));
      maxY = std::max(maxY, static_cast<float>(zone.y()));
      hasLocations = true;
    }
  } catch (const std::exception&) {
    // Ignore if zones not found
  }

  // If no locations found, return empty rectangle
  if (!hasLocations) {
    return Rectangle{0, 0, 0, 0};
  }

  // Convert to Rectangle format
  return Rectangle{minX, minY, maxX - minX, maxY - minY};
}

VRPTProblem* ProblemManager::getCurrentProblem() {
  if (!isProblemLoaded()) {
    return nullptr;
  }

  return problems_[current_problem_filename_].get();
}

bool ProblemManager::isProblemLoaded() const {
  return !current_problem_filename_.empty() &&
         problems_.find(current_problem_filename_) != problems_.end();
}

const std::vector<std::string>& ProblemManager::getAvailableProblemFiles() const {
  return available_problems_;
}

bool ProblemManager::hasValidAlgorithmSelection() const {
  return !selected_algorithm_.empty() && AlgorithmRegistry::exists(selected_algorithm_);
}

bool ProblemManager::isAlgorithmSelected() const {
  return !selected_algorithm_.empty() && AlgorithmRegistry::exists(selected_algorithm_);
}

bool ProblemManager::runAlgorithm() {
  if (!isAlgorithmSelected() || !isProblemLoaded()) {
    return false;
  }

  try {
    // Get the current problem
    VRPTProblem* problem = getCurrentProblem();
    if (!problem) {
      return false;
    }

    // If no algorithm instance exists or algorithm type changed, create a new one
    if (!current_algorithm_ || current_algorithm_type_ != selected_algorithm_) {
      current_algorithm_ =
        AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>(selected_algorithm_);
      current_algorithm_type_ = selected_algorithm_;
    }

    // Use the configured algorithm to solve the problem
    solution_ = std::make_unique<algorithm::VRPTSolution>(current_algorithm_->solve(*problem));
    has_solution_ = true;
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error running algorithm: " << e.what() << std::endl;
    return false;
  }
}

void ProblemManager::renderAlgorithmConfigurationUI() {
  if (!current_algorithm_) {
    if (!isAlgorithmSelected()) {
      return;
    }
    // Create the algorithm instance if it doesn't exist
    try {
      current_algorithm_ =
        AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>(selected_algorithm_);
      current_algorithm_type_ = selected_algorithm_;
    } catch (const std::exception& e) {
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", e.what());
      return;
    }
  }

  // Render the algorithm's configuration UI
  current_algorithm_->renderConfigurationUI();
}

}  // namespace visualization
}  // namespace daa
