#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
#include "imgui.h"
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

    // Get all collection zones (line 3: while C ≠ ∅)
    std::vector<Location> zones = problem.getZones();
    std::unordered_set<std::string> unassigned_zones;
    for (const auto& zone : zones) {
      unassigned_zones.insert(zone.id());
    }

    // Generate routes until all zones are assigned
    int route_count = 1;
    while (!unassigned_zones.empty()) {
      // Create a new route (lines 4-6)
      std::string vehicle_id = "CV" + std::to_string(route_count++);
      CVRoute route(vehicle_id, problem.getCVCapacity(), problem.getCVMaxDuration());

      // Start from depot (implicit in line 4: Rk ← {depot})
      std::string current_location_id = problem.getDepot().id();

      // Main route building loop (line 7: while true)
      while (true) {
        // Find closest unassigned zone (line 8)
        std::optional<std::string> closest_zone;
        double min_distance = std::numeric_limits<double>::max();

        for (const auto& zone_id : unassigned_zones) {
          double distance = problem.getDistance(current_location_id, zone_id).value();
          if (distance < min_distance) {
            // Check feasibility (lines 9-10)
            // Time needed to visit zone, a SWTS, and return to depot
            const auto& zone = problem.getLocation(zone_id);
            auto nearest_swts = problem.findNearest(zone, LocationType::SWTS);

            if (!nearest_swts)
              continue;

            Duration travel_time_to_zone = problem.getTravelTime(current_location_id, zone_id);
            Duration service_time = zone.serviceTime();
            Duration travel_time_to_swts = problem.getTravelTime(zone_id, nearest_swts->id());
            Duration travel_time_to_depot =
              problem.getTravelTime(nearest_swts->id(), problem.getDepot().id());

            Duration total_time =
              travel_time_to_zone + service_time + travel_time_to_swts + travel_time_to_depot;

            // Check capacity and time constraints
            if (zone.wasteAmount() <= route.residualCapacity() &&
                total_time <= route.residualTime()) {
              min_distance = distance;
              closest_zone = zone_id;
            }
          }
        }

        if (closest_zone) {
          // Add zone to route (lines 11-14)
          route.addLocation(*closest_zone, problem);
          current_location_id = *closest_zone;
          unassigned_zones.erase(*closest_zone);
        } else {
          // Cannot add a zone directly (lines 15-16)
          // Check if going to SWTS is feasible time-wise
          const auto& current_loc = problem.getLocation(current_location_id);
          auto nearest_swts = problem.findNearest(current_loc, LocationType::SWTS);

          if (nearest_swts && route.canVisit(nearest_swts->id(), problem)) {
            // Go to SWTS and reset capacity (lines 17-20)
            route.addLocation(nearest_swts->id(), problem);
            current_location_id = nearest_swts->id();
          } else {
            // Cannot continue this route (line 22)
            break;
          }
        }
      }

      // Finalize the route (lines 26-31)
      const auto& current_loc = problem.getLocation(current_location_id);
      if (current_loc.type() != LocationType::SWTS) {
        // If not at SWTS, find closest SWTS and go there
        auto nearest_swts = problem.findNearest(current_loc, LocationType::SWTS);
        if (nearest_swts && route.canVisit(nearest_swts->id(), problem)) {
          route.addLocation(nearest_swts->id(), problem);
        }
      }

      // Return to depot
      route.addLocation(problem.getDepot().id(), problem);

      // Add route to solution if it's not empty (line 32)
      if (!route.isEmpty()) {
        solution.addCVRoute(std::move(route));
      }
    }

    return solution;
  }

  std::string name() const override { return "Greedy CV Generator"; }

  /**
   * @brief Render UI elements for configuring the greedy generator
   *
   * This generator doesn't have configurable parameters, so we just display info.
   */
  void renderConfigurationUI() override {
    ImGui::TextWrapped("The Greedy CV Generator has no configurable parameters.");
    ImGui::TextWrapped("It always selects the closest feasible collection zone.");
  }
};

// Register the algorithm
namespace {
inline static const bool Greedy_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerGenerator<GreedyCVGenerator>("GreedyCVGenerator");
}

}  // namespace algorithm
}  // namespace daa
