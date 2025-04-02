#pragma once

#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
#include "meta_heuristic_components.h"
#include "meta_heuristic_factory.h"

namespace daa {
namespace algorithm {

/**
 * @brief Multi-Start metaheuristic for VRPT problem with RVND
 *
 * Generates multiple initial solutions using GRASP and
 * applies RVND (Random Variable Neighborhood Descent) to each,
 * returning the best solution found.
 */
class MultiStart : public TypedAlgorithm<VRPTProblem, VRPTSolution> {
 public:
  explicit MultiStart(
    int num_starts = 10,
    const std::string& generator_name = "GreedyCVGenerator",
    std::vector<std::string> search_names =
      {"TaskReinsertionSearch", "TaskExchangeSearch", "TwoOptSearch"}
  )
      : num_starts_(num_starts),
        generator_name_(generator_name),
        search_names_(std::move(search_names)) {
    // Initialize components
    initializeComponents();
  }

  VRPTSolution solve(const VRPTProblem& problem) override {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    // Create generator if needed
    if (!generator_) {
      generator_ = MetaFactory::createGenerator(generator_name_);
    }

    // Create local searches if needed
    if (local_searches_.empty() && !search_names_.empty()) {
      for (const auto& name : search_names_) {
        local_searches_.push_back(MetaFactory::createSearch(name));
      }
    }

    // Keep track of the best solution
    std::optional<VRPTSolution> best_solution;
    size_t best_cv_count = std::numeric_limits<size_t>::max();
    double best_total_duration = std::numeric_limits<double>::max();

    // Random number generator for RVND
    std::random_device rd;
    std::mt19937 gen(rd());

    // Perform multiple starts
    for (int i = 0; i < num_starts_; ++i) {
      // Generate an initial solution
      VRPTSolution current_solution = generator_->generateSolution(problem);

      // Apply Random VND if we have multiple neighborhoods
      if (!local_searches_.empty()) {
        bool improved = true;

        while (improved) {
          improved = false;

          // Create list of available neighborhoods
          std::vector<size_t> available_searches(local_searches_.size());
          std::iota(available_searches.begin(), available_searches.end(), 0);

          // Try neighborhoods in random order until no improvement
          while (!available_searches.empty() && !improved) {
            // Randomly select a neighborhood
            std::uniform_int_distribution<size_t> dist(0, available_searches.size() - 1);
            size_t idx = dist(gen);
            size_t search_idx = available_searches[idx];

            // Apply the selected neighborhood search
            VRPTSolution candidate_solution =
              local_searches_[search_idx]->improveSolution(problem, current_solution);

            // Check if solution improved
            size_t candidate_cv_count = candidate_solution.getCVCount();
            double candidate_duration = candidate_solution.totalDuration().value();

            bool is_better = false;
            if (candidate_cv_count < current_solution.getCVCount()) {
              is_better = true;
            } else if (candidate_cv_count == current_solution.getCVCount() &&
                       candidate_duration < current_solution.totalDuration().value()) {
              is_better = true;
            }

            if (is_better) {
              // Accept improvement
              current_solution = candidate_solution;
              improved = true;
            } else {
              // Remove this neighborhood from consideration
              available_searches.erase(available_searches.begin() + idx);
            }
          }
        }
      } else if (!search_names_.empty()) {
        // Fallback to single local search if neighborhoods couldn't be created
        try {
          auto single_search = MetaFactory::createSearch(search_names_[0]);
          current_solution = single_search->improveSolution(problem, current_solution);
        } catch (const std::exception&) {
          // Continue with unimproved solution
        }
      }

      // Calculate total duration for the improved solution
      double total_duration = current_solution.totalDuration().value();
      size_t cv_count = current_solution.getCVCount();

      // Check if this is the best solution so far
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
        best_solution = current_solution;
        best_cv_count = cv_count;
        best_total_duration = total_duration;
      }
    }

    return best_solution.value_or(generator_->generateSolution(problem));
  }

  std::string name() const override {
    return "Multi-Start-RVND(" + std::to_string(num_starts_) + ", " + generator_name_ + ", " +
           std::to_string(search_names_.size()) + " neighborhoods)";
  }

  std::string description() const override {
    return "Multi-Start metaheuristic with RVND that generates " + std::to_string(num_starts_) +
           " initial solutions using " + generator_name_ +
           " and improves each with Random VND using " + std::to_string(search_names_.size()) +
           " neighborhoods";
  }

  std::string timeComplexity() const override {
    return "O(n × k × m)";  // n = starts, k = neighborhoods, m = complexity of local search
  }

  void renderConfigurationUI() override;

 private:
  int num_starts_;
  std::string generator_name_;
  std::vector<std::string> search_names_;

  // Component instances for reuse
  std::unique_ptr<::meta::SolutionGenerator<VRPTSolution, VRPTProblem>> generator_;
  std::vector<std::unique_ptr<::meta::LocalSearch<VRPTSolution, VRPTProblem>>> local_searches_;

  // Map to track neighborhood search instances by name for UI configuration
  std::unordered_map<std::string, ::meta::LocalSearch<VRPTSolution, VRPTProblem>*> search_map_;

  // Initialize/update components when configuration changes
  void initializeComponents() {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    try {
      if (!generator_name_.empty()) {
        generator_ = MetaFactory::createGenerator(generator_name_);
      }

      // Create neighborhood searches
      local_searches_.clear();
      search_map_.clear();

      for (const auto& name : search_names_) {
        auto search = MetaFactory::createSearch(name);
        search_map_[name] = search.get();
        local_searches_.push_back(std::move(search));
      }
    } catch (const std::exception&) {
      // Initialization will be retried later if needed
    }
  }

  // Update neighborhoods when selection changes
  void updateSearches() {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    // Create a copy of the current search map to track which ones to keep
    std::unordered_map<std::string, ::meta::LocalSearch<VRPTSolution, VRPTProblem>*> existing_map =
      search_map_;

    // Clear current searches and rebuild
    std::vector<std::unique_ptr<::meta::LocalSearch<VRPTSolution, VRPTProblem>>> updated_searches;
    std::unordered_map<std::string, ::meta::LocalSearch<VRPTSolution, VRPTProblem>*>
      updated_search_map;

    // Process each selected search name
    for (const auto& name : search_names_) {
      auto it = existing_map.find(name);

      if (it != existing_map.end()) {
        // This search already exists, reuse it
        for (auto& search : local_searches_) {
          if (search.get() == it->second) {
            // Found it, move to updated list
            updated_search_map[name] = search.get();
            updated_searches.push_back(std::move(search));
            break;
          }
        }
        // Remove from existing map to mark as processed
        existing_map.erase(name);
      } else {
        // This is a new search, create it
        try {
          auto search = MetaFactory::createSearch(name);
          updated_search_map[name] = search.get();
          updated_searches.push_back(std::move(search));
        } catch (const std::exception&) {
          // Skip if creation fails
        }
      }
    }

    // Update the member variables
    local_searches_ = std::move(updated_searches);
    search_map_ = std::move(updated_search_map);
  }
};

// Register the algorithm with default parameters
REGISTER_ALGORITHM(MultiStart, "MultiStart-RVND");

}  // namespace algorithm
}  // namespace daa
