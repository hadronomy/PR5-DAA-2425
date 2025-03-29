#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "problem/vrpt_problem.h"
#include "raylib.h"

namespace daa {
namespace visualization {

class ProblemManager {
 public:
  ProblemManager();

  // Define a callback type for problem loading notification
  using ProblemLoadedCallback = std::function<void(const Rectangle&)>;

  // Set callback to be called when a problem is loaded
  void SetProblemLoadedCallback(ProblemLoadedCallback callback) {
    problem_loaded_callback_ = callback;
  }

  // Scan a directory for problem files
  bool scanDirectory(const std::string& dir_path);

  // Load a problem by filename
  bool loadProblem(const std::string& filename);

  // Get the currently active problem
  VRPTProblem* getCurrentProblem();

  // Get the current problem filename
  const std::string& getCurrentProblemFilename() const { return current_problem_filename_; }

  // Check if a problem is loaded
  bool isProblemLoaded() const;

  // Get list of available problems
  const std::vector<std::string>& getAvailableProblemFiles() const;

  // Get available algorithms
  std::vector<std::string> getAvailableGenerators() const;
  std::vector<std::string> getAvailableSearches() const;

  // Set selected algorithms
  void setSelectedGenerator(const std::string& name);
  void setSelectedSearch(const std::string& name);

  // Get selected algorithms
  const std::string& getSelectedGenerator() const { return selected_generator_; }
  const std::string& getSelectedSearch() const { return selected_search_; }

  // Check if algorithm selection is valid
  bool hasValidAlgorithmSelection() const;

  // Run selected algorithm with current problem
  bool runAlgorithm();

  // Get the bounds of the current problem
  Rectangle GetCurrentProblemBounds() const;

 private:
  // Map of filename to problem instance
  std::unordered_map<std::string, std::unique_ptr<VRPTProblem>> problems_;

  // Currently loaded problem
  std::string current_problem_filename_;

  // List of available problem files
  std::vector<std::string> available_problems_;

  // Selected algorithms
  std::string selected_generator_;
  std::string selected_search_;

  // Callback to notify when a problem is loaded
  ProblemLoadedCallback problem_loaded_callback_;
};

}  // namespace visualization
}  // namespace daa
