#pragma once

#include <memory>
#include <random>
#include <string>
#include <vector>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
#include "meta_heuristic_factory.h"

namespace daa {
namespace algorithm {

/**
 * @brief General Variable Neighborhood Search for VRPT problem
 *
 * Implements the GVNS metaheuristic, using multiple neighborhood structures
 * to search for better solutions.
 */
class GVNS : public TypedAlgorithm<VRPTProblem, VRPTSolution> {
 public:
  explicit GVNS(
    int max_iterations = 50,
    const std::string& generator_name = "GRASPCVGenerator",
    std::vector<std::string> neighborhoods =
      {"TaskReinsertionSearch", "TaskExchangeSearch", "TwoOptSearch"}
  )
      : max_iterations_(max_iterations),
        generator_name_(generator_name),
        neighborhood_names_(std::move(neighborhoods)) {}

  VRPTSolution solve(const VRPTProblem& problem) override {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    // Create generator
    auto generator = MetaFactory::createGenerator(generator_name_);

    // Create neighborhood searches
    std::vector<std::unique_ptr<::meta::LocalSearch<VRPTSolution, VRPTProblem>>> neighborhoods;
    for (const auto& name : neighborhood_names_) {
      neighborhoods.push_back(MetaFactory::createSearch(name));
    }

    // No neighborhoods defined
    if (neighborhoods.empty()) {
      return generator->generateSolution(problem);
    }

    // Generate initial solution
    VRPTSolution current_solution = generator->generateSolution(problem);
    VRPTSolution best_solution = current_solution;
    size_t best_cv_count = best_solution.getCVCount();

    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());

    // Main GVNS loop
    int iteration = 0;
    while (iteration < max_iterations_) {
      // Variable neighborhood descent (VND)
      size_t k = 0;
      while (k < neighborhoods.size()) {
        // Apply current neighborhood search
        VRPTSolution improved_solution =
          neighborhoods[k]->improveSolution(problem, current_solution);

        // Check if solution improved
        size_t improved_cv_count = improved_solution.getCVCount();
        if (improved_cv_count < current_solution.getCVCount()) {
          // Improvement found, restart with first neighborhood
          current_solution = improved_solution;
          k = 0;
        } else {
          // No improvement, move to next neighborhood
          k++;
        }
      }

      // Check if we found a new best solution
      if (current_solution.getCVCount() < best_cv_count) {
        best_solution = current_solution;
        best_cv_count = current_solution.getCVCount();
      }

      // Shaking - perturb the current solution
      current_solution = shake(problem, current_solution, gen);

      iteration++;
    }

    return best_solution;
  }

  /**
   * @brief Shake the current solution to escape local optima
   *
   * @param problem The VRPT problem instance
   * @param solution The current solution
   * @param gen Random number generator
   * @return A perturbed solution
   */
  VRPTSolution shake(const VRPTProblem& problem, const VRPTSolution& solution, std::mt19937& gen) {
    // Copy the solution
    VRPTSolution new_solution = solution;
    auto& routes = new_solution.getCVRoutes();

    // Need at least two routes to perform shaking
    if (routes.size() < 2) {
      return new_solution;
    }

    // Select two random routes
    std::uniform_int_distribution<size_t> route_dist(0, routes.size() - 1);
    size_t r1_idx = route_dist(gen);
    size_t r2_idx;
    do {
      r2_idx = route_dist(gen);
    } while (r1_idx == r2_idx);

    // Get route locations
    const auto& r1_locs = routes[r1_idx].locationIds();
    const auto& r2_locs = routes[r2_idx].locationIds();

    // Collect collection zones from both routes
    std::vector<std::string> r1_zones, r2_zones;
    for (const auto& loc_id : r1_locs) {
      const auto& loc = problem.getLocation(loc_id);
      if (loc.type() == LocationType::COLLECTION_ZONE) {
        r1_zones.push_back(loc_id);
      }
    }

    for (const auto& loc_id : r2_locs) {
      const auto& loc = problem.getLocation(loc_id);
      if (loc.type() == LocationType::COLLECTION_ZONE) {
        r2_zones.push_back(loc_id);
      }
    }

    // If either route has no zones, return the original solution
    if (r1_zones.empty() || r2_zones.empty()) {
      return new_solution;
    }

    // Select random zones to transfer
    std::uniform_int_distribution<size_t> zone_dist1(0, r1_zones.size() - 1);
    std::uniform_int_distribution<size_t> zone_dist2(0, r2_zones.size() - 1);

    std::string zone1 = r1_zones[zone_dist1(gen)];
    std::string zone2 = r2_zones[zone_dist2(gen)];

    // Create new routes with zones swapped
    std::vector<std::string> new_r1_locs, new_r2_locs;
    for (const auto& loc_id : r1_locs) {
      if (loc_id != zone1) {
        new_r1_locs.push_back(loc_id);
      } else {
        new_r1_locs.push_back(zone2);
      }
    }

    for (const auto& loc_id : r2_locs) {
      if (loc_id != zone2) {
        new_r2_locs.push_back(loc_id);
      } else {
        new_r2_locs.push_back(zone1);
      }
    }

    // Build new routes
    Capacity cv_capacity = problem.getCVCapacity();
    Duration cv_max_duration = problem.getCVMaxDuration();

    // Create new routes - if not feasible, return original solution
    CVRoute new_r1(routes[r1_idx].vehicleId(), cv_capacity, cv_max_duration);
    for (const auto& loc_id : new_r1_locs) {
      if (!new_r1.canVisit(loc_id, problem)) {
        return solution;
      }
      new_r1.addLocation(loc_id, problem);
    }

    CVRoute new_r2(routes[r2_idx].vehicleId(), cv_capacity, cv_max_duration);
    for (const auto& loc_id : new_r2_locs) {
      if (!new_r2.canVisit(loc_id, problem)) {
        return solution;
      }
      new_r2.addLocation(loc_id, problem);
    }

    // Update routes
    routes[r1_idx] = new_r1;
    routes[r2_idx] = new_r2;

    return new_solution;
  }

  std::string name() const override {
    return "GVNS(" + std::to_string(max_iterations_) + ", " + generator_name_ + ")";
  }

  std::string description() const override {
    return "General Variable Neighborhood Search with " +
           std::to_string(neighborhood_names_.size()) + " neighborhoods and max " +
           std::to_string(max_iterations_) + " iterations";
  }

  std::string timeComplexity() const override {
    return "O(k × i × n)";  // k = neighborhoods, i = iterations, n = complexity of each move
  }

 private:
  int max_iterations_;
  std::string generator_name_;
  std::vector<std::string> neighborhood_names_;
};

// Register the algorithm with default parameters
REGISTER_ALGORITHM(GVNS, "GVNS");

}  // namespace algorithm
}  // namespace daa
