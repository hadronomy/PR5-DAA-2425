#pragma once

#include <memory>
#include <string>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
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
      : num_starts_(num_starts), generator_name_(generator_name), search_name_(search_name) {}

  VRPTSolution solve(const VRPTProblem& problem) override {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    // Create components
    auto generator = MetaFactory::createGenerator(generator_name_);
    auto local_search = MetaFactory::createSearch(search_name_);

    // Keep track of the best solution
    std::optional<VRPTSolution> best_solution;
    size_t best_cv_count = std::numeric_limits<size_t>::max();

    // Perform multiple starts
    for (int i = 0; i < num_starts_; ++i) {
      // Generate an initial solution
      VRPTSolution initial_solution = generator->generateSolution(problem);

      // Apply local search
      VRPTSolution improved_solution = local_search->improveSolution(problem, initial_solution);

      // Check if this is the best solution so far
      size_t cv_count = improved_solution.getCVCount();
      if (!best_solution || cv_count < best_cv_count) {
        best_solution = improved_solution;
        best_cv_count = cv_count;
      }
    }

    return best_solution.value_or(generator->generateSolution(problem));
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
};

// Register the algorithm with default parameters
REGISTER_ALGORITHM(MultiStart, "MultiStart");

}  // namespace algorithm
}  // namespace daa
