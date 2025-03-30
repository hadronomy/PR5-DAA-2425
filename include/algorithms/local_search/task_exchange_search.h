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
 * @brief Task Exchange local search for CV routes
 *
 * Swaps the positions of two collection zones (within the same route or between two different
 * routes)
 */
class TaskExchangeSearch : public CVLocalSearch {
 public:
  /**
   * @brief Constructor with parameters
   * @param max_iterations Maximum number of iterations
   * @param first_improvement Whether to use first improvement
   */
  explicit TaskExchangeSearch(int max_iterations = 100, bool first_improvement = false)
      : CVLocalSearch(max_iterations, first_improvement) {}

  /**
   * @brief Search the exchange neighborhood
   */
  VRPTSolution searchNeighborhood(const VRPTProblem& problem, const VRPTSolution& current_solution)
    override {
    VRPTSolution best_solution = current_solution;
    size_t best_cv_count = best_solution.getCVCount();
    int max_cv_vehicles = problem.getNumCVVehicles();

    // Create a copy of the solution's CV routes to modify
    auto routes = current_solution.getCVRoutes();

    // Try to swap each pair of collection zones
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

        // Find another zone to swap with
        for (size_t r2_idx = r1_idx; r2_idx < routes.size(); ++r2_idx) {
          const auto& r2 = routes[r2_idx];
          const auto& locations2 = r2.locationIds();

          size_t start_pos2 = (r1_idx == r2_idx) ? pos1 + 1 : 0;
          for (size_t pos2 = start_pos2; pos2 < locations2.size(); ++pos2) {
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

            if (r1_idx == r2_idx) {
              // Swap within same route
              std::swap(new_r1_locations[pos1], new_r1_locations[pos2]);
            } else {
              // Swap between different routes
              new_r1_locations[pos1] = location_id2;
              new_r2_locations[pos2] = location_id1;
            }

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

            if (r1_idx != r2_idx) {
              CVRoute new_r2(r2.vehicleId(), cv_capacity, cv_max_duration);
              for (const auto& loc_id : new_r2_locations) {
                if (!new_r2.canVisit(loc_id, problem)) {
                  continue;
                }
                new_r2.addLocation(loc_id, problem);
              }

              // Update the second route
              new_routes[r2_idx] = new_r2;
            }

            // Update the first route
            new_routes[r1_idx] = new_r1;

            // Remove empty routes
            new_routes.erase(
              std::remove_if(
                new_routes.begin(), new_routes.end(), [](const CVRoute& r) { return r.isEmpty(); }
              ),
              new_routes.end()
            );

            // Check if the new solution is better
            size_t new_cv_count = new_routes.size();

            bool is_better = false;

            // First constraint: Use at most the maximum number of allowed vehicles
            if (best_cv_count > static_cast<size_t>(max_cv_vehicles) &&
                new_cv_count <= static_cast<size_t>(max_cv_vehicles)) {
              // The new solution satisfies the max vehicles constraint while the current best
              // doesn't
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
    }

    return best_solution;
  }

  std::string name() const override { return "Task Exchange Search"; }
};

namespace {
inline static const bool TaskExchangeSearch_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerSearch<TaskExchangeSearch>("TaskExchangeSearch");
}

}  // namespace algorithm
}  // namespace daa
