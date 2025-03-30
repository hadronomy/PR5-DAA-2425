#pragma once

#include <algorithm>
#include <limits>
#include <memory>
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
   * @brief Default constructor
   */
  GreedyTVScheduler() = default;

  /**
   * @brief Constructor with problem reference
   * @param problem Reference to the problem instance
   */
  explicit GreedyTVScheduler(const VRPTProblem& problem) : problem_(&problem) {}

  /**
   * @brief Solve Phase 2 of the VRPT problem
   *
   * @param input Solution from Phase 1 containing CV routes
   * @return Complete solution with both CV and TV routes
   * @throws TVSchedulerError If scheduling is infeasible
   */
  VRPTSolution solve(const VRPTSolution& input) override {
    // Create a copy of the input solution
    VRPTSolution solution = input;

    // Get the problem instance
    const VRPTProblem& problem = getProblem();

    // Get all delivery tasks from CV routes, sorted by arrival time
    std::vector<DeliveryTask> tasks = solution.getAllDeliveryTasks();

    // Check if there are any tasks
    if (tasks.empty()) {
      solution.setComplete(true);
      return solution;
    }

    // Process all tasks and create TV routes
    std::vector<TVRoute> tv_routes = createTVRoutes(tasks, problem);

    // Finalize all routes - make sure they end at the landfill
    for (auto& route : tv_routes) {
      if (!route.finalize(problem)) {
        throw TVSchedulerError(
          "Failed to finalize TV route - cannot return to landfill within time constraints"
        );
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

  /**
   * @brief Get the problem instance from the current thread context
   *
   * This is a helper method to access the problem instance.
   *
   * @return Reference to the problem instance
   * @throws TVSchedulerError if the problem instance is not available
   */
  virtual const VRPTProblem& getProblem() const {
    if (!problem_) {
      throw TVSchedulerError(
        "Problem instance not available - use constructor with problem reference"
      );
    }
    return *problem_;
  }

  std::string name() const override { return "Greedy TV Scheduler"; }

  std::string description() const override {
    return "Greedy scheduler for Transportation Vehicles that processes delivery tasks "
           "in chronological order and assigns each to the best available TV";
  }

  std::string timeComplexity() const override {
    return "O(n × m)";  // n = number of tasks, m = number of TV routes
  }

 private:
  /**
   * @brief Create TV routes from delivery tasks
   *
   * @param tasks Sorted delivery tasks
   * @param problem The problem instance
   * @return Vector of TV routes
   */
  std::vector<TVRoute>
    createTVRoutes(const std::vector<DeliveryTask>& tasks, const VRPTProblem& problem) const {

    std::vector<TVRoute> tv_routes;

    // Find minimum waste amount for deciding when to return to landfill
    Capacity q_min = findMinimumWasteAmount(tasks);

    // Parameters for TVs
    const Capacity tv_capacity = problem.getTVCapacity();
    const Duration tv_max_duration = problem.getTVMaxDuration();

    // Process each task in chronological order
    for (const auto& task : tasks) {
      std::optional<std::pair<size_t, double>> best_vehicle =
        findBestVehicle(task, tv_routes, problem);

      if (!best_vehicle) {
        // No existing vehicle can handle this task, create a new one
        createNewRoute(task, tv_routes, q_min, tv_capacity, tv_max_duration, problem);
      } else {
        // Add to existing vehicle
        const size_t best_vehicle_idx = best_vehicle->first;

        if (!addTaskToExistingRoute(task, tv_routes[best_vehicle_idx], q_min, problem)) {
          // If pickup failed unexpectedly, create a new route
          createNewRoute(task, tv_routes, q_min, tv_capacity, tv_max_duration, problem);
        }
      }
    }

    return tv_routes;
  }

  /**
   * @brief Find the minimum waste amount from tasks
   *
   * @param tasks The delivery tasks
   * @return The minimum waste capacity
   */
  Capacity findMinimumWasteAmount(const std::vector<DeliveryTask>& tasks) const {
    double min_waste_amount = std::numeric_limits<double>::max();
    for (const auto& task : tasks) {
      min_waste_amount = std::min(min_waste_amount, task.amount().value());
    }
    return Capacity{min_waste_amount};
  }

  /**
   * @brief Find the best vehicle to handle a task
   *
   * @param task The delivery task
   * @param tv_routes Current TV routes
   * @param problem The problem instance
   * @return Optional pair with best vehicle index and insertion cost, or nullopt if no suitable
   * vehicle found
   */
  std::optional<std::pair<size_t, double>> findBestVehicle(
    const DeliveryTask& task,
    const std::vector<TVRoute>& tv_routes,
    const VRPTProblem& problem
  ) const {

    int best_vehicle_idx = -1;
    double min_insertion_cost = std::numeric_limits<double>::max();

    // Check each existing TV route
    for (size_t i = 0; i < tv_routes.size(); ++i) {
      const TVRoute& route = tv_routes[i];

      // Check feasibility conditions
      if (!isTaskFeasibleForRoute(task, route, problem)) {
        continue;
      }

      // Calculate insertion cost
      std::string last_location =
        route.isEmpty() ? problem.getLandfill().id() : route.lastLocationId();
      Duration travel_time = problem.getTravelTime(last_location, task.swtsId());
      double insertion_cost = travel_time.value();

      // If this is better than the current best, update
      if (insertion_cost < min_insertion_cost) {
        min_insertion_cost = insertion_cost;
        best_vehicle_idx = static_cast<int>(i);
      }
    }

    if (best_vehicle_idx == -1) {
      return std::nullopt;
    }

    return std::make_pair(best_vehicle_idx, min_insertion_cost);
  }

  /**
   * @brief Check if a task is feasible for a route
   *
   * @param task The delivery task
   * @param route The TV route
   * @param problem The problem instance
   * @return true if the task can be added to the route, false otherwise
   */
  bool isTaskFeasibleForRoute(
    const DeliveryTask& task,
    const TVRoute& route,
    const VRPTProblem& problem
  ) const {

    // Check capacity feasibility
    if (route.residualCapacity() < task.amount()) {
      return false;
    }

    // Check time feasibility - can we reach the SWTS in time?
    if (!route.isEmpty()) {
      std::string last_location = route.lastLocationId();
      Duration travel_time = problem.getTravelTime(last_location, task.swtsId());
      Duration current_time = route.currentTime();

      // We must arrive at or before the CV delivery time to avoid waste sitting
      if (current_time + travel_time > task.arrivalTime()) {
        return false;
      }
    }

    // Check duration constraint
    std::string last_location =
      route.isEmpty() ? problem.getLandfill().id() : route.lastLocationId();

    // Estimate total route duration after adding this task
    Duration travel_to_swts = problem.getTravelTime(last_location, task.swtsId());
    Duration travel_to_landfill = problem.getTravelTime(task.swtsId(), problem.getLandfill().id());
    Duration estimated_total_duration = route.currentTime() + travel_to_swts + travel_to_landfill;

    return estimated_total_duration <= problem.getTVMaxDuration();
  }

  /**
   * @brief Add a task to an existing route
   *
   * @param task The delivery task
   * @param route The TV route to modify
   * @param q_min Minimum waste amount for landfill visits
   * @param problem The problem instance
   * @return true if the task was added successfully, false otherwise
   */
  bool addTaskToExistingRoute(
    const DeliveryTask& task,
    TVRoute& route,
    const Capacity& q_min,
    const VRPTProblem& problem
  ) const {

    // Add pickup at the SWTS
    if (!route.addPickup(task.swtsId(), task.arrivalTime(), task.amount(), problem)) {
      return false;
    }

    // Check if we need to visit the landfill based on min waste amount
    if (route.currentLoad() < q_min) {
      if (!route.addLocation(problem.getLandfill().id(), problem)) {
        return false;
      }
    }

    return true;
  }

  /**
   * @brief Create a new TV route for a task
   *
   * @param task The delivery task
   * @param tv_routes Vector of routes to add the new route to
   * @param q_min Minimum waste amount for landfill visits
   * @param tv_capacity TV capacity
   * @param tv_max_duration Maximum TV route duration
   * @param problem The problem instance
   * @throws TVSchedulerError if the task cannot be assigned to a new route
   */
  void createNewRoute(
    const DeliveryTask& task,
    std::vector<TVRoute>& tv_routes,
    const Capacity& q_min,
    const Capacity& tv_capacity,
    const Duration& tv_max_duration,
    const VRPTProblem& problem
  ) const {

    // Create a new TV route
    std::string vehicle_id = "TV" + std::to_string(tv_routes.size() + 1);
    TVRoute new_route(vehicle_id, tv_capacity, tv_max_duration);

    // Explicitly start the route at the landfill
    std::string landfill_id = problem.getLandfill().id();
    if (new_route.isEmpty() && !new_route.addLocation(landfill_id, problem)) {
      throw TVSchedulerError("Failed to add landfill as starting location for new TV route");
    }

    // Check if the time constraints make this task inherently infeasible
    // Calculate time to reach SWTS and then return to landfill
    Duration travel_time_to_swts = problem.getTravelTime(landfill_id, task.swtsId());
    Duration travel_time_back = problem.getTravelTime(task.swtsId(), landfill_id);
    Duration total_route_time = travel_time_to_swts + travel_time_back;

    if (total_route_time > tv_max_duration) {
      throw TVSchedulerError(
        "Task infeasible: Cannot service SWTS " + task.swtsId() +
        " with any vehicle - total minimum route time " + std::to_string(total_route_time.value()) +
        " exceeds maximum duration " + std::to_string(tv_max_duration.value())
      );
    }

    // Check if arrival time constraint makes this task infeasible
    Duration required_departure_time = task.arrivalTime() - travel_time_to_swts;

    // If we need to leave before time 0, the task is infeasible
    if (required_departure_time.value() < 0) {
      throw TVSchedulerError(
        "Task infeasible: Cannot reach SWTS " + task.swtsId() + " in time for task arriving at " +
        std::to_string(task.arrivalTime().value())
      );
    }

    // Check if arrival time plus return time exceeds max duration
    // This can happen if the task arrives late in the schedule
    if (task.arrivalTime() + travel_time_back > tv_max_duration) {
      throw TVSchedulerError(
        "Task infeasible: Task arriving at time " + std::to_string(task.arrivalTime().value()) +
        " cannot be serviced within time limit even by a new vehicle. " +
        "Task arrival + return time (" +
        std::to_string((task.arrivalTime() + travel_time_back).value()) +
        ") exceeds maximum duration (" + std::to_string(tv_max_duration.value()) + ")"
      );
    }

    // Try to add pickup at the SWTS
    if (!new_route.addPickup(task.swtsId(), task.arrivalTime(), task.amount(), problem)) {
      throw TVSchedulerError(
        "Failed to add pickup to new route for task at SWTS " + task.swtsId() + " with amount " +
        std::to_string(task.amount().value()) + " arriving at time " +
        std::to_string(task.arrivalTime().value())
      );
    }

    // Check if we need to visit the landfill based on min waste amount
    if (new_route.currentLoad() < q_min &&
        !new_route.addLocation(problem.getLandfill().id(), problem)) {
      throw TVSchedulerError("Failed to add landfill visit after pickup");
    }

    // Add the new route
    tv_routes.push_back(std::move(new_route));
  }

  // Pointer to the problem instance
  const VRPTProblem* problem_ = nullptr;
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
  VRPTSolver(
    std::string cv_algorithm = "MultiStart",
    std::string tv_algorithm = "GreedyTVScheduler"
  )
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

    // Phase 2: Create TV scheduler with problem reference
    GreedyTVScheduler scheduler(problem);
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
