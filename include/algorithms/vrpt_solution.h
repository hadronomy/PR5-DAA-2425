#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "problem/location.h"
#include "problem/strong_types.h"
#include "problem/vrpt_problem.h"

namespace daa {
namespace algorithm {

/**
 * @brief Represents a waste delivery at a SWTS
 */
class DeliveryTask {
 public:
  DeliveryTask(Capacity amount, std::string swts_id, Duration arrival_time)
      : amount_(amount), swts_id_(std::move(swts_id)), arrival_time_(arrival_time) {}

  [[nodiscard]] const Capacity& amount() const { return amount_; }
  [[nodiscard]] const std::string& swtsId() const { return swts_id_; }
  [[nodiscard]] const Duration& arrivalTime() const { return arrival_time_; }

 private:
  Capacity amount_;        // Amount of waste delivered
  std::string swts_id_;    // SWTS location ID
  Duration arrival_time_;  // Time when the CV arrives
};

/**
 * @brief Class representing a collection vehicle route
 */
class CVRoute {
 public:
  CVRoute(std::string vehicle_id, Capacity capacity, Duration max_duration)
      : vehicle_id_(std::move(vehicle_id)), max_capacity_(capacity), max_duration_(max_duration) {
    // Start with empty load and zero time
    load_profile_.push_back(Capacity{0.0});
    time_profile_.push_back(Duration{0.0});
    duration_profile_.push_back(Duration{0.0});
  }

  void addLocation(const std::string& location_id, const VRPTProblem& problem) {
    // Get the previous location if any
    std::string prev_id = location_ids_.empty() ? problem.getDepot().id() : location_ids_.back();
    const auto& location = problem.getLocation(location_id);

    // Calculate travel time from previous location
    Duration travel_time = problem.getTravelTime(prev_id, location_id);

    // Update total duration
    total_duration_ = total_duration_ + travel_time;

    if (location.type() == LocationType::COLLECTION_ZONE) {
      // Add waste from the zone and service time
      current_load_ = current_load_ + location.wasteAmount();
      total_duration_ = total_duration_ + location.serviceTime();
    } else if (location.type() == LocationType::SWTS) {
      // Record delivery at this SWTS
      deliveries_.emplace_back(current_load_, location_id, total_duration_);

      // Reset load after unloading at SWTS
      current_load_ = Capacity{0.0};
    }

    // Add location to route
    location_ids_.push_back(location_id);

    // Update profiles
    load_profile_.push_back(current_load_);
    time_profile_.push_back(total_duration_);
    duration_profile_.push_back(total_duration_);
  }

  [[nodiscard]] bool canVisit(const std::string& location_id, const VRPTProblem& problem) const {
    // Check capacity constraints
    const auto& location = problem.getLocation(location_id);

    // If this is a collection zone, make sure we have enough capacity
    if (location.type() == LocationType::COLLECTION_ZONE &&
        current_load_ + location.wasteAmount() > max_capacity_) {
      return false;
    }

    // Get the previous location
    std::string prev_id = location_ids_.empty() ? problem.getDepot().id() : location_ids_.back();

    // Calculate travel time to the new location
    Duration travel_time = problem.getTravelTime(prev_id, location_id);
    Duration total_time = total_duration_ + travel_time;

    if (location.type() == LocationType::COLLECTION_ZONE) {
      total_time = total_time + location.serviceTime();
    }

    // Check time to return to depot after visiting this location
    std::optional<Location> nearest_swts;
    if (location.type() != LocationType::SWTS) {
      nearest_swts = problem.findNearest(location, LocationType::SWTS);
    }

    Duration return_time;
    if (nearest_swts) {
      return_time = problem.getTravelTime(location_id, nearest_swts->id()) +
                    problem.getTravelTime(nearest_swts->id(), problem.getDepot().id());
    } else {
      return_time = problem.getTravelTime(location_id, problem.getDepot().id());
    }

    // Check if we can visit location and still return to depot within time limit
    return (total_time + return_time) <= max_duration_;
  }

  // Getters
  [[nodiscard]] const std::vector<std::string>& locationIds() const { return location_ids_; }
  [[nodiscard]] const std::string& vehicleId() const { return vehicle_id_; }
  [[nodiscard]] Capacity currentLoad() const { return current_load_; }
  [[nodiscard]] Duration totalDuration() const { return total_duration_; }
  [[nodiscard]] const std::vector<DeliveryTask>& deliveries() const { return deliveries_; }
  [[nodiscard]] bool isEmpty() const { return location_ids_.empty(); }

  // Get the last location
  [[nodiscard]] std::string lastLocationId() const {
    return location_ids_.empty() ? "" : location_ids_.back();
  }

