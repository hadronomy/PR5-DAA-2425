#pragma once

#include <cmath>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "concepts.h"
#include "kdtree.h"
#include "location.h"
#include "parser/vrpt_driver.h"
#include "problem/units.h"
#include "strong_types.h"

// Forward declaration for SolutionType
namespace daa {
namespace algorithm {
class VRPTSolution;
}
}  // namespace daa

namespace daa {

/**
 * @class VRPTProblem
 * @brief Class representing the Vehicle Routing Problem with Transshipments for Solid Waste
 * Collection
 */
class VRPTProblem {
 public:
  // Required for meta::Problem concept
  using SolutionType = algorithm::VRPTSolution;

 private:
  // Problem parameters
  Duration cv_max_duration_{0.0};  // L1: Max duration for collection vehicles
  Duration tv_max_duration_{0.0};  // L2: Max duration for transportation vehicles
  int num_cv_vehicles_{0};         // Number of collection vehicles
  int num_zones_{0};               // Number of collection zones
  double map_width_{0.0};          // Map width (Lx)
  double map_height_{0.0};         // Map height (Ly)
  Capacity cv_capacity_{0.0};      // Q1: Collection vehicle capacity
  Capacity tv_capacity_{0.0};      // Q2: Transportation vehicle capacity
  Speed vehicle_speed_{
    0.0,
    units::DistanceUnit::Kilometers,
    units::TimeUnit::Hours
  };  // V: Vehicle speed (km/h)
  double epsilon_{0.0};  // Epsilon parameter
  double offset_{0.0};   // Offset parameter
  int k_param_{0};       // k parameter

  // KDTree for spatial queries
  KDTree location_tree_;

  // Specialized location IDs for quick access
  std::string depot_id_;
  std::string landfill_id_;
  std::vector<std::string> swts_ids_;
  std::vector<std::string> zone_ids_;

 public:
  // Default constructor
  VRPTProblem() = default;

  static std::optional<VRPTProblem> loadFile(const std::string& filepath) {
    VRPTProblem problem;
    if (!problem.loadFromFile(filepath)) {
      return std::nullopt;
    }
    return problem;
  }

  /**
   * @brief Load problem from a file
   * @param filepath Path to the input file
   * @return True if loading was successful, false otherwise
   */
  bool loadFromFile(const std::string& filepath) {
    try {
      // Use the VRPT parser to parse the file
      VRPTDriver driver;
      if (!driver.parse_file(filepath)) {
        std::cerr << "Failed to parse file: " << filepath << std::endl;
        return false;
      }

      // Clear previous data
      depot_id_.clear();
      landfill_id_.clear();
      swts_ids_.clear();
      zone_ids_.clear();

      // Temporary storage for locations before building the KDTree
      std::vector<Location> locations;

      // Set parameters
      cv_max_duration_ = Duration{driver.parameters.l1, units::TimeUnit::Minutes};
      tv_max_duration_ = Duration{driver.parameters.l2, units::TimeUnit::Minutes};
      num_cv_vehicles_ = driver.parameters.num_vehicles;
      num_zones_ = driver.parameters.num_zones;
      map_width_ = driver.parameters.map_width;
      map_height_ = driver.parameters.map_height;
      cv_capacity_ = Capacity{driver.parameters.q1};
      tv_capacity_ = Capacity{driver.parameters.q2};
      vehicle_speed_ = Speed{
        driver.parameters.vehicle_speed,
        units::DistanceUnit::Kilometers,
        units::TimeUnit::Hours,
      };
      epsilon_ = driver.parameters.epsilon;
      offset_ = driver.parameters.offset;
      k_param_ = driver.parameters.k_param;

      // Process locations
      for (const auto& loc : driver.locations) {
        if (loc.type == "Depot") {
          depot_id_ = "depot";
          auto depot = Location::Builder()
                         .setId(depot_id_)
                         .setCoordinates(loc.x * 1000.0, loc.y * 1000.0)  // Convert km to m
                         .setType(LocationType::DEPOT)
                         .setName("Depot")
                         .setServiceTime(Duration{0.0, units::TimeUnit::Minutes})
                         .setWasteAmount(Capacity{0.0})
                         .build();

          locations.push_back(depot);
        } else if (loc.type == "Dumpsite") {
          landfill_id_ = "landfill";
          auto landfill = Location::Builder()
                            .setId(landfill_id_)
                            .setCoordinates(loc.x * 1000.0, loc.y * 1000.0)  // Convert km to m
                            .setType(LocationType::LANDFILL)
                            .setName("Landfill")
                            .setServiceTime(Duration{0.0, units::TimeUnit::Minutes})
                            .setWasteAmount(Capacity{0.0})
                            .build();

          locations.push_back(landfill);
        } else if (loc.type == "IF" || loc.type == "IF1") {
          std::string swts_id = "swts_" + loc.type;
          auto swts = Location::Builder()
                        .setId(swts_id)
                        .setCoordinates(loc.x * 1000.0, loc.y * 1000.0)  // Convert km to m
                        .setType(LocationType::SWTS)
                        .setName("SWTS " + loc.type)
                        .setServiceTime(Duration{0.0, units::TimeUnit::Minutes})
                        .setWasteAmount(Capacity{0.0})
                        .build();

          locations.push_back(swts);
          swts_ids_.push_back(swts_id);
        }
      }

      // Process zones
      for (const auto& zone : driver.zones) {
        std::string id = "zone_" + std::to_string(zone.id);
        auto zone_loc = Location::Builder()
                          .setId(id)
                          .setCoordinates(zone.x * 1000.0, zone.y * 1000.0)  // Convert km to m
                          .setType(LocationType::COLLECTION_ZONE)
                          .setName("Zone " + std::to_string(zone.id))
                          .setServiceTime(Duration{zone.service_time, units::TimeUnit::Minutes})
                          .setWasteAmount(Capacity{zone.waste_amount})
                          .build();

        locations.push_back(zone_loc);
        zone_ids_.push_back(id);
      }

      // Build the KDTree with all locations
      location_tree_.build(std::move(locations));

      // Check if we have all the required elements
      if (depot_id_.empty() || landfill_id_.empty() || swts_ids_.empty() || zone_ids_.empty()) {
        std::cerr << "Error: Missing required elements in problem definition" << std::endl;
        return false;
      }

      return true;
    } catch (const std::exception& e) {
      std::cerr << "Error loading problem: " << e.what() << std::endl;
      return false;
    }
  }

