#pragma once

#include <string>
#include <vector>

#include "algorithm_registry.h"
#include "algorithms/local_search/cv_local_search.h"
#include "algorithms/vrpt_solution.h"
#include "meta_heuristic_factory.h"
#include "problem/vrpt_problem.h"

namespace daa {
namespace algorithm {

/**
 * @brief 2-Opt local search for CV routes
 *
 * Reverses a segment of the route between two non-adjacent nodes
 * to potentially reduce travel distance/time
 */
class TwoOptSearch : public CVLocalSearch {
 public:
  /**
   * @brief Constructor with parameters
   * @param max_iterations Maximum number of iterations
   * @param first_improvement Whether to use first improvement
   */
  explicit TwoOptSearch(int max_iterations = 100, bool first_improvement = false)
      : CVLocalSearch(max_iterations, first_improvement) {}

  /**
   * @brief Search the 2-opt neighborhood
   */
  VRPTSolution searchNeighborhood(const VRPTProblem& problem, const VRPTSolution& current_solution)
    override {
    VRPTSolution best_solution = current_solution;
    size_t best_cv_count = best_solution.getCVCount();
    int max_cv_vehicles = problem.getNumCVVehicles();

    // Create a copy of the solution's CV routes to modify
    auto routes = current_solution.getCVRoutes();

    // Apply 2-opt to each route
    for (size_t route_idx = 0; route_idx < routes.size(); ++route_idx) {
      const auto& route = routes[route_idx];
      const auto& locations = route.locationIds();

      // Need at least 4 locations for 2-opt to make sense
      if (locations.size() < 4) {
        continue;
      }

      // Try each possible 2-opt swap
      for (size_t i = 0; i < locations.size() - 2; ++i) {
        for (size_t j = i + 2; j < locations.size(); ++j) {
          // Create a new solution with the 2-opt swap
          VRPTSolution new_solution = current_solution;
          auto& new_routes = new_solution.getCVRoutes();

          // Create new route with the segment reversed
          std::vector<std::string> new_locations;

          // Add locations before the reversed segment
          for (size_t k = 0; k <= i; ++k) {
            new_locations.push_back(locations[k]);
          }

          // Add reversed segment
          for (size_t k = j; k > i; --k) {
            new_locations.push_back(locations[k]);
          }

          // Add locations after the reversed segment
          for (size_t k = j + 1; k < locations.size(); ++k) {
            new_locations.push_back(locations[k]);
          }

          // Rebuild the route with the new sequence
          Capacity cv_capacity = problem.getCVCapacity();
          Duration cv_max_duration = problem.getCVMaxDuration();

          CVRoute new_route(route.vehicleId(), cv_capacity, cv_max_duration);
          for (const auto& loc_id : new_locations) {
            if (!new_route.canVisit(loc_id, problem)) {
              continue;
            }
            new_route.addLocation(loc_id, problem);
          }

          // Update the route
          new_routes[route_idx] = new_route;

          // Check if the new solution is better
          size_t new_cv_count = new_routes.size();

          bool is_better = false;

          // First constraint: Use at most the maximum number of allowed vehicles
          if (best_cv_count > static_cast<size_t>(max_cv_vehicles) &&
              new_cv_count <= static_cast<size_t>(max_cv_vehicles)) {
            // The new solution satisfies the max vehicles constraint while the current best doesn't
            is_better = true;
          } else if ((best_cv_count > static_cast<size_t>(max_cv_vehicles) &&
                      new_cv_count < best_cv_count) ||
                     (best_cv_count <= static_cast<size_t>(max_cv_vehicles) &&
                      new_cv_count < best_cv_count &&
                      new_cv_count <= static_cast<size_t>(max_cv_vehicles))) {
            // Either both violate the constraint but new uses fewer vehicles,
            // or both satisfy the constraint and new uses fewer vehicles
            is_better = true;
          }

          if (is_better) {
            best_solution = new_solution;
            best_cv_count = new_cv_count;

            if (first_improvement_) {
              return best_solution;
            }
          }
        }
      }
    }

    return best_solution;
  }

  std::string name() const override { return "2-Opt Search"; }
};

namespace {
inline static const bool TwoOptSearch_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerSearch<TwoOptSearch>("TwoOptSearch");
}

}  // namespace algorithm
}  // namespace daa