  // Validate route
  [[nodiscard]] bool isValid(const VRPTProblem& _) const {
    // Check if route is empty
    if (location_ids_.empty()) {
      return true;  // Empty route is considered valid
    }

    // Check that we don't exceed maximum capacity at any point
    for (const auto& load : load_profile_) {
      if (load > max_capacity_) {
        return false;
      }
    }

    // Check that total duration is within limit
    if (total_duration_ > max_duration_) {
      return false;
    }

    return true;
  }

  // Calculate the residual capacity (how much more can be loaded)
  [[nodiscard]] Capacity residualCapacity() const { return max_capacity_ - current_load_; }

  // Calculate the residual time (how much time is left)
  [[nodiscard]] Duration residualTime() const { return max_duration_ - total_duration_; }

 private:
  std::vector<std::string> location_ids_;   // Sequence of location IDs (zones, SWTS, depot)
  std::string vehicle_id_;                  // Vehicle ID
  Capacity max_capacity_;                   // Maximum capacity of the vehicle
  Duration max_duration_;                   // Maximum duration of the route
  Duration total_duration_{0.0};            // Current total duration
  Capacity current_load_{0.0};              // Current load at each step
  std::vector<Capacity> load_profile_;      // Load at each step of the route
  std::vector<Duration> time_profile_;      // Time at each step of the route
  std::vector<Duration> duration_profile_;  // Cumulative duration at each step
  std::vector<DeliveryTask> deliveries_;    // Waste deliveries at SWTS
};

/**
 * @brief Class representing a transportation vehicle route
 */
class TVRoute {
 public:
  TVRoute(std::string vehicle_id, Capacity capacity, Duration max_duration)
      : vehicle_id_(std::move(vehicle_id)), max_capacity_(capacity), max_duration_(max_duration) {
    // Start with zero load and time
    load_profile_.push_back(Capacity{0.0});
    time_profile_.push_back(Duration{0.0});
  }

  bool addPickup(
    const std::string& swts_id,
    const Duration& arrival_time,
    const Capacity& amount,
    const VRPTProblem& problem
  ) {
    // Get the previous location
    std::string prev_id = location_ids_.empty() ? problem.getLandfill().id() : location_ids_.back();

    // Calculate travel time to the SWTS
    Duration travel_time = problem.getTravelTime(prev_id, swts_id);

    // Update current time based on travel time
    Duration new_time = current_time_ + travel_time;

    // Check if we arrive after the CV delivery time
    if (new_time < arrival_time) {
      // Wait until the CV arrives
      new_time = arrival_time;
    }

    // Add SWTS to route
    location_ids_.push_back(swts_id);

    // Update current time and load
    current_time_ = new_time;
    current_load_ = current_load_ + amount;

    // Record pickup details
    pickups_.emplace_back(swts_id, arrival_time);

    // Update profiles
    load_profile_.push_back(current_load_);
    time_profile_.push_back(current_time_);

    return true;
  }

  bool addLocation(const std::string& location_id, const VRPTProblem& problem) {
    // Get the previous location
    std::string prev_id = location_ids_.empty() ? problem.getLandfill().id() : location_ids_.back();

    // Calculate travel time
    Duration travel_time = problem.getTravelTime(prev_id, location_id);

    // Update current time
    current_time_ = current_time_ + travel_time;

    // If this is a landfill, reset the load
    if (location_id == problem.getLandfill().id()) {
      current_load_ = Capacity{0.0};
    }

    // Check time constraint
    // Skip the time constraint check if going to the landfill
    if (location_id != problem.getLandfill().id() &&
        current_time_ > max_duration_ + problem.getEpsilon()) {
      return false;
    }

    // Add location to route
    location_ids_.push_back(location_id);

    // Update profiles
    load_profile_.push_back(current_load_);
    time_profile_.push_back(current_time_);

    return true;
  }

  // Finalize the route by returning to the landfill if needed
  bool finalize(const VRPTProblem& problem) {
    // If already at landfill or empty route, nothing to do
    if (location_ids_.empty() || location_ids_.back() == problem.getLandfill().id()) {
      return true;
    }

    // Add landfill as final destination
    return addLocation(problem.getLandfill().id(), problem);
  }

  // Getters
  [[nodiscard]] const std::vector<std::string>& locationIds() const { return location_ids_; }
  [[nodiscard]] const std::string& vehicleId() const { return vehicle_id_; }
  [[nodiscard]] Capacity currentLoad() const { return current_load_; }
  [[nodiscard]] Duration currentTime() const { return current_time_; }
  [[nodiscard]] const std::vector<std::pair<std::string, Duration>>& pickups() const {
    return pickups_;
  }
  [[nodiscard]] bool isEmpty() const { return location_ids_.empty(); }

  // Get the last location
  [[nodiscard]] std::string lastLocationId() const {
    return location_ids_.empty() ? "" : location_ids_.back();
  }

  // Residual capacity
  [[nodiscard]] Capacity residualCapacity() const { return max_capacity_ - current_load_; }

