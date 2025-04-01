#pragma once

#include <algorithm>
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
      bool need_landfill_return = false;

      // Try to find the best existing TV route that can handle this task
      for (size_t e = 0; e < tv_routes.size(); ++e) {
        auto& route = tv_routes[e];

        // Check feasibility conditions
        const std::string& last_location = route.lastLocationId();

        Duration travel_time = (last_location.empty())
                               ? Duration{0.0}
                               : problem.getTravelTime(last_location, task.swtsId());

        // (a) Timing: We can always handle the task regardless of when we arrive
        // If we arrive early, we'll wait; if we arrive late, the task is missed
        Duration tv_arrival_time = route.currentTime() + travel_time;
        Duration waiting_time = Duration{0.0};

        if (tv_arrival_time <= task.arrivalTime()) {
          // TV arrives before the task, needs to wait
          waiting_time = task.arrivalTime() - tv_arrival_time;
        } else {
          // If we arrive late, this route is not feasible
          continue;
        }

        // (b) Capacity: Do we have enough capacity?
        bool capacity_feasible = route.residualCapacity() >= task.amount();

        // If we have significant waiting time, consider going to landfill and back
        bool can_visit_landfill_during_wait = false;
        Duration landfill_detour_time = Duration{0.0};

        const std::string& landfill_id = problem.getLandfill().id();

        if (!capacity_feasible && waiting_time > Duration{0.0}) {
          // Calculate time needed for landfill detour
          Duration to_landfill = problem.getTravelTime(last_location, landfill_id);
          Duration from_landfill = problem.getTravelTime(landfill_id, task.swtsId());
          landfill_detour_time = to_landfill + from_landfill;

          // Check if we can visit landfill during waiting time
          if (landfill_detour_time <= waiting_time) {
            can_visit_landfill_during_wait = true;
            capacity_feasible = true;  // After landfill visit, we have full capacity

            // Adjust waiting time to account for landfill detour
            waiting_time = waiting_time - landfill_detour_time;
          }
        }

        // (c) Duration: Can we visit SWTS and still return to landfill within time limit?
        Duration return_time = problem.getTravelTime(task.swtsId(), landfill_id);

        // The effective task service time is the max of TV arrival time and task arrival time
        Duration effective_service_time;
        Duration total_time;

        if (can_visit_landfill_during_wait) {
          // If we're visiting landfill during wait, adjust times accordingly
          effective_service_time = task.arrivalTime();
          total_time = effective_service_time + return_time;
        } else if (!capacity_feasible) {
          // If capacity is not feasible and we can't use waiting time for landfill,
          // we need to go to landfill first and then to the task
          Duration to_landfill = problem.getTravelTime(last_location, landfill_id);
          Duration from_landfill = problem.getTravelTime(landfill_id, task.swtsId());

          tv_arrival_time = route.currentTime() + to_landfill + from_landfill;

          if (tv_arrival_time <= task.arrivalTime()) {
            effective_service_time = task.arrivalTime();
            total_time = effective_service_time + return_time;
            capacity_feasible = true;  // After landfill visit, we have full capacity
            need_landfill_return = true;
          } else {
            // We can't make it in time even with landfill visit
            continue;
          }
        } else {
          // Normal case: go directly to task
          effective_service_time = std::max(tv_arrival_time, task.arrivalTime());
          total_time = effective_service_time + return_time;
        }

        bool duration_feasible = total_time <= problem.getTVMaxDuration() + problem.getEpsilon();

        // If feasible, calculate insertion cost (waiting time + travel time)
        if (capacity_feasible && duration_feasible) {
          // For cost calculation, consider utilization - prefer vehicles that are already in use
          Duration insertion_cost = travel_time;

          // Look ahead to see if this vehicle will be useful for future tasks
          bool good_for_future = false;
          if (i < tasks.size() - 1) {
            const auto& next_task = tasks[i + 1];
            Duration time_to_next = next_task.arrivalTime() - effective_service_time;
            Duration travel_to_next = problem.getTravelTime(task.swtsId(), next_task.swtsId());

            if (travel_to_next <= time_to_next &&
                route.residualCapacity() - task.amount() >= next_task.amount()) {
              good_for_future = true;
            }
          }

          // Adjust insertion cost based on vehicle utilization and future potential
          if (good_for_future) {
            insertion_cost = insertion_cost * 0.8;  // Prefer vehicles useful for future tasks
          }

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

        // Only return to landfill if capacity is very low or if it's the last task
        bool return_to_landfill = new_route.residualCapacity() < q_min || (i == tasks.size() - 1);

        // Also consider returning if next task is far in the future
        if (i < tasks.size() - 1) {
          const auto& next_task = tasks[i + 1];
          Duration time_to_next = next_task.arrivalTime() - task.arrivalTime();
          Duration to_landfill = problem.getTravelTime(task.swtsId(), problem.getLandfill().id());
          Duration from_landfill =
            problem.getTravelTime(problem.getLandfill().id(), next_task.swtsId());

          if (to_landfill + from_landfill <= time_to_next) {
            return_to_landfill = true;
          }
        }

        if (return_to_landfill) {
          new_route.addLocation(problem.getLandfill().id(), problem);
        }

        tv_routes.push_back(std::move(new_route));
      } else {
        // Add to existing route
        auto& route = tv_routes[best_vehicle_idx];
        const std::string& last_location = route.lastLocationId();

        // Check if we need to visit landfill first due to capacity
        if (need_landfill_return || route.residualCapacity() < task.amount()) {
          route.addLocation(problem.getLandfill().id(), problem);
        } else if (!last_location.empty() && last_location != task.swtsId()) {
          // Check if we have significant waiting time to visit landfill
          Duration travel_time = problem.getTravelTime(last_location, task.swtsId());
          Duration tv_arrival_time = route.currentTime() + travel_time;

          if (tv_arrival_time < task.arrivalTime()) {
            Duration waiting_time = task.arrivalTime() - tv_arrival_time;
            const std::string& landfill_id = problem.getLandfill().id();

            Duration to_landfill = problem.getTravelTime(last_location, landfill_id);
            Duration from_landfill = problem.getTravelTime(landfill_id, task.swtsId());

            if (to_landfill + from_landfill <= waiting_time) {
              // Use waiting time to visit landfill
              route.addLocation(landfill_id, problem);
            }
          }
        }

        bool pickup_success =
          route.addPickup(task.swtsId(), task.arrivalTime(), task.amount(), problem);

        if (!pickup_success) {
          throw TVSchedulerError("Failed to add pickup to existing route!");
        }

        // Consider whether to return to landfill after this task
        // Look ahead to next task to decide
        bool return_to_landfill = route.residualCapacity() < q_min || (i == tasks.size() - 1);

        // Consider returning if it's beneficial for the next task
        if (i < tasks.size() - 1 && !return_to_landfill) {
          const auto& next_task = tasks[i + 1];
          Duration time_to_next = next_task.arrivalTime() - task.arrivalTime();
          Duration to_landfill = problem.getTravelTime(task.swtsId(), problem.getLandfill().id());
          Duration from_landfill =
            problem.getTravelTime(problem.getLandfill().id(), next_task.swtsId());
          Duration direct_to_next = problem.getTravelTime(task.swtsId(), next_task.swtsId());

          // Return to landfill if:
          // 1. It fits within the time window, and
          // 2. Either we need more capacity for next task or it's more efficient
          if (to_landfill + from_landfill <= time_to_next &&
              (route.residualCapacity() < next_task.amount() ||
               to_landfill + from_landfill < direct_to_next)) {
            return_to_landfill = true;
          }
        }

        if (return_to_landfill) {
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
