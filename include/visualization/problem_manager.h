#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "algorithms/vrpt_solution.h"
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

  // Set selected algorithm
  void setSelectedAlgorithm(const std::string& name);

  // Get selected algorithm
  const std::string& getSelectedAlgorithm() const { return selected_algorithm_; }

  // Check if algorithm selection is valid
  bool hasValidAlgorithmSelection() const;

  // Run selected algorithm with current problem
  bool runAlgorithm();

  // Get the bounds of the current problem
  Rectangle GetCurrentProblemBounds() const;

  // Solution-related methods
  bool hasSolution() const { return solution_ != nullptr; }
  const algorithm::VRPTSolution* getSolution() const { return solution_.get(); }
  void clearSolution() { solution_.reset(); }

  // New algorithm configuration getters/setters
  void setSelectedGenerator(const std::string& generator) { selected_generator_ = generator; }
  const std::string& getSelectedGenerator() const { return selected_generator_; }

  void setSelectedSearches(const std::vector<std::string>& searches) {
    selected_searches_ = searches;
  }
  const std::vector<std::string>& getSelectedSearches() const { return selected_searches_; }

  void setIterationsOrStarts(int value) { iterations_or_starts_ = value; }
  int getIterationsOrStarts() const { return iterations_or_starts_; }

 private:
  // Map of filename to problem instance
  std::unordered_map<std::string, std::unique_ptr<VRPTProblem>> problems_;

  // Currently loaded problem
  std::string current_problem_filename_;

  // List of available problem files
  std::vector<std::string> available_problems_;

  // Selected algorithm
  std::string selected_algorithm_;

  // Current solution
  std::unique_ptr<algorithm::VRPTSolution> solution_;

  // Callback to notify when a problem is loaded
  ProblemLoadedCallback problem_loaded_callback_;

  // Algorithm configuration
  std::string selected_generator_;
  std::vector<std::string> selected_searches_;
  int iterations_or_starts_ = 10;
};

}  // namespace visualization
}  // namespace daa
