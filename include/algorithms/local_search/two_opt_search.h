#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
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
    size_t original_cv_count = current_solution.getCVCount();

    // Calculate total duration of the current best solution
    Duration best_total_duration = best_solution.totalDuration();

    // Count zones visited in the initial solution
    size_t original_zones_count = best_solution.visitedZones(problem);

    // Apply 2-opt to each route
    for (size_t route_idx = 0; route_idx < current_solution.getCVRoutes().size(); ++route_idx) {
      const auto& route = current_solution.getCVRoutes()[route_idx];
      const auto& locations = route.locationIds();

      // Need at least 4 locations for 2-opt to make sense
      if (locations.size() < 4)
        continue;

      // Use the helper function to iterate through all valid 2-opt swap pairs
      forEach2OptPair(locations.size(), [&](size_t i, size_t j) {
        // Create a new solution with the 2-opt swap
        VRPTSolution new_solution = current_solution;
        auto& new_routes = new_solution.getCVRoutes();

        // Create the new sequence with the reversed segment using std::reverse
        std::vector<std::string> new_locations(locations);
        std::reverse(new_locations.begin() + i + 1, new_locations.begin() + j + 1);

        // Rebuild the route with the new sequence
        CVRoute new_route(route.vehicleId(), problem.getCVCapacity(), problem.getCVMaxDuration());

        for (const auto& loc_id : new_locations) {
          if (new_route.canVisit(loc_id, problem)) {
            new_route.addLocation(loc_id, problem);
          }
        }

        // Only proceed if the new route is valid:
        // 1. Final load must be 0 (all waste delivered)
        // 2. Route must end at depot
        if (new_route.currentLoad().value() != 0.0 ||
            (new_route.lastLocationId() != problem.getDepot().id())) {
          return;  // Skip invalid routes
        }

        // Update the route
        new_routes[route_idx] = new_route;

        // Check if the new solution is better
        size_t new_cv_count = new_solution.getCVCount();
        Duration new_total_duration = new_solution.totalDuration();
        size_t new_zones_count = new_solution.visitedZones(problem);

        // Check if solution is better (with simplified evaluation logic)
        if (new_cv_count <= original_cv_count && new_zones_count >= original_zones_count &&
            new_total_duration < best_total_duration) {
          best_solution = new_solution;
          best_total_duration = new_total_duration;
          original_zones_count = new_zones_count;

          if (first_improvement_) {
            return;  // Early return from lambda to exit the search
          }
        }
      });

      // If first_improvement_ and we found an improvement, the lambda would have returned
      if (first_improvement_ && best_solution != current_solution) {
        break;
      }
    }

    return best_solution;
  }

  std::string name() const override { return "2-Opt Search"; }

 private:
  /**
   * @brief Iterates through all valid 2-opt swap index pairs using C++20 ranges
   * @param size The size of the route
   * @param callback Function to call for each valid (i,j) pair
   */
  template <typename Callback>
  requires std::invocable<Callback, size_t, size_t> void
    forEach2OptPair(size_t size, Callback callback) {
    // Generate all index pairs (i,j) where i < j-1 (non-adjacent)
    auto indices = std::views::iota(0u, size);
    for (const auto i : indices | std::views::take(size - 2)) {
      for (const auto j : std::views::iota(i + 2, size)) {
        callback(i, j);
      }
    }
  }
};

namespace {
inline static const bool TwoOptSearch_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerSearch<TwoOptSearch>("TwoOptSearch");
}

}  // namespace algorithm
}  // namespace daa