  /**
   * @brief Evaluate solution quality - primary objective is to minimize the number of CV vehicles
   * @param solution The solution to evaluate
   * @return Solution cost (lower is better)
   */
  double evaluateSolution(const algorithm::VRPTSolution& solution) const;

  // Getters for problem parameters
  [[nodiscard]] Duration getCVMaxDuration() const noexcept { return cv_max_duration_; }
  [[nodiscard]] Duration getTVMaxDuration() const noexcept { return tv_max_duration_; }
  [[nodiscard]] int getNumCVVehicles() const noexcept { return num_cv_vehicles_; }
  [[nodiscard]] int getNumZones() const noexcept { return num_zones_; }
  [[nodiscard]] Capacity getCVCapacity() const noexcept { return cv_capacity_; }
  [[nodiscard]] Capacity getTVCapacity() const noexcept { return tv_capacity_; }
  [[nodiscard]] Speed getVehicleSpeed() const noexcept { return vehicle_speed_; }

  /**
   * @brief Get the depot location
   * @return The depot location
   * @throws std::runtime_error if depot not found
   */
  [[nodiscard]] const Location& getDepot() const {
    if (depot_id_.empty()) {
      throw std::runtime_error("Depot not found");
    }
    return location_tree_.getLocations().at(depot_id_);
  }

  /**
   * @brief Get the landfill location
   * @return The landfill location
   * @throws std::runtime_error if landfill not found
   */
  [[nodiscard]] const Location& getLandfill() const {
    if (landfill_id_.empty()) {
      throw std::runtime_error("Landfill not found");
    }
    return location_tree_.getLocations().at(landfill_id_);
  }

  /**
   * @brief Get all SWTS locations
   * @return Vector of SWTS locations
   */
  [[nodiscard]] std::vector<Location> getSWTS() const {
    std::vector<Location> result;
    const auto& locations = location_tree_.getLocations();

    for (const auto& id : swts_ids_) {
      auto it = locations.find(id);
      if (it != locations.end()) {
        result.push_back(it->second);
      }
    }

    return result;
  }

  /**
   * @brief Get all collection zone locations
   * @return Vector of collection zone locations
   */
  [[nodiscard]] std::vector<Location> getZones() const {
    std::vector<Location> result;
    const auto& locations = location_tree_.getLocations();

    for (const auto& id : zone_ids_) {
      auto it = locations.find(id);
      if (it != locations.end()) {
        result.push_back(it->second);
      }
    }

    return result;
  }

