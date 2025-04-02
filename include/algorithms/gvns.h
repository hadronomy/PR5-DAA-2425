#pragma once

#include <latch>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
#include "meta_heuristic_components.h"
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
  // Default constructor with default parameters
  explicit GVNS(
    int max_iterations = 50,
    const std::string& generator_name = "GRASPCVGenerator",
    std::vector<std::string> neighborhoods =
      {"TaskReinsertionSearch", "TaskExchangeSearch", "TwoOptSearch"}
  )
      : max_iterations_(max_iterations),
        generator_name_(generator_name),
        neighborhood_names_(std::move(neighborhoods)) {
    // Initialize components
    initializeComponents();
  }

  VRPTSolution solve(const VRPTProblem& problem) override {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    // Create generator if not already created
    if (!generator_) {
      generator_ = MetaFactory::createGenerator(generator_name_);
    }

    // Create neighborhood searches if not already created
    if (neighborhoods_.empty()) {
      for (const auto& name : neighborhood_names_) {
        neighborhoods_.push_back(MetaFactory::createSearch(name));
      }
    }

    // No neighborhoods defined
    if (neighborhoods_.empty()) {
      return generator_->generateSolution(problem);
    }

    // Generate initial solution
    VRPTSolution initial_solution = generator_->generateSolution(problem);

    // Thread-safe container for the best solution
    std::mutex best_solution_mutex;
    VRPTSolution best_solution = initial_solution;
    size_t best_cv_count = best_solution.getCVCount();
    size_t best_zones_count = best_solution.visitedZones(problem);
    Duration best_total_duration = best_solution.totalDuration();

    // Create a thread pool
    const unsigned int thread_count = std::thread::hardware_concurrency();
    std::vector<std::jthread> threads;
    std::latch completion_latch(max_iterations_);

    // Split work among threads
    for (int thread_id = 0;
         thread_id < std::min(thread_count, static_cast<unsigned int>(max_iterations_));
         ++thread_id) {
      threads.emplace_back([&, thread_id]() {
        // Thread-local random number generator
        std::random_device rd;
        std::mt19937 gen(rd() + thread_id);  // Add thread_id to make each generator unique

        // Calculate start and end indices for this thread
        int iterations_per_thread = max_iterations_ / thread_count;
        int start_idx = thread_id * iterations_per_thread;
        int end_idx = (thread_id == thread_count - 1) ? max_iterations_
                                                      : (thread_id + 1) * iterations_per_thread;

        // Create thread-local copies of metaheuristic components
        auto thread_generator = MetaFactory::createGenerator(generator_name_);
        std::vector<std::unique_ptr<::meta::LocalSearch<VRPTSolution, VRPTProblem>>>
          thread_neighborhoods;
        for (const auto& name : neighborhood_names_) {
          thread_neighborhoods.push_back(MetaFactory::createSearch(name));
        }

        // Start with a copy of the initial solution
        VRPTSolution current_solution = initial_solution;

        // Process assigned iterations
        for (int iteration = start_idx; iteration < end_idx; ++iteration) {
          // Random Variable Neighborhood Descent (RVND)
          std::vector<size_t> available_neighborhoods(thread_neighborhoods.size());
          std::iota(available_neighborhoods.begin(), available_neighborhoods.end(), 0);

          while (!available_neighborhoods.empty()) {
            // Randomly select neighborhood
            std::uniform_int_distribution<size_t> dist(0, available_neighborhoods.size() - 1);
            size_t idx = dist(gen);
            size_t k = available_neighborhoods[idx];

            // Apply current neighborhood search
            VRPTSolution improved_solution =
              thread_neighborhoods[k]->improveSolution(problem, current_solution);

            // Check if solution improved using all criteria
            size_t improved_cv_count = improved_solution.getCVCount();
            size_t improved_zones_count = improved_solution.visitedZones(problem);
            Duration improved_total_duration = improved_solution.totalDuration();

            bool is_better = false;

            // Solution is better if:
            // 1. It uses fewer vehicles, OR
            // 2. It uses same number of vehicles but visits more zones, OR
            // 3. It uses same vehicles, visits same zones, but has shorter duration
            if (improved_cv_count < current_solution.getCVCount()) {
              is_better = true;
            } else if (improved_cv_count == current_solution.getCVCount()) {
              if (improved_zones_count > current_solution.visitedZones(problem)) {
                is_better = true;
              } else if (improved_zones_count == current_solution.visitedZones(problem) &&
                         improved_total_duration < current_solution.totalDuration()) {
                is_better = true;
              }
            }

            if (is_better) {
              // Improvement found, reset available neighborhoods
              current_solution = improved_solution;
              available_neighborhoods.resize(thread_neighborhoods.size());
              std::iota(available_neighborhoods.begin(), available_neighborhoods.end(), 0);
            } else {
              // No improvement, remove this neighborhood from consideration
              available_neighborhoods.erase(available_neighborhoods.begin() + idx);
            }
          }

          // Check if we found a new best solution - thread-safe update
          {
            std::lock_guard<std::mutex> lock(best_solution_mutex);

            size_t current_cv_count = current_solution.getCVCount();
            size_t current_zones_count = current_solution.visitedZones(problem);
            Duration current_total_duration = current_solution.totalDuration();

            bool is_new_best = false;
            if (current_cv_count < best_cv_count) {
              is_new_best = true;
            } else if (current_cv_count == best_cv_count) {
              if (current_zones_count > best_zones_count) {
                is_new_best = true;
              } else if (current_zones_count == best_zones_count &&
                         current_total_duration < best_total_duration) {
                is_new_best = true;
              }
            }

            if (is_new_best) {
              best_solution = current_solution;
              best_cv_count = current_cv_count;
              best_zones_count = current_zones_count;
              best_total_duration = current_total_duration;
            }
          }

          // Shaking - perturb the current solution
          current_solution = shake(problem, current_solution, gen);

          // Mark this iteration as completed
          completion_latch.count_down();
        }
      });
    }

    // Wait for all iterations to complete
    completion_latch.wait();
    // jthreads will automatically join when they go out of scope

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

    // Check if the first route ends at depot and has 0 load
    if (new_r1.currentLoad().value() != 0.0 || new_r1.lastLocationId() != problem.getDepot().id()) {
      return solution;  // Return original solution if invalid
    }

    CVRoute new_r2(routes[r2_idx].vehicleId(), cv_capacity, cv_max_duration);
    for (const auto& loc_id : new_r2_locs) {
      if (!new_r2.canVisit(loc_id, problem)) {
        return solution;
      }
      new_r2.addLocation(loc_id, problem);
    }

    // Check if the second route ends at depot and has 0 load
    if (new_r2.currentLoad().value() != 0.0 || new_r2.lastLocationId() != problem.getDepot().id()) {
      return solution;  // Return original solution if invalid
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

  // UI configuration rendering
  void renderConfigurationUI() override;

 private:
  int max_iterations_;
  std::string generator_name_;
  std::vector<std::string> neighborhood_names_;

  // Component instances for reuse
  std::unique_ptr<::meta::SolutionGenerator<VRPTSolution, VRPTProblem>> generator_;
  std::vector<std::unique_ptr<::meta::LocalSearch<VRPTSolution, VRPTProblem>>> neighborhoods_;

  // Map to track neighborhood search instances by name for UI configuration
  std::unordered_map<std::string, ::meta::LocalSearch<VRPTSolution, VRPTProblem>*> search_map_;

  // Initialize/update components when configuration changes
  void initializeComponents() {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    try {
      // Create generator
      if (!generator_name_.empty()) {
        generator_ = MetaFactory::createGenerator(generator_name_);
      }

      // Create neighborhood searches
      neighborhoods_.clear();
      search_map_.clear();

      for (const auto& name : neighborhood_names_) {
        auto search = MetaFactory::createSearch(name);
        search_map_[name] = search.get();
        neighborhoods_.push_back(std::move(search));
      }
    } catch (const std::exception&) {
      // Initialization will be retried later if needed
    }
  }

  // Update neighborhoods when selection changes
  void updateNeighborhoods() {
    using MetaFactory =
      MetaHeuristicFactory<VRPTSolution, VRPTProblem, TypedAlgorithm<VRPTProblem, VRPTSolution>>;

    // Create a copy of the current search map to track which ones to keep
    std::unordered_map<std::string, ::meta::LocalSearch<VRPTSolution, VRPTProblem>*> existing_map =
      search_map_;

    // Clear current neighborhoods and rebuild
    std::vector<std::unique_ptr<::meta::LocalSearch<VRPTSolution, VRPTProblem>>>
      updated_neighborhoods;
    std::unordered_map<std::string, ::meta::LocalSearch<VRPTSolution, VRPTProblem>*>
      updated_search_map;

    // Process each selected neighborhood name
    for (const auto& name : neighborhood_names_) {
      auto it = existing_map.find(name);

      if (it != existing_map.end()) {
        // This neighborhood already exists, reuse it
        // Find the corresponding neighborhood in neighborhoods_
        for (auto& neighborhood : neighborhoods_) {
          if (neighborhood.get() == it->second) {
            // Found it, move to updated list
            updated_search_map[name] = neighborhood.get();
            updated_neighborhoods.push_back(std::move(neighborhood));
            break;
          }
        }
        // Remove from existing map to mark as processed
        existing_map.erase(name);
      } else {
        // This is a new neighborhood, create it
        try {
          auto search = MetaFactory::createSearch(name);
          updated_search_map[name] = search.get();
          updated_neighborhoods.push_back(std::move(search));
        } catch (const std::exception&) {
          // Skip if creation fails
        }
      }
    }

    // Update the member variables
    neighborhoods_ = std::move(updated_neighborhoods);
    search_map_ = std::move(updated_search_map);
  }
};

// Register the algorithm with default parameters
REGISTER_ALGORITHM(GVNS, "GVNS");

}  // namespace algorithm
}  // namespace daa
