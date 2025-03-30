#pragma once

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "algorithm_registry.h"
#include "algorithms/vrpt_solution.h"
#include "problem/vrpt_problem.h"

namespace daa {
namespace algorithm {

/**
 * @brief Greedy algorithm for scheduling Transportation Vehicle routes (Phase 2)
 *
 * Based on Algorithms 3 & 4 in the paper, this algorithm takes CV routes as input
 * and computes TV routes to complete the VRPT solution. It processes delivery tasks
 * in chronological order and assigns each to the best available TV or creates a new one.
 */
class GreedyTVScheduler : public TypedAlgorithm<VRPTSolution, VRPTSolution> {
 public:
  /**
   * @brief Solve Phase 2 of the VRPT problem
   *
   * @param input Solution from Phase 1 containing CV routes
   * @return Complete solution with both CV and TV routes
   */
  VRPTSolution solve(const VRPTSolution& input) override {
    // Create a copy of the input solution
    VRPTSolution solution = input;

    // Get the problem instance from the current thread context
    // This is somewhat of a workaround as we don't have direct access to the problem
    // In a real implementation, we might want to pass the problem directly
    const VRPTProblem& problem = *reinterpret_cast<const VRPTProblem*>(getProblemFromContext());

    // Get all delivery tasks from CV routes, sorted by arrival time
    std::vector<DeliveryTask> tasks = solution.getAllDeliveryTasks();

    // Check if there are any tasks
    if (tasks.empty()) {
      solution.setComplete(true);
      return solution;
    }

    // Initialize TV routes
    std::vector<TVRoute> tv_routes;

    // Find minimum waste amount for deciding when to return to landfill
    double min_waste_amount = std::numeric_limits<double>::max();
    for (const auto& task : tasks) {
      min_waste_amount = std::min(min_waste_amount, task.amount().value());
    }
    Capacity q_min{min_waste_amount};

    // Parameters for TVs
    Capacity tv_capacity = problem.getTVCapacity();
    Duration tv_max_duration = problem.getTVMaxDuration();

    // Process each task in chronological order
    for (const auto& task : tasks) {
      // Find the best vehicle for this task
      int best_vehicle_idx = -1;
      double min_insertion_cost = std::numeric_limits<double>::max();

      // Check each existing TV route
      for (size_t i = 0; i < tv_routes.size(); ++i) {
        TVRoute& route = tv_routes[i];

        // Check if this TV can handle the task
        bool time_feasible = true;
        bool capacity_feasible = route.residualCapacity() >= task.amount();

        // Check time feasibility - can we reach the SWTS in time?
        if (!route.isEmpty()) {
          std::string last_location = route.lastLocationId();
          Duration travel_time = problem.getTravelTime(last_location, task.swtsId());
          Duration current_time = route.currentTime();

          // We must arrive at or before the CV delivery time to avoid waste sitting
          if (current_time + travel_time > task.arrivalTime()) {
            time_feasible = false;
          }
        }

        // Check if we can add the task and still return to landfill
        if (time_feasible && capacity_feasible) {
          // Calculate insertion cost (additional time)
          std::string last_location =
            route.isEmpty() ? problem.getLandfill().id() : route.lastLocationId();
          Duration travel_time = problem.getTravelTime(last_location, task.swtsId());

          // The cost is the additional distance/time
          double insertion_cost = travel_time.value();

          // If this is better than the current best, update
          if (insertion_cost < min_insertion_cost) {
            min_insertion_cost = insertion_cost;
            best_vehicle_idx = static_cast<int>(i);
          }
        }
      }

      // Assign the task to the best vehicle or create a new one
      if (best_vehicle_idx == -1) {
        // No existing vehicle can handle this task, create a new one
        std::string vehicle_id = "TV" + std::to_string(tv_routes.size() + 1);
        TVRoute new_route(vehicle_id, tv_capacity, tv_max_duration);

        // Add pickup at the SWTS
        if (new_route.addPickup(task.swtsId(), task.arrivalTime(), task.amount(), problem)) {
          // Check if we need to visit the landfill based on min waste amount
          if (new_route.currentLoad() < q_min) {
            new_route.addLocation(problem.getLandfill().id(), problem);
          }

          // Add the new route
          tv_routes.push_back(std::move(new_route));
        } else {
          // This should not happen with a new route, but handle error case
          throw std::runtime_error("Failed to add pickup to new route");
        }
      } else {
        // Add to existing vehicle
        TVRoute& best_route = tv_routes[best_vehicle_idx];

        // Add pickup at the SWTS
        if (best_route.addPickup(task.swtsId(), task.arrivalTime(), task.amount(), problem)) {
          // Check if we need to visit the landfill based on min waste amount
          if (best_route.currentLoad() < q_min) {
            best_route.addLocation(problem.getLandfill().id(), problem);
          }
        } else {
          // This should not happen as we checked feasibility, but handle error case
          throw std::runtime_error("Failed to add pickup to selected route");
        }
      }
    }

    // Finalize all routes - make sure they end at the landfill
    for (auto& route : tv_routes) {
      route.finalize(problem);
    }

    // Add TV routes to the solution
    for (auto& route : tv_routes) {
      solution.addTVRoute(std::move(route));
    }

    // Mark the solution as complete
    solution.setComplete(true);

    return solution;
  }

