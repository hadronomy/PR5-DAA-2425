#pragma once

#include <algorithm>
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
 * @brief Task Exchange Within Route local search for CV routes
 *
 * Swaps the positions of two collection zones within the same route only.
 * This is a specialized version of TaskExchangeSearch that only considers
 * exchanges within the same route.
 */
class TaskExchangeWithinRouteSearch : public CVLocalSearch {
 public:
  /**
   * @brief Constructor with parameters
   * @param max_iterations Maximum number of iterations
   * @param first_improvement Whether to use first improvement
   */
  explicit TaskExchangeWithinRouteSearch(int max_iterations = 100, bool first_improvement = false)
      : CVLocalSearch(max_iterations, first_improvement) {}

  /**
   * @brief Search the exchange neighborhood (within route only)
   */
  VRPTSolution searchNeighborhood(const VRPTProblem& problem, const VRPTSolution& current_solution)
    override {
    VRPTSolution best_solution = current_solution;
    size_t best_cv_count = best_solution.getCVCount();
    int max_cv_vehicles = problem.getNumCVVehicles();

    // Track current solution's metrics for comparison
    size_t original_zones_count = current_solution.visitedZones(problem);
    Duration best_total_duration = best_solution.totalDuration();

    // Create a copy of the solution's CV routes to modify
    auto routes = current_solution.getCVRoutes();

    // Try to swap each pair of collection zones within the same route
    for (size_t r_idx = 0; r_idx < routes.size(); ++r_idx) {
      const auto& route = routes[r_idx];
      const auto& locations = route.locationIds();

      // Need at least 2 locations to perform a swap
      if (locations.size() < 2) {
        continue;
      }

      for (size_t pos1 = 0; pos1 < locations.size(); ++pos1) {
        const std::string& location_id1 = locations[pos1];
        const auto& location1 = problem.getLocation(location_id1);

        // Only consider collection zones (not SWTS or depot)
        if (location1.type() != LocationType::COLLECTION_ZONE) {
          continue;
        }

        // Find another zone in the same route to swap with
        for (size_t pos2 = pos1 + 1; pos2 < locations.size(); ++pos2) {
          const std::string& location_id2 = locations[pos2];
          const auto& location2 = problem.getLocation(location_id2);

          // Only consider collection zones
          if (location2.type() != LocationType::COLLECTION_ZONE) {
            continue;
          }

          // Create a new solution with the swap
          VRPTSolution new_solution = current_solution;
          auto& new_routes = new_solution.getCVRoutes();

          // Create new route sequence with the swap
          std::vector<std::string> new_locations = locations;
          std::swap(new_locations[pos1], new_locations[pos2]);

          // Rebuild the route with the new sequence
          Capacity cv_capacity = problem.getCVCapacity();
          Duration cv_max_duration = problem.getCVMaxDuration();

          // Create new route
          CVRoute new_route(route.vehicleId(), cv_capacity, cv_max_duration);
          for (const auto& loc_id : new_locations) {
            if (!new_route.canVisit(loc_id, problem)) {
              continue;
            }
            new_route.addLocation(loc_id, problem);
          }

          // Check if the route ends at depot and has 0 load
          if (new_route.currentLoad().value() != 0.0 ||
              (new_route.lastLocationId() != problem.getDepot().id())) {
            continue;  // Skip invalid routes
          }

          // Update the route in the new solution
          new_routes[r_idx] = new_route;

          // Check if the new solution is better
          size_t new_cv_count = new_routes.size();
          size_t new_zones_count = new_solution.visitedZones(problem);
          Duration new_total_duration = new_solution.totalDuration();

          bool is_better = false;

          // New solution is better if:
          // 1. It uses fewer or equal number of vehicles (never more)
          // 2. It visits at least the same number of zones
          // 3. It has shorter total duration
          if (new_cv_count <= best_cv_count && new_zones_count >= original_zones_count &&
              new_total_duration < best_total_duration) {
            is_better = true;
          }

          if (is_better) {
            best_solution = new_solution;
            best_cv_count = new_cv_count;
            best_total_duration = new_total_duration;

            if (first_improvement_) {
              return best_solution;
            }
          }
        }
      }
    }

    return best_solution;
  }

  std::string name() const override { return "Task Exchange Within Route Search"; }
};

namespace {
inline static const bool TaskExchangeWithinRouteSearch_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerSearch<TaskExchangeWithinRouteSearch>("TaskExchangeWithinRouteSearch");
}

}  // namespace algorithm
}  // namespace daa
