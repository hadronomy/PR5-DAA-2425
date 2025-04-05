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
 * @brief Task Exchange Between Routes local search for CV routes
 *
 * Swaps the positions of two collection zones between two different routes only.
 * This is a specialized version of TaskExchangeSearch that only considers
 * exchanges between different routes.
 */
class TaskExchangeBetweenRoutesSearch : public CVLocalSearch {
 public:
  /**
   * @brief Constructor with parameters
   * @param max_iterations Maximum number of iterations
   * @param first_improvement Whether to use first improvement
   */
  explicit TaskExchangeBetweenRoutesSearch(int max_iterations = 100, bool first_improvement = false)
      : CVLocalSearch(max_iterations, first_improvement) {}

  /**
   * @brief Search the exchange neighborhood (between routes only)
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

    // Need at least 2 routes to perform between-route exchanges
    if (routes.size() < 2) {
      return best_solution;
    }

    // Try to swap each pair of collection zones between different routes
    for (size_t r1_idx = 0; r1_idx < routes.size(); ++r1_idx) {
      const auto& r1 = routes[r1_idx];
      const auto& locations1 = r1.locationIds();

      for (size_t pos1 = 0; pos1 < locations1.size(); ++pos1) {
        const std::string& location_id1 = locations1[pos1];
        const auto& location1 = problem.getLocation(location_id1);

        // Only consider collection zones (not SWTS or depot)
        if (location1.type() != LocationType::COLLECTION_ZONE) {
          continue;
        }

        // Find another zone in a different route to swap with
        for (size_t r2_idx = r1_idx + 1; r2_idx < routes.size(); ++r2_idx) {
          const auto& r2 = routes[r2_idx];
          const auto& locations2 = r2.locationIds();

          for (size_t pos2 = 0; pos2 < locations2.size(); ++pos2) {
            const std::string& location_id2 = locations2[pos2];
            const auto& location2 = problem.getLocation(location_id2);

            // Only consider collection zones
            if (location2.type() != LocationType::COLLECTION_ZONE) {
              continue;
            }

            // Create a new solution with the swap
            VRPTSolution new_solution = current_solution;
            auto& new_routes = new_solution.getCVRoutes();

            // Create new route sequences with the swap
            std::vector<std::string> new_r1_locations = locations1;
            std::vector<std::string> new_r2_locations = locations2;

            // Swap between different routes
            new_r1_locations[pos1] = location_id2;
            new_r2_locations[pos2] = location_id1;

            // Rebuild the routes with the new sequences
            Capacity cv_capacity = problem.getCVCapacity();
            Duration cv_max_duration = problem.getCVMaxDuration();

            // Create new routes
            CVRoute new_r1(r1.vehicleId(), cv_capacity, cv_max_duration);
            for (const auto& loc_id : new_r1_locations) {
              if (!new_r1.canVisit(loc_id, problem)) {
                continue;
              }
              new_r1.addLocation(loc_id, problem);
            }

            // Check if the first route ends at depot and has 0 load
            if (new_r1.currentLoad().value() != 0.0 ||
                (new_r1.lastLocationId() != problem.getDepot().id())) {
              continue;  // Skip invalid routes
            }

            CVRoute new_r2(r2.vehicleId(), cv_capacity, cv_max_duration);
            for (const auto& loc_id : new_r2_locations) {
              if (!new_r2.canVisit(loc_id, problem)) {
                continue;
              }
              new_r2.addLocation(loc_id, problem);
            }

            // Check if the second route ends at depot and has 0 load
            if (new_r2.currentLoad().value() != 0.0 ||
                (new_r2.lastLocationId() != problem.getDepot().id())) {
              continue;  // Skip invalid routes
            }

            // Update the routes in the new solution
            new_routes[r1_idx] = new_r1;
            new_routes[r2_idx] = new_r2;

            // Remove empty routes
            new_routes.erase(
              std::remove_if(
                new_routes.begin(), new_routes.end(), [](const CVRoute& r) { return r.isEmpty(); }
              ),
              new_routes.end()
            );

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
    }

    return best_solution;
  }

  std::string name() const override { return "Task Exchange Between Routes Search"; }
};

namespace {
inline static const bool TaskExchangeBetweenRoutesSearch_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerSearch<TaskExchangeBetweenRoutesSearch>("TaskExchangeBetweenRoutesSearch");
}

}  // namespace algorithm
}  // namespace daa