  /**
   * @brief Get a specific location by ID
   * @param id The location ID
   * @return The location
   * @throws std::runtime_error if location not found
   */
  [[nodiscard]] const Location& getLocation(const std::string& id) const {
    const auto& locations = location_tree_.getLocations();
    auto it = locations.find(id);
    if (it == locations.end()) {
      throw std::runtime_error("Location not found: " + id);
    }
    return it->second;
  }

  /**
   * @brief Find nearest location of a specific type
   * @param from Source location
   * @param type Type of location to find
   * @return Nearest location of specified type or nullopt if not found
   */
  [[nodiscard]] std::optional<Location> findNearest(const Location& from, LocationType type) const {
    return location_tree_.findNearest(from, type);
  }

  /**
   * @brief Find k nearest locations of a specific type
   * @param from Source location
   * @param type Type of location to find
   * @param k Number of nearest neighbors to find
   * @return Vector of k nearest locations of the specified type
   */
  [[nodiscard]] std::vector<Location>
    findKNearest(const Location& from, LocationType type, size_t k) const {
    return location_tree_.findKNearest(from, type, k);
  }

  /**
   * @brief Get distance between two locations
   * @param from_id Source location ID
   * @param to_id Target location ID
   * @return Distance between locations
   */
  [[nodiscard]] Distance getDistance(const std::string& from_id, const std::string& to_id) const {
    return location_tree_.getDistance(from_id, to_id);
  }

  /**
   * @brief Get travel time between two locations
   * @param from_id Source location ID
   * @param to_id Target location ID
   * @return Travel time between locations
   */
  [[nodiscard]] Duration getTravelTime(const std::string& from_id, const std::string& to_id) const {
    return location_tree_.getTravelTime(from_id, to_id);
  }

  /**
   * @brief Check if the problem is loaded
   * @return True if problem data is loaded, false otherwise
   */
  [[nodiscard]] bool isLoaded() const noexcept {
    return !depot_id_.empty() && !landfill_id_.empty() && !zone_ids_.empty();
  }

  /**
   * @brief Convert the problem to a string representation
   * @return String representation of the problem in the same format as the input file
   */
  [[nodiscard]] std::string toString() const {
    std::ostringstream oss;

    // Problem parameters
    oss << "L1 " << cv_max_duration_.value(units::TimeUnit::Minutes) << '\n';
    oss << "L2 " << tv_max_duration_.value(units::TimeUnit::Minutes) << '\n';
    oss << "num_vehicles " << num_cv_vehicles_ << '\n';
    oss << "num_zones " << num_zones_ << '\n';
    oss << "Lx " << map_width_ << '\n';
    oss << "Ly " << map_height_ << '\n';
    oss << "Q1 " << cv_capacity_.value() << '\n';
    oss << "Q2 " << tv_capacity_.value() << '\n';
    oss << "V " << vehicle_speed_.getValue(units::DistanceUnit::Kilometers, units::TimeUnit::Hours)
        << '\n';

    // Locations
    const auto& locations = location_tree_.getLocations();

    // Depot
    try {
      const auto& depot = getDepot();
      oss << "Depot " << (depot.x() / 1000.0) << " " << (depot.y() / 1000.0) << '\n';
    } catch (const std::exception&) {
      // Skip if not found
    }

    // SWTS locations
    for (const auto& id : swts_ids_) {
      auto it = locations.find(id);
      if (it != locations.end()) {
        const auto& swts = it->second;
        std::string if_name = swts.name().substr(5);
        oss << if_name << " " << (swts.x() / 1000.0) << " " << (swts.y() / 1000.0) << '\n';
      }
    }

    // Landfill
    try {
      const auto& landfill = getLandfill();
      oss << "Dumpsite " << (landfill.x() / 1000.0) << " " << (landfill.y() / 1000.0) << '\n';
    } catch (const std::exception&) {
      // Skip if not found
    }

    // Parameters
    oss << "epsilon " << epsilon_ << '\n';
    oss << "offset " << offset_ << '\n';
    oss << "k " << k_param_ << '\n';

    // Collection zones
    for (const auto& id : zone_ids_) {
      auto it = locations.find(id);
      if (it != locations.end()) {
        const auto& zone = it->second;
        // Extract zone number from ID, assuming format like "zone_1"
        std::string zone_number = id.substr(5);
        oss << zone_number << " " << zone.x() << " " << zone.y() << " "
            << zone.wasteAmount().value() << " "
            << zone.serviceTime().value(units::TimeUnit::Seconds) << '\n';
      }
    }

    return oss.str();
  }
};

}  // namespace daa
