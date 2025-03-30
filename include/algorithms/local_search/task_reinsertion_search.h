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
 * @brief Task Reinsertion local search for CV routes
 *
 * Tries to move a collection zone from its current position in one route
 * to a different position (either in the same route or a different route)
 */
class TaskReinsertionSearch : public CVLocalSearch {
 public:
  /**
   * @brief Constructor with parameters
   * @param max_iterations Maximum number of iterations
   * @param first_improvement Whether to use first improvement
   */
  explicit TaskReinsertionSearch(int max_iterations = 100, bool first_improvement = false)
      : CVLocalSearch(max_iterations, first_improvement) {}

  /**
   * @brief Search the reinsertion neighborhood
   */
  VRPTSolution searchNeighborhood(const VRPTProblem& problem, const VRPTSolution& current_solution)
    override {
    VRPTSolution best_solution = current_solution;
    size_t best_cv_count = best_solution.getCVCount();
    int max_cv_vehicles = problem.getNumCVVehicles();

    // Create a copy of the solution's CV routes to modify
    auto routes = current_solution.getCVRoutes();

    // Try to move each collection zone to a different position
    for (size_t r1_idx = 0; r1_idx < routes.size(); ++r1_idx) {
      const auto& r1 = routes[r1_idx];
      const auto& locations = r1.locationIds();

      // Check each location in the route
      for (size_t pos1 = 0; pos1 < locations.size(); ++pos1) {
        const std::string& location_id = locations[pos1];
        const auto& location = problem.getLocation(location_id);

        // Only consider collection zones (not SWTS or depot)
        if (location.type() != LocationType::COLLECTION_ZONE) {
          continue;
        }

        // Try to move this zone to every possible position in every route
        for (size_t r2_idx = 0; r2_idx < routes.size(); ++r2_idx) {
          // Skip if it's the same route and there's only one location
          if (r1_idx == r2_idx && locations.size() <= 1) {
            continue;
          }

          const auto& r2 = routes[r2_idx];
          const auto& target_locations = r2.locationIds();

          // Try each possible insertion position
          for (size_t pos2 = 0; pos2 <= target_locations.size(); ++pos2) {
            // Skip if trying to insert at the same position in the same route
            if (r1_idx == r2_idx && (pos2 == pos1 || pos2 == pos1 + 1)) {
              continue;
            }

            // Create a new solution with the move
            VRPTSolution new_solution = current_solution;
            auto& new_routes = new_solution.getCVRoutes();

            // Remove the zone from its original route
            std::vector<std::string> new_r1_locations;
            for (size_t i = 0; i < locations.size(); ++i) {
              if (i != pos1) {
                new_r1_locations.push_back(locations[i]);
              }
            }

            // Insert the zone into the target route
            std::vector<std::string> new_r2_locations;
            for (size_t i = 0; i < target_locations.size(); ++i) {
              if (i == pos2) {
                new_r2_locations.push_back(location_id);
              }
              new_r2_locations.push_back(target_locations[i]);
            }

            // Handle insertion at the end
            if (pos2 == target_locations.size()) {
              new_r2_locations.push_back(location_id);
            }

            // Rebuild the routes with the new location sequences
            Capacity cv_capacity = problem.getCVCapacity();
            Duration cv_max_duration = problem.getCVMaxDuration();

            // Create new routes
            CVRoute new_r1(r1.vehicleId(), cv_capacity, cv_max_duration);
            for (const auto& loc_id : new_r1_locations) {
              if (!new_r1.canVisit(loc_id, problem)) {
                // Skip this move if it's not feasible
                continue;
              }
              new_r1.addLocation(loc_id, problem);
            }

            CVRoute new_r2(r2.vehicleId(), cv_capacity, cv_max_duration);
            for (const auto& loc_id : new_r2_locations) {
              if (!new_r2.canVisit(loc_id, problem)) {
                continue;
              }
              new_r2.addLocation(loc_id, problem);
            }

            // Update the routes in the new solution
            if (r1_idx == r2_idx) {
              // Same route case
              new_routes[r1_idx] = new_r2;
            } else {
              // Different routes case
              new_routes[r1_idx] = new_r1;
              new_routes[r2_idx] = new_r2;
            }

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

  std::string name() const override { return "Task Reinsertion Search"; }
};

namespace {
inline static const bool TaskReinsertionSearch_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerSearch<TaskReinsertionSearch>("TaskReinsertionSearch");
}

}  // namespace algorithm
}  // namespace daa