  // Validate route
  [[nodiscard]] bool isValid(const VRPTProblem& problem) const {
    // Check if route is empty
    if (location_ids_.empty()) {
      return true;
    }

    // Check that we don't exceed maximum capacity at any point
    for (const auto& load : load_profile_) {
      if (load > max_capacity_) {
        return false;
      }
    }

    // Check that total duration is within limit and route ends at landfill
    return current_time_ <= max_duration_ &&
           (location_ids_.empty() || location_ids_.back() == problem.getLandfill().id());
  }

 private:
  std::vector<std::string> location_ids_;  // Sequence of location IDs (SWTS, landfill)
  std::string vehicle_id_;                 // Vehicle ID
  Capacity max_capacity_;                  // Maximum capacity of the vehicle
  Duration max_duration_;                  // Maximum duration of the route
  Duration current_time_{0.0};             // Current time at each step
  Capacity current_load_{0.0};             // Current load at each step
  std::vector<Capacity> load_profile_;     // Load at each step of the route
  std::vector<Duration> time_profile_;     // Time at each step of the route
  std::vector<std::pair<std::string, Duration>> pickups_;  // SWTS pickups with time constraints
};

/**
 * @brief Complete solution for the VRPT problem
 */
class VRPTSolution {
 public:
  // Add a CV route
  void addCVRoute(CVRoute route) { cv_routes_.push_back(std::move(route)); }

  // Add a TV route
  void addTVRoute(TVRoute route) { tv_routes_.push_back(std::move(route)); }

  // Get all delivery tasks from CV routes
  [[nodiscard]] std::vector<DeliveryTask> getAllDeliveryTasks() const {
    std::vector<DeliveryTask> all_tasks;

    for (const auto& route : cv_routes_) {
      const auto& deliveries = route.deliveries();
      all_tasks.insert(all_tasks.end(), deliveries.begin(), deliveries.end());
    }

    // Sort by arrival time
    std::sort(all_tasks.begin(), all_tasks.end(), [](const DeliveryTask& a, const DeliveryTask& b) {
      return a.arrivalTime() < b.arrivalTime();
    });

    return all_tasks;
  }

  // Calculate total duration across all CV routes
  [[nodiscard]] Duration totalDuration() const {
    Duration total{0.0};
    for (const auto& duration : cv_routes_ | std::views::transform(&CVRoute::totalDuration)) {
      total = total + duration;
    }
    return total;
  }

  // Count unique collection zones visited across all routes
  [[nodiscard]] size_t visitedZones(const VRPTProblem& problem) const {
    std::unordered_set<std::string> visited_zones;

    auto is_collection_zone = [&problem](const std::string& id) {
      try {
        return problem.getLocation(id).type() == LocationType::COLLECTION_ZONE;
      } catch (const std::exception&) {
        return false;
      }
    };

    for (const auto& route : cv_routes_) {
      for (const auto& id : route.locationIds() | std::views::filter(is_collection_zone)) {
        visited_zones.insert(id);
      }
    }

    return visited_zones.size();
  }

  // Getters
  [[nodiscard]] const std::vector<CVRoute>& getCVRoutes() const { return cv_routes_; }
  [[nodiscard]] std::vector<CVRoute>& getCVRoutes() { return cv_routes_; }

  [[nodiscard]] const std::vector<TVRoute>& getTVRoutes() const { return tv_routes_; }
  [[nodiscard]] std::vector<TVRoute>& getTVRoutes() { return tv_routes_; }

  [[nodiscard]] bool isComplete() const { return is_complete_; }
  void setComplete(bool complete) { is_complete_ = complete; }

  // Count number of vehicles used
  [[nodiscard]] size_t getCVCount() const { return cv_routes_.size(); }
  [[nodiscard]] size_t getTVCount() const { return tv_routes_.size(); }

  // Calculate total waste collected (for validation)
  [[nodiscard]] Capacity totalWasteCollected() const {
    Capacity total{0.0};
    for (const auto& route : cv_routes_) {
      for (const auto& delivery : route.deliveries()) {
        total = total + delivery.amount();
      }
    }
    return total;
  }

  // Validate the solution
  [[nodiscard]] bool isValid(const VRPTProblem& problem) const {
    // Check if all CV routes are valid
    for (const auto& route : cv_routes_) {
      if (!route.isValid(problem)) {
        return false;
      }
    }

    // If solution is complete, check TV routes
    if (is_complete_) {
      for (const auto& route : tv_routes_) {
        if (!route.isValid(problem)) {
          return false;
        }
      }

      // Ensure all CV deliveries are handled by TV routes
      const auto all_deliveries = getAllDeliveryTasks();
      // TODO: Add validation for TV pickups matching CV deliveries
    }

    return true;
  }

 private:
  std::vector<CVRoute> cv_routes_;  // Collection vehicle routes
  std::vector<TVRoute> tv_routes_;  // Transportation vehicle routes
  bool is_complete_ = false;        // Flag indicating if both phases are solved
};

}  // namespace algorithm
}  // namespace daa
