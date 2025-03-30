#pragma once

#include <algorithm>
#include <random>
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
 * @brief GRASP constructive heuristic for Collection Vehicle routes
 *
 * This implements the constructive phase of a GRASP algorithm for CV routing,
 * using a restricted candidate list (RCL) approach.
 */
class GRASPCVGenerator : public ::meta::SolutionGenerator<VRPTSolution, VRPTProblem> {
 public:
  /**
   * @brief Constructor with alpha parameter
   * @param alpha Greediness parameter (0.0 = pure greedy, 1.0 = pure random)
   * @param rcl_size Maximum size of restricted candidate list
   */
  GRASPCVGenerator(double alpha = 0.3, std::size_t rcl_size = 5)
      : alpha_(alpha), rcl_size_(rcl_size) {}

  /**
   * @brief Generate initial solution using GRASP approach
   * @param problem The problem instance
   * @return A solution with CV routes
   */
  VRPTSolution generateSolution(const VRPTProblem& problem) override {
    VRPTSolution solution;

    // Set up random number generator
    std::random_device rd;
    std::mt19937 gen(rd());

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

        // Build candidate list of feasible zones
        std::vector<std::pair<std::string, double>> candidates;

        for (const auto& zone_id : unassigned_zones) {
          // Check if this zone can be added to the route
          if (route.canVisit(zone_id, problem)) {
            // Calculate distance from current location
            double distance = problem.getDistance(current_location_id, zone_id).value();
            candidates.emplace_back(zone_id, distance);
          }
        }

        if (!candidates.empty()) {
          // Sort candidates by distance
          std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
          });

          // Build restricted candidate list (RCL)
          std::vector<std::string> rcl;
          double min_dist = candidates.front().second;
          double max_dist = candidates.back().second;
          double threshold = min_dist + alpha_ * (max_dist - min_dist);

          // Add candidates that meet the threshold
          for (const auto& [zone_id, distance] : candidates) {
            if (distance <= threshold) {
              rcl.push_back(zone_id);
              if (rcl.size() >= rcl_size_)
                break;
            }
          }

          // Select random zone from RCL
          std::uniform_int_distribution<int> dist(0, rcl.size() - 1);
          std::string selected_zone = rcl[dist(gen)];

          // Add the selected zone to the route
          route.addLocation(selected_zone, problem);
          current_location_id = selected_zone;
          unassigned_zones.erase(selected_zone);
          zone_added = true;
        } else {
          // No more zones can be added directly, check if we need to visit SWTS
          if (route.currentLoad().value() > 0) {
            // Get candidate SWTS options
            auto swts_locations = problem.getSWTS();

            std::vector<std::pair<std::string, double>> swts_candidates;
            for (const auto& swts : swts_locations) {
              if (route.canVisit(swts.id(), problem)) {
                double distance = problem.getDistance(current_location_id, swts.id()).value();
                swts_candidates.emplace_back(swts.id(), distance);
              }
            }

            if (!swts_candidates.empty()) {
              // Sort SWTS by distance
              std::sort(
                swts_candidates.begin(),
                swts_candidates.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
              );

              // Select the nearest SWTS or use GRASP selection
              std::string selected_swts;
              if (swts_candidates.size() > 1 && alpha_ > 0.0) {
                // Build RCL for SWTS
                std::vector<std::string> swts_rcl;
                double min_dist = swts_candidates.front().second;
                double max_dist = swts_candidates.back().second;
                double threshold = min_dist + alpha_ * (max_dist - min_dist);

                for (const auto& [swts_id, distance] : swts_candidates) {
                  if (distance <= threshold) {
                    swts_rcl.push_back(swts_id);
                    if (swts_rcl.size() >= rcl_size_)
                      break;
                  }
                }

                // Select random SWTS from RCL
                std::uniform_int_distribution<int> dist(0, swts_rcl.size() - 1);
                selected_swts = swts_rcl[dist(gen)];
              } else {
                // Just pick the closest
                selected_swts = swts_candidates.front().first;
              }

              // Visit SWTS to unload
              route.addLocation(selected_swts, problem);
              current_location_id = selected_swts;

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
        // Get all SWTS candidates
        auto swts_locations = problem.getSWTS();

        std::vector<std::pair<std::string, double>> swts_candidates;
        for (const auto& swts : swts_locations) {
          if (route.canVisit(swts.id(), problem)) {
            double distance = problem.getDistance(current_location_id, swts.id()).value();
            swts_candidates.emplace_back(swts.id(), distance);
          }
        }

        if (!swts_candidates.empty()) {
          // Sort by distance and select the nearest
          std::sort(
            swts_candidates.begin(),
            swts_candidates.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; }
          );

          route.addLocation(swts_candidates.front().first, problem);
        }
      }

      // Add the route to the solution if it's not empty
      if (!route.isEmpty()) {
        solution.addCVRoute(std::move(route));
      }
    }

    return solution;
  }

  std::string name() const override {
    return "GRASP CV Generator (alpha=" + std::to_string(alpha_) +
           ", rcl_size=" + std::to_string(rcl_size_) + ")";
  }

 private:
  double alpha_;          // Greediness parameter (0.0 = pure greedy, 1.0 = pure random)
  std::size_t rcl_size_;  // Maximum size of restricted candidate list
};

namespace {
inline static const bool GRASP_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerGenerator<GRASPCVGenerator>("GRASPCVGenerator");
}

}  // namespace algorithm
}  // namespace daa
