#pragma once

#include <algorithm>
#include <random>
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

    // Generate routes until all zones are assigned
    int route_count = 1;
    while (!unassigned_zones.empty()) {
      // Create a new route
      std::string vehicle_id = "CV" + std::to_string(route_count++);
      CVRoute route(vehicle_id, cv_capacity, cv_max_duration);

      // Start with a T1 leg (Depot -> Zones -> SWTS)
      if (!buildT1Leg(route, unassigned_zones, problem, gen)) {
        // If we can't build a T1 leg, something is wrong with the problem
        break;
      }

      // Add T2 legs (SWTS -> Zones -> SWTS) while possible
      while (!unassigned_zones.empty()) {
        // Calculate return time to depot from current location
        Duration return_time =
          problem.getTravelTime(route.lastLocationId(), problem.getDepot().id());

        // Make sure we have enough time for another leg plus return to depot
        if (route.totalDuration() + return_time >= route.residualTime()) {
          break;
        }

        // Try building a T2 leg
        if (!buildT2Leg(route, unassigned_zones, problem, gen)) {
          break;  // No more T2 legs can be added
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

  /**
   * @brief Render UI elements for configuring the GRASP generator
   */
  void renderConfigurationUI() override {
    // Slider for alpha parameter (0.0 = pure greedy, 1.0 = pure random)
    float alpha = static_cast<float>(alpha_);
    if (ImGui::SliderFloat("Alpha", &alpha, 0.0f, 1.0f, "%.2f")) {
      alpha_ = static_cast<double>(alpha);
    }
    ImGui::SameLine();
    ImGui::HelpMarker("Alpha controls randomization: 0.0 = pure greedy, 1.0 = pure random");

    // Input for RCL size
    int rcl_size = static_cast<int>(rcl_size_);
    if (ImGui::InputInt("RCL Size", &rcl_size)) {
      rcl_size_ = std::max(1, rcl_size);  // Ensure at least 1
    }
    ImGui::SameLine();
    ImGui::HelpMarker("Maximum size of the Restricted Candidate List");
  }

 private:
  double alpha_;          // Greediness parameter (0.0 = pure greedy, 1.0 = pure random)
  std::size_t rcl_size_;  // Maximum size of restricted candidate list

  /**
   * @brief Build a T1 leg (Depot -> Zones -> SWTS)
   * @param route The route to add the leg to
   * @param unassigned_zones Set of unassigned zones
   * @param problem The problem instance
   * @param gen Random number generator
   * @return true if a T1 leg was successfully built, false otherwise
   */
  bool buildT1Leg(
    CVRoute& route,
    std::unordered_set<std::string>& unassigned_zones,
    const VRPTProblem& problem,
    std::mt19937& gen
  ) {

    // Start location is depot
    std::string current_location_id = problem.getDepot().id();
    bool added_zones = false;

    // Keep adding zones until capacity is reached or no more can be added
    while (!unassigned_zones.empty()) {
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

      // If no candidates, stop adding zones
      if (candidates.empty()) {
        break;
      }

      // Apply GRASP selection from candidates
      std::string selected_zone = selectCandidateFromRCL(candidates, gen);

      // Add the selected zone to the route
      route.addLocation(selected_zone, problem);
      current_location_id = selected_zone;
      unassigned_zones.erase(selected_zone);
      added_zones = true;
    }

    // If we added zones, finish the leg by going to the nearest SWTS
    if (added_zones) {
      // Get SWTS candidates
      auto swts_locations = problem.getSWTS();
      std::vector<std::pair<std::string, double>> swts_candidates;

      for (const auto& swts : swts_locations) {
        if (route.canVisit(swts.id(), problem)) {
          double distance = problem.getDistance(current_location_id, swts.id()).value();
          swts_candidates.emplace_back(swts.id(), distance);
        }
      }

      if (!swts_candidates.empty()) {
        // Select SWTS using GRASP
        std::string selected_swts = selectCandidateFromRCL(swts_candidates, gen);

        // Add SWTS to complete T1 leg
        route.addLocation(selected_swts, problem);
        return true;
      }
    }

    // If we couldn't complete the T1 leg, return false
    return false;
  }

  /**
   * @brief Build a T2 leg (SWTS -> Zones -> SWTS)
   * @param route The route to add the leg to
   * @param unassigned_zones Set of unassigned zones
   * @param problem The problem instance
   * @param gen Random number generator
   * @return true if a T2 leg was successfully built, false otherwise
   */
  bool buildT2Leg(
    CVRoute& route,
    std::unordered_set<std::string>& unassigned_zones,
    const VRPTProblem& problem,
    std::mt19937& gen
  ) {

    // Current location should be a SWTS
    std::string current_location_id = route.lastLocationId();
    bool added_zones = false;

    // Keep adding zones until capacity is reached or no more can be added
    while (!unassigned_zones.empty()) {
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

      // If no candidates, stop adding zones
      if (candidates.empty()) {
        break;
      }

      // Apply GRASP selection from candidates
      std::string selected_zone = selectCandidateFromRCL(candidates, gen);

      // Add the selected zone to the route
      route.addLocation(selected_zone, problem);
      current_location_id = selected_zone;
      unassigned_zones.erase(selected_zone);
      added_zones = true;
    }

    // If we added zones, finish the leg by going to the nearest SWTS
    if (added_zones) {
      // Get SWTS candidates
      auto swts_locations = problem.getSWTS();
      std::vector<std::pair<std::string, double>> swts_candidates;

      for (const auto& swts : swts_locations) {
        if (route.canVisit(swts.id(), problem)) {
          double distance = problem.getDistance(current_location_id, swts.id()).value();
          swts_candidates.emplace_back(swts.id(), distance);
        }
      }

      if (!swts_candidates.empty()) {
        // Select SWTS using GRASP
        std::string selected_swts = selectCandidateFromRCL(swts_candidates, gen);

        // Add SWTS to complete T2 leg
        route.addLocation(selected_swts, problem);
        return true;
      }
    }

    // If we couldn't add any zones or reach a SWTS, return false
    return added_zones;
  }

  /**
   * @brief Select a candidate from a restricted candidate list using GRASP
   * @param candidates Vector of candidates with their distances
   * @param gen Random number generator
   * @return Selected candidate ID
   */
  std::string selectCandidateFromRCL(
    const std::vector<std::pair<std::string, double>>& candidates,
    std::mt19937& gen
  ) {

    if (candidates.empty()) {
      return "";
    }

    if (candidates.size() == 1 || alpha_ <= 0.0) {
      return candidates.front().first;
    }

    // Sort candidates by distance (should already be sorted, but ensure it)
    auto sorted_candidates = candidates;
    std::sort(sorted_candidates.begin(), sorted_candidates.end(), [](const auto& a, const auto& b) {
      return a.second < b.second;
    });

    // Build restricted candidate list (RCL)
    std::vector<std::string> rcl;
    double min_dist = sorted_candidates.front().second;
    double max_dist = sorted_candidates.back().second;
    double threshold = min_dist + alpha_ * (max_dist - min_dist);

    // Add candidates that meet the threshold
    for (const auto& [id, distance] : sorted_candidates) {
      if (distance <= threshold) {
        rcl.push_back(id);
        if (rcl.size() >= rcl_size_) {
          break;
        }
      }
    }

    // Select random candidate from RCL
    std::uniform_int_distribution<int> dist(0, rcl.size() - 1);
    return rcl[dist(gen)];
  }
};

namespace {
inline static const bool GRASP_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerGenerator<GRASPCVGenerator>("GRASPCVGenerator");
}

}  // namespace algorithm
}  // namespace daa
