#pragma once

#include "algorithms/vrpt_solution.h"
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
    VRPTSolution current_solution = initial_solution;
    VRPTSolution best_solution = current_solution;
    size_t best_cv_count = best_solution.getCVCount();
    int max_cv_vehicles = problem.getNumCVVehicles();

    bool improved;
    int iteration = 0;

    do {
      improved = false;

      // Apply neighborhood search
      VRPTSolution neighbor_solution = searchNeighborhood(problem, current_solution);
      size_t neighbor_cv_count = neighbor_solution.getCVCount();

      // Check if the neighbor is better
      bool is_better = false;

      // First constraint: Use at most the maximum number of allowed vehicles
      if (best_cv_count > static_cast<size_t>(max_cv_vehicles) &&
          neighbor_cv_count <= static_cast<size_t>(max_cv_vehicles)) {
        // The new solution satisfies the max vehicles constraint while the current best doesn't
        is_better = true;
      } else if ((best_cv_count > static_cast<size_t>(max_cv_vehicles) &&
                  neighbor_cv_count < best_cv_count) ||
                 (best_cv_count <= static_cast<size_t>(max_cv_vehicles) &&
                  neighbor_cv_count < best_cv_count &&
                  neighbor_cv_count <= static_cast<size_t>(max_cv_vehicles))) {
        // Either both violate the constraint but neighbor uses fewer vehicles,
        // or both satisfy the constraint and neighbor uses fewer vehicles
        is_better = true;
      }

      if (is_better) {
        best_solution = neighbor_solution;
        best_cv_count = neighbor_cv_count;
        current_solution = neighbor_solution;
        improved = true;
      }

      iteration++;
    } while (improved && iteration < max_iterations_);

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

 protected:
  int max_iterations_ = 100;
  bool first_improvement_ = false;
};

}  // namespace algorithm
}  // namespace daa
