#pragma once

#include <memory>
#include <string>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
#include "meta_heuristic_components.h"
#include "meta_heuristic_factory.h"

namespace daa {
namespace algorithm {

/**
 * @brief Multi-Start metaheuristic for VRPT problem
 *
 * Generates multiple initial solutions using GRASP and
 * applies local search to each, returning the best solution found.
 */
class MultiStart : public TypedAlgorithm<VRPTProblem, VRPTSolution> {
 public:
  explicit MultiStart(
    int num_starts = 10,
    const std::string& generator_name = "GreedyCVGenerator",
    const std::string& search_name = "TaskReinsertionSearch"
  )
      : num_starts_(num_starts), generator_name_(generator_name), search_name_(search_name) {
    // Initialize components
    initializeComponents();
  }

  VRPTSolution solve(const VRPTProblem& problem) override {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    // Create components if needed
    if (!generator_ || !local_search_) {
      generator_ = MetaFactory::createGenerator(generator_name_);
      local_search_ = MetaFactory::createSearch(search_name_);
    }

    // Keep track of the best solution
    std::optional<VRPTSolution> best_solution;
    size_t best_cv_count = std::numeric_limits<size_t>::max();
    double best_total_duration = std::numeric_limits<double>::max();

    // Perform multiple starts
    for (int i = 0; i < num_starts_; ++i) {
      // Generate an initial solution
      VRPTSolution initial_solution = generator_->generateSolution(problem);

      // Apply local search
      VRPTSolution improved_solution = local_search_->improveSolution(problem, initial_solution);

      // Calculate total duration for the improved solution
      double total_duration = improved_solution.totalDuration().value();

      // Check if this is the best solution so far
      size_t cv_count = improved_solution.getCVCount();

      bool is_better = false;
      if (!best_solution) {
        // First solution found
        is_better = true;
      } else if (cv_count < best_cv_count) {
        // Better primary criterion: fewer vehicles
        is_better = true;
      } else if (cv_count == best_cv_count && total_duration < best_total_duration) {
        // Same number of vehicles but better secondary criterion: lower duration
        is_better = true;
      }

      if (is_better) {
        best_solution = improved_solution;
        best_cv_count = cv_count;
        best_total_duration = total_duration;
      }
    }

    return best_solution.value_or(generator_->generateSolution(problem));
  }

  std::string name() const override {
    return "Multi-Start(" + std::to_string(num_starts_) + ", " + generator_name_ + ", " +
           search_name_ + ")";
  }

  std::string description() const override {
    return "Multi-Start metaheuristic that generates " + std::to_string(num_starts_) +
           " initial solutions using " + generator_name_ + " and improves each with " +
           search_name_;
  }

  std::string timeComplexity() const override {
    return "O(n × m)";  // n = number of starts, m = complexity of local search
  }

  void renderConfigurationUI() override;

 private:
  int num_starts_;
  std::string generator_name_;
  std::string search_name_;

  // Component instances for reuse
  std::unique_ptr<::meta::SolutionGenerator<VRPTSolution, VRPTProblem>> generator_;
  std::unique_ptr<::meta::LocalSearch<VRPTSolution, VRPTProblem>> local_search_;

  // Initialize/update components when configuration changes
  void initializeComponents() {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    try {
      if (!generator_name_.empty()) {
        generator_ = MetaFactory::createGenerator(generator_name_);
      }

      if (!search_name_.empty()) {
        local_search_ = MetaFactory::createSearch(search_name_);
      }
    } catch (const std::exception&) {
      // Initialization will be retried later if needed
    }
  }
};

// Register the algorithm with default parameters
REGISTER_ALGORITHM(MultiStart, "MultiStart");

}  // namespace algorithm
}  // namespace daa
