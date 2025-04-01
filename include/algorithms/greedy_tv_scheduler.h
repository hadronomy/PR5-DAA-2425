#pragma once

#include <algorithm>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
#include "problem/vrpt_problem.h"

namespace daa {
namespace algorithm {

/**
 * @brief Exception class for TV scheduling errors
 */
class TVSchedulerError : public std::runtime_error {
 public:
  explicit TVSchedulerError(const std::string& message) : std::runtime_error(message) {}
};

struct VRPTData {
  const VRPTProblem& problem;
  VRPTSolution solution;

  VRPTData() = delete;
  VRPTData(const VRPTProblem& problem, const VRPTSolution& solution)
      : problem(problem), solution(solution) {}
};

/**
 * @brief Greedy algorithm for scheduling Transportation Vehicle routes (Phase 2)
 *
 * Based on Algorithms 3 & 4 in the paper, this algorithm takes CV routes as input
 * and computes TV routes to complete the VRPT solution. It processes delivery tasks
 * in chronological order and assigns each to the best available TV or creates a new one.
 */
class GreedyTVScheduler : public TypedAlgorithm<VRPTData, VRPTSolution> {
 public:
  /**
   * @brief Default constructor
   */
  GreedyTVScheduler() = default;

  /**
   * @brief Solve Phase 2 of the VRPT problem
   *
   * @param input Solution from Phase 1 containing CV routes
   * @return Complete solution with both CV and TV routes
   * @throws TVSchedulerError If scheduling is infeasible
   */
  VRPTSolution solve(const VRPTData& data) override {
    // Create a copy of the input solution
    VRPTSolution solution = data.solution;
    const VRPTProblem& problem = data.problem;

    // Get all delivery tasks from CV routes, sorted by arrival time
    std::vector<DeliveryTask> tasks = solution.getAllDeliveryTasks();

    // Check if there are any tasks
    if (tasks.empty()) {
      solution.setComplete(true);
      return solution;
    }

    // Determine minimum waste amount (q_min) for deciding when to return to landfill
    Capacity q_min = tasks.front().amount();
    for (const auto& task : tasks) {
      if (task.amount() < q_min) {
        q_min = task.amount();
      }
    }

    // Initialize empty set of TV routes
    std::vector<TVRoute> tv_routes;

    // Process each task in order of arrival time (tasks are already sorted)
    for (size_t i = 0; i < tasks.size(); ++i) {
      const auto& task = tasks[i];

      int best_vehicle_idx = -1;
      std::optional<Duration> min_insertion_cost = std::nullopt;

      // Try to find the best existing TV route that can handle this task
      for (size_t e = 0; e < tv_routes.size(); ++e) {
        auto& route = tv_routes[e];

        // Check feasibility conditions
        const std::string& last_location = route.lastLocationId();

        Duration travel_time = (last_location.empty())
                               ? Duration{0.0}
                               : problem.getTravelTime(last_location, task.swtsId());

        // (a) Timing: We can always handle the task regardless of when we arrive
        // If we arrive early, we'll wait; if we arrive late, the waste is already there
        // So timing is always feasible - we just need to account for waiting time in insertion cost
        Duration tv_arrival_time = route.currentTime() + travel_time;
        Duration waiting_time = Duration{0.0};

        if (tv_arrival_time <= task.arrivalTime()) {
          // TV arrives before the task, needs to wait
          waiting_time = task.arrivalTime() - tv_arrival_time;
        }

        // (b) Capacity: Do we have enough capacity?
        bool capacity_feasible = route.residualCapacity() >= task.amount();

        // (c) Duration: Can we visit SWTS and still return to landfill within time limit?
        const std::string& landfill_id = problem.getLandfill().id();
        Duration return_time = problem.getTravelTime(task.swtsId(), landfill_id);

        // The effective task service time is the max of TV arrival time and task arrival time
        Duration effective_service_time = std::max(tv_arrival_time, task.arrivalTime());
        Duration total_time = effective_service_time + return_time;

        if (!capacity_feasible) {
          // If capacity is not feasible, we need to return to landfill first
          effective_service_time = std::max(tv_arrival_time + return_time * 2, task.arrivalTime());
          total_time = effective_service_time + return_time;
          route.addLocation(landfill_id, problem);
        }

        bool duration_feasible = total_time <= problem.getTVMaxDuration() + problem.getEpsilon();

        // If feasible, calculate insertion cost (waiting time + travel time)
        if (capacity_feasible && duration_feasible) {
          Duration insertion_cost = waiting_time + travel_time;

          if (!min_insertion_cost.has_value() || insertion_cost < min_insertion_cost.value()) {
            best_vehicle_idx = static_cast<int>(e);
            min_insertion_cost = insertion_cost;
          }
        }
      }

      // Assign task to best vehicle or create a new one
      if (best_vehicle_idx == -1) {
        // Create a new TV route
        TVRoute new_route{
          "TV_" + std::to_string(tv_routes.size() + 1),
          problem.getTVCapacity(),
          problem.getTVMaxDuration()
        };

        // Start from landfill
        new_route.addLocation(problem.getLandfill().id(), problem);

        // Add pickup at SWTS
        bool pickup_success =
          new_route.addPickup(task.swtsId(), task.arrivalTime(), task.amount(), problem);

        if (!pickup_success) {
          throw TVSchedulerError("Failed to add pickup to new route!");
        }

        // Check if we need to return to landfill immediately
        if (new_route.residualCapacity() < q_min) {
          new_route.addLocation(problem.getLandfill().id(), problem);
        }

        tv_routes.push_back(std::move(new_route));
      } else {
        // Add to existing route
        auto& route = tv_routes[best_vehicle_idx];

        bool pickup_success =
          route.addPickup(task.swtsId(), task.arrivalTime(), task.amount(), problem);

        if (!pickup_success) {
          throw TVSchedulerError("Failed to add pickup to existing route!");
        }

        // Check if we need to return to landfill
        if (route.residualCapacity() < q_min) {
          route.addLocation(problem.getLandfill().id(), problem);
        }
      }
    }

    // Finalization: Ensure all routes end at the landfill
    for (auto& route : tv_routes) {
      if (!route.isEmpty() && route.lastLocationId() != problem.getLandfill().id()) {
        route.addLocation(problem.getLandfill().id(), problem);
      }
    }

    // Add TV routes to the solution
    for (auto& route : tv_routes) {
      solution.addTVRoute(std::move(route));
    }

    // Mark the solution as complete
    solution.setComplete(true);

    return solution;
  }

  std::string name() const override { return "Greedy TV Scheduler"; }

  std::string description() const override {
    return "Greedy scheduler for Transportation Vehicles that processes delivery tasks "
           "in chronological order and assigns each to the best available TV";
  }

  std::string timeComplexity() const override {
    return "O(n × m)";  // n = number of tasks, m = number of TV routes
  }
};

// Register the algorithm
REGISTER_ALGORITHM(GreedyTVScheduler, "GreedyTVScheduler");

}  // namespace algorithm
}  // namespace daa