  /**
   * @brief Get the problem instance from the current thread context
   *
   * This is a helper method to access the problem instance. In a real implementation,
   * this would be replaced by a more direct approach.
   */
  virtual const void* getProblemFromContext() const {
    // This is just a placeholder for demonstration
    // In a real implementation, we would either:
    // 1. Pass the problem directly to the solve method
    // 2. Store it in the scheduler when it's created
    // 3. Access it through some context mechanism
    throw std::runtime_error("Problem instance not available - this is a demonstration only");
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

/**
 * @brief Complete VRPT solver that combines Phase 1 and Phase 2
 *
 * This algorithm ties together the CV routing (Phase 1) and TV routing (Phase 2)
 * to provide a complete solution to the VRPT problem.
 */
class VRPTSolver : public TypedAlgorithm<VRPTProblem, VRPTSolution> {
 public:
  /**
   * @brief Constructor with algorithm selection
   *
   * @param cv_algorithm The algorithm to use for CV routing (Phase 1)
   * @param tv_algorithm The algorithm to use for TV routing (Phase 2)
   */
  VRPTSolver(std::string cv_algorithm = "GVNS", std::string tv_algorithm = "GreedyTVScheduler")
      : cv_algorithm_(std::move(cv_algorithm)), tv_algorithm_(std::move(tv_algorithm)) {}

  /**
   * @brief Solve the complete VRPT problem
   *
   * @param problem The problem instance
   * @return A complete solution with both CV and TV routes
   */
  VRPTSolution solve(const VRPTProblem& problem) override {
    // Phase 1: Solve the CV routing problem
    auto cv_solver = AlgorithmRegistry::createTyped<VRPTProblem, VRPTSolution>(cv_algorithm_);
    VRPTSolution cv_solution = cv_solver->solve(problem);

    // Phase 2: Solve the TV routing problem using the CV solution
    auto tv_solver_base = AlgorithmRegistry::create(tv_algorithm_);
    auto* tv_solver = dynamic_cast<GreedyTVScheduler*>(tv_solver_base.get());

    if (!tv_solver) {
      throw std::runtime_error("Failed to cast TV solver to GreedyTVScheduler");
    }

    // Create a custom TV scheduler that has access to the problem
    class ConcreteScheduler : public GreedyTVScheduler {
     public:
      explicit ConcreteScheduler(const VRPTProblem& prob) : problem_(&prob) {}

      const void* getProblemFromContext() const override { return problem_; }

     private:
      const VRPTProblem* problem_;
    };

    ConcreteScheduler scheduler(problem);
    VRPTSolution final_solution = scheduler.solve(cv_solution);

    return final_solution;
  }

  std::string name() const override {
    return "VRPT Solver (" + cv_algorithm_ + " + " + tv_algorithm_ + ")";
  }

  std::string description() const override {
    return "Complete VRPT solver that uses " + cv_algorithm_ +
           " for Collection Vehicle routing "
           "and " +
           tv_algorithm_ + " for Transportation Vehicle routing";
  }

  std::string timeComplexity() const override {
    return "O(CV + TV)";  // Complexity depends on the underlying algorithms
  }

 private:
  std::string cv_algorithm_;
  std::string tv_algorithm_;
};

// Register the complete solver
REGISTER_ALGORITHM(VRPTSolver, "VRPTSolver");

}  // namespace algorithm
}  // namespace daa
