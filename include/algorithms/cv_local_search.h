#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
#include "meta_heuristic_components.h"
#include "meta_heuristic_factory.h"
#include "problem/vrpt_problem.h"

namespace daa {
namespace algorithm {

/**
 * @brief Base class for Collection Vehicle route local search algorithms
 */
class CVLocalSearch : public ::meta::LocalSearch<VRPTSolution, VRPTProblem> {
 public:
  /**
   * @brief Default constructor
   */
  CVLocalSearch() = default;

  /**
   * @brief Constructor with local search parameters
   * @param max_iterations Maximum number of iterations to perform
   * @param first_improvement Whether to use first improvement (true) or best improvement (false)
   */
  explicit CVLocalSearch(int max_iterations = 100, bool first_improvement = false)
      : max_iterations_(max_iterations), first_improvement_(first_improvement) {}

  /**
   * @brief Improve an existing solution using local search
   * @param problem The problem instance
   * @param initial_solution The initial solution to improve
   * @return The improved solution
   */
  VRPTSolution improveSolution(const VRPTProblem& problem, const VRPTSolution& initial_solution)
    override {
    VRPTSolution current_solution = initial_solution;
    VRPTSolution best_solution = current_solution;
    size_t best_cv_count = best_solution.getCVCount();

    bool improved;
    int iteration = 0;

    do {
      improved = false;

      // Apply neighborhood search
      VRPTSolution neighbor_solution = searchNeighborhood(problem, current_solution);
      size_t neighbor_cv_count = neighbor_solution.getCVCount();

      // Check if the neighbor is better
      if (neighbor_cv_count < best_cv_count) {
        best_solution = neighbor_solution;
        best_cv_count = neighbor_cv_count;
        current_solution = neighbor_solution;
        improved = true;
      }

      iteration++;
    } while (improved && iteration < max_iterations_);

    return best_solution;
  }

  /**
   * @brief Search the neighborhood for an improved solution
   * @param problem The problem instance
   * @param current_solution The current solution
   * @return The best neighboring solution found
   */
  virtual VRPTSolution
    searchNeighborhood(const VRPTProblem& problem, const VRPTSolution& current_solution) = 0;

 protected:
  int max_iterations_ = 100;
  bool first_improvement_ = false;
};

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
            if (new_cv_count < best_cv_count) {
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
            if (new_cv_count < best_cv_count) {
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
    size_t best_cv_count = best_solution.getCVCount();

    // Create a copy of the solution's CV routes to modify
    auto routes = current_solution.getCVRoutes();

    // Apply 2-opt to each route
    for (size_t route_idx = 0; route_idx < routes.size(); ++route_idx) {
      const auto& route = routes[route_idx];
      const auto& locations = route.locationIds();

      // Need at least 4 locations for 2-opt to make sense
      if (locations.size() < 4) {
        continue;
      }

      // Try each possible 2-opt swap
      for (size_t i = 0; i < locations.size() - 2; ++i) {
        for (size_t j = i + 2; j < locations.size(); ++j) {
          // Create a new solution with the 2-opt swap
          VRPTSolution new_solution = current_solution;
          auto& new_routes = new_solution.getCVRoutes();

          // Create new route with the segment reversed
          std::vector<std::string> new_locations;

          // Add locations before the reversed segment
          for (size_t k = 0; k <= i; ++k) {
            new_locations.push_back(locations[k]);
          }

          // Add reversed segment
          for (size_t k = j; k > i; --k) {
            new_locations.push_back(locations[k]);
          }

          // Add locations after the reversed segment
          for (size_t k = j + 1; k < locations.size(); ++k) {
            new_locations.push_back(locations[k]);
          }

          // Rebuild the route with the new sequence
          Capacity cv_capacity = problem.getCVCapacity();
          Duration cv_max_duration = problem.getCVMaxDuration();

          CVRoute new_route(route.vehicleId(), cv_capacity, cv_max_duration);
          for (const auto& loc_id : new_locations) {
            if (!new_route.canVisit(loc_id, problem)) {
              continue;
            }
            new_route.addLocation(loc_id, problem);
          }

          // Update the route
          new_routes[route_idx] = new_route;

          // Check if the new solution is better
          size_t new_cv_count = new_routes.size();
          if (new_cv_count < best_cv_count) {
            best_solution = new_solution;
            best_cv_count = new_cv_count;

            if (first_improvement_) {
              return best_solution;
            }
          }
        }
      }
    }

    return best_solution;
  }

  std::string name() const override { return "2-Opt Search"; }
};

namespace {
inline static const bool TaskReinsertionSearch_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerSearch<TaskReinsertionSearch>("TaskReinsertionSearch");

inline static const bool TaskExchangeSearch_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerSearch<TaskExchangeSearch>("TaskExchangeSearch");

inline static const bool TwoOptSearch_registered_gen =
  MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>::
    registerSearch<TwoOptSearch>("TwoOptSearch");
}

}  // namespace algorithm
}  // namespace daa
