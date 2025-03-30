#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
#include "meta_heuristic_components.h"
#include "meta_heuristic_factory.h"
#include "problem/vrpt_problem.h"

namespace daa {
namespace algorithm {

/**
 * @brief Greedy constructive heuristic for Collection Vehicle routes
 *
 * This implements Algorithm 1 from the paper as described in the summary,
 * using a greedy nearest-neighbor approach to build Collection Vehicle routes.
 */
class GreedyCVGenerator : public ::meta::SolutionGenerator<VRPTSolution, VRPTProblem> {
 public:
  /**
   * @brief Generate initial solution using greedy approach
   * @param problem The problem instance
   * @return A solution with CV routes
   */
  VRPTSolution generateSolution(const VRPTProblem& problem) override {
    VRPTSolution solution;

    // Get all collection zones
    std::vector<Location> zones = problem.getZones();
    std::unordered_set<std::string> unassigned_zones;
    for (const auto& zone : zones) {
      unassigned_zones.insert(zone.id());
    }

    // Vehicle parameters
    const Capacity cv_capacity = problem.getCVCapacity();
    const Duration cv_max_duration = problem.getCVMaxDuration();

    // Depot information
    const auto& depot = problem.getDepot();

    // Generate routes until all zones are assigned
    int route_count = 1;
    while (!unassigned_zones.empty()) {
      // Create a new route
      std::string vehicle_id = "CV" + std::to_string(route_count++);
      CVRoute route(vehicle_id, cv_capacity, cv_max_duration);

      // Set current location to depot
      std::string current_location_id = depot.id();

      // Keep adding zones until no more can be added
      bool zone_added;
      do {
        zone_added = false;

        // Find closest unassigned zone
        std::optional<std::string> closest_zone;
        double min_distance = std::numeric_limits<double>::max();

        for (const auto& zone_id : unassigned_zones) {
          // Check if this zone can be added to the route
          if (route.canVisit(zone_id, problem)) {
            // Calculate distance from current location
            double distance = problem.getDistance(current_location_id, zone_id).value();

            if (distance < min_distance) {
              min_distance = distance;
              closest_zone = zone_id;
            }
          }
        }

        if (closest_zone) {
          // Add the closest zone to the route
          route.addLocation(*closest_zone, problem);
          current_location_id = *closest_zone;
          unassigned_zones.erase(*closest_zone);
          zone_added = true;
        } else {
          // No more zones can be added directly, check if we need to visit SWTS
          if (route.currentLoad().value() > 0) {
            // Find closest SWTS
            const auto& current_location = problem.getLocation(current_location_id);
            auto nearest_swts = problem.findNearest(current_location, LocationType::SWTS);

            if (nearest_swts && route.canVisit(nearest_swts->id(), problem)) {
              // Visit SWTS to unload
              route.addLocation(nearest_swts->id(), problem);
              current_location_id = nearest_swts->id();

              // Try adding zones again
              continue;
            }
          }

          // Cannot add more zones or visit SWTS
          zone_added = false;
        }
      } while (zone_added);

      // Finalize route - ensure it ends at a SWTS if it has any load
      if (!route.isEmpty() && route.currentLoad().value() > 0) {
        // Find nearest SWTS to last location
        const auto& last_location = problem.getLocation(current_location_id);
        auto nearest_swts = problem.findNearest(last_location, LocationType::SWTS);

        if (nearest_swts) {
          route.addLocation(nearest_swts->id(), problem);
        }
      }

      // Add the route to the solution if it's not empty
      if (!route.isEmpty()) {
        solution.addCVRoute(std::move(route));
      }
    }

    return solution;
  }

  std::string name() const override { return "Greedy CV Generator"; }
};

// Register the algorithm
namespace {
inline static const bool Greedy_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerGenerator<GreedyCVGenerator>("GreedyCVGenerator");
}

}  // namespace algorithm
}  // namespace daa
