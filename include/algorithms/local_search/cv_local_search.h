#pragma once

#include <unordered_set>
#include "algorithms/vrpt_solution.h"
#include "imgui.h"
#include "meta_heuristic_components.h"
#include "problem/vrpt_problem.h"

namespace daa {
namespace algorithm {

/**
 * @brief Base class for Collection Vehicle route local search algorithms
 */
class CVLocalSearch : public ::meta::LocalSearch<VRPTSolution, VRPTProblem> {
 public:
  /**
   * @brief Default constructor
   */
  CVLocalSearch() = default;

  /**
   * @brief Constructor with local search parameters
   * @param max_iterations Maximum number of iterations to perform
   * @param first_improvement Whether to use first improvement (true) or best improvement (false)
   */
  explicit CVLocalSearch(int max_iterations = 100, bool first_improvement = false)
      : max_iterations_(max_iterations), first_improvement_(first_improvement) {}

  /**
   * @brief Improve an existing solution using local search
   * @param problem The problem instance
   * @param initial_solution The initial solution to improve
   * @return The improved solution
   */
  VRPTSolution improveSolution(const VRPTProblem& problem, const VRPTSolution& initial_solution)
    override {
    auto current_solution = initial_solution;
    auto best_solution = current_solution;

    // Store solution metrics for comparison
    struct SolutionMetrics {
      size_t cv_count;
      size_t zones_count;
      double total_duration;
    };

    auto getSolutionMetrics = [&problem](const VRPTSolution& solution) -> SolutionMetrics {
      return {
        .cv_count = solution.getCVCount(),
        .zones_count = solution.visitedZones(problem),
        .total_duration = solution.totalDuration().value()
      };
    };

    const auto max_cv_vehicles = problem.getNumCVVehicles();
    auto best_metrics = getSolutionMetrics(best_solution);

    auto is_better_solution =
      [max_cv_vehicles](const SolutionMetrics& current, const SolutionMetrics& candidate) -> bool {
      // First constraint: Use at most the maximum number of allowed vehicles
      if (current.cv_count > static_cast<size_t>(max_cv_vehicles) &&
          candidate.cv_count <= static_cast<size_t>(max_cv_vehicles)) {
        return true;  // Candidate satisfies vehicle constraint while current doesn't
      }

      if ((current.cv_count > static_cast<size_t>(max_cv_vehicles) &&
           candidate.cv_count < current.cv_count) ||
          (current.cv_count <= static_cast<size_t>(max_cv_vehicles) &&
           candidate.cv_count < current.cv_count &&
           candidate.cv_count <= static_cast<size_t>(max_cv_vehicles))) {
        return true;  // Fewer vehicles while respecting constraints
      }

      if (current.cv_count == candidate.cv_count) {
        // Same number of vehicles, check zones count
        if (candidate.zones_count > current.zones_count) {
          return true;  // More zones visited
        }

        if (candidate.zones_count == current.zones_count &&
            candidate.total_duration < current.total_duration) {
          return true;  // Same zones with better duration
        }
      }

      return false;
    };

    for (int iteration = 0; iteration < max_iterations_; ++iteration) {
      auto neighbor_solution = searchNeighborhood(problem, current_solution);
      auto neighbor_metrics = getSolutionMetrics(neighbor_solution);

      if (is_better_solution(best_metrics, neighbor_metrics)) {
        best_solution = neighbor_solution;
        best_metrics = neighbor_metrics;
        current_solution = std::move(neighbor_solution);
      } else {
        // No improvement found
        break;
      }
    }

    return best_solution;
  }

  /**
   * @brief Search the neighborhood for an improved solution
   * @param problem The problem instance
   * @param current_solution The current solution
   * @return The best neighboring solution found
   */
  virtual VRPTSolution
    searchNeighborhood(const VRPTProblem& problem, const VRPTSolution& current_solution) = 0;

  /**
   * @brief Render UI elements for configuring this local search algorithm
   */
  void renderConfigurationUI() override {
    ImGui::SliderInt("Max Iterations", &max_iterations_, 1, 1000);
    ImGui::Checkbox("First Improvement", &first_improvement_);
  }

 protected:
  int max_iterations_ = 100;
  bool first_improvement_ = false;
};

}  // namespace algorithm
}  // namespace daa
