#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace units {
// Distance
constexpr double meters_to_kilometers = 0.001;
constexpr double kilometers_to_meters = 1000.0;
constexpr double meters_to_miles = 0.000621371;
constexpr double miles_to_meters = 1609.34;

// Time
constexpr double ns_to_seconds = 1e-9;
constexpr double seconds_to_ns = 1e9;
constexpr double ns_to_minutes = 1.66667e-11;
constexpr double minutes_to_ns = 6e10;
constexpr double ns_to_hours = 2.77778e-13;
constexpr double hours_to_ns = 3.6e12;

enum class DistanceUnit { Meters, Kilometers, Miles };

enum class TimeUnit { Nanoseconds, Seconds, Minutes, Hours };
}  // namespace units

// Concepts
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

template <typename T>
concept Point2D = requires(T p) {
  { p.x } -> Numeric;
  { p.y } -> Numeric;
};

template <typename T>
concept TimeConvertible = requires(T t) {
  { t.nanoseconds() } -> std::convertible_to<int64_t>;
  { t.seconds() } -> std::convertible_to<double>;
  { t.minutes() } -> std::convertible_to<double>;
  { t.hours() } -> std::convertible_to<double>;
};

template <typename T>
concept DistanceConvertible = requires(T d) {
  { d.meters() } -> std::convertible_to<double>;
  { d.kilometers() } -> std::convertible_to<double>;
  { d.miles() } -> std::convertible_to<double>;
};

enum class LocationType { DEPOT, COLLECTION_ZONE, SWTS, LANDFILL };

template <typename T>
concept LocationLike = requires(T l) {
  { l.id() } -> std::convertible_to<int>;
  { l.x() } -> Numeric;
  { l.y() } -> Numeric;
  { l.type() } -> std::convertible_to<LocationType>;
};

// Strong types with validation
class Capacity {
 private:
  double value_;

 public:
  explicit constexpr Capacity(double v) : value_(v) {
    if (v < 0.0)
      throw std::invalid_argument("Capacity cannot be negative");
  }
  [[nodiscard]] constexpr double value() const noexcept { return value_; }
};

class Duration {
 private:
  int64_t nanoseconds_;  // Store internally in nanoseconds

 public:
  explicit constexpr Duration(double value, units::TimeUnit unit = units::TimeUnit::Nanoseconds)
      : nanoseconds_(convertToNanoseconds(value, unit)) {
    if (nanoseconds_ < 0)
      throw std::invalid_argument("Duration cannot be negative");
  }

  [[nodiscard]] static constexpr int64_t convertToNanoseconds(double value, units::TimeUnit from) {
    switch (from) {
      case units::TimeUnit::Nanoseconds:
        return static_cast<int64_t>(value);
      case units::TimeUnit::Seconds:
        return static_cast<int64_t>(value * units::seconds_to_ns);
      case units::TimeUnit::Minutes:
        return static_cast<int64_t>(value * units::minutes_to_ns);
      case units::TimeUnit::Hours:
        return static_cast<int64_t>(value * units::hours_to_ns);
      default:
        throw std::invalid_argument("Unknown time unit");
    }
  }

  [[nodiscard]] constexpr double getValue(units::TimeUnit unit) const {
    switch (unit) {
      case units::TimeUnit::Nanoseconds:
        return static_cast<double>(nanoseconds_);
      case units::TimeUnit::Seconds:
        return static_cast<double>(nanoseconds_) * units::ns_to_seconds;
      case units::TimeUnit::Minutes:
        return static_cast<double>(nanoseconds_) * units::ns_to_minutes;
      case units::TimeUnit::Hours:
        return static_cast<double>(nanoseconds_) * units::ns_to_hours;
      default:
        throw std::invalid_argument("Unknown time unit");
    }
  }

  [[nodiscard]] constexpr int64_t nanoseconds() const noexcept { return nanoseconds_; }
  [[nodiscard]] constexpr double seconds() const noexcept {
    return static_cast<double>(nanoseconds_) * units::ns_to_seconds;
  }
  [[nodiscard]] constexpr double minutes() const noexcept {
    return static_cast<double>(nanoseconds_) * units::ns_to_minutes;
  }
  [[nodiscard]] constexpr double hours() const noexcept {
    return static_cast<double>(nanoseconds_) * units::ns_to_hours;
  }

  Duration& operator+=(const Duration& other) noexcept {
    nanoseconds_ += other.nanoseconds_;
    return *this;
  }

  template <TimeConvertible T>
  Duration& operator+=(const T& other) noexcept {
    nanoseconds_ += other.nanoseconds();
    return *this;
  }

  friend Duration operator+(Duration lhs, const Duration& rhs) noexcept {
    lhs += rhs;
    return lhs;
  }

  template <TimeConvertible T>
  friend Duration operator+(Duration lhs, const T& rhs) noexcept {
    lhs += rhs;
    return lhs;
  }
};

class Distance {
 private:
  double meters_;  // Store internally in meters

 public:
  explicit constexpr Distance(double value, units::DistanceUnit unit = units::DistanceUnit::Meters)
      : meters_(convertToMeters(value, unit)) {
    if (meters_ < 0.0)
      throw std::invalid_argument("Distance cannot be negative");
  }

  [[nodiscard]] static constexpr double convertToMeters(double value, units::DistanceUnit from) {
    switch (from) {
      case units::DistanceUnit::Meters:
        return value;
      case units::DistanceUnit::Kilometers:
        return value * units::kilometers_to_meters;
      case units::DistanceUnit::Miles:
        return value * units::miles_to_meters;
      default:
        throw std::invalid_argument("Unknown distance unit");
    }
  }

  [[nodiscard]] constexpr double getValue(units::DistanceUnit unit) const {
    switch (unit) {
      case units::DistanceUnit::Meters:
        return meters_;
      case units::DistanceUnit::Kilometers:
        return meters_ * units::meters_to_kilometers;
      case units::DistanceUnit::Miles:
        return meters_ * units::meters_to_miles;
      default:
        throw std::invalid_argument("Unknown distance unit");
    }
  }

  [[nodiscard]] constexpr double meters() const noexcept { return meters_; }
  [[nodiscard]] constexpr double kilometers() const noexcept {
    return meters_ * units::meters_to_kilometers;
  }
  [[nodiscard]] constexpr double miles() const noexcept { return meters_ * units::meters_to_miles; }
};

class Location {
 public:
  class Builder;

 private:
  std::string id_;  // Changed from int to std::string
  double x_;
  double y_;
  LocationType type_;
  std::string name_;
  Duration service_time_;
  Capacity waste_amount_;

 public:
  Location(
    std::string id,  // Changed parameter type
    double x,
    double y,
    LocationType type,
    std::string name,
    Duration service_time,
    Capacity waste_amount
  )
      : id_(std::move(id)),  // Move the string
        x_(x),
        y_(y),
        type_(type),
        name_(std::move(name)),
        service_time_(service_time),
        waste_amount_(waste_amount) {}

  [[nodiscard]] const std::string& id() const noexcept { return id_; }  // Changed return type
  [[nodiscard]] double x() const noexcept { return x_; }
  [[nodiscard]] double y() const noexcept { return y_; }
  [[nodiscard]] LocationType type() const noexcept { return type_; }
  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  [[nodiscard]] const Duration& serviceTime() const noexcept { return service_time_; }
  [[nodiscard]] const Capacity& wasteAmount() const noexcept { return waste_amount_; }
};

class Location::Builder {
 private:
  std::string id_;
  double x_ = 0.0;
  double y_ = 0.0;
  LocationType type_ = LocationType::DEPOT;
  std::string name_;
  Duration service_time_{0.0};
  Capacity waste_amount_{0.0};

 public:
  Builder& setId(std::string id) {  // Changed parameter type
    id_ = std::move(id);
    return *this;
  }

  Builder& setCoordinates(double x, double y) {
    x_ = x;
    y_ = y;
    return *this;
  }

  Builder& setType(LocationType type) {
    type_ = type;
    return *this;
  }

  Builder& setName(std::string name) {
    name_ = std::move(name);
    return *this;
  }

  Builder& setServiceTime(Duration time) {
    service_time_ = time;
    return *this;
  }

  Builder& setWasteAmount(Capacity amount) {
    waste_amount_ = amount;
    return *this;
  }

  [[nodiscard]] Location build() const {
    return Location(id_, x_, y_, type_, name_, service_time_, waste_amount_);
  }
};

class KDTree {
 private:
  struct Node {
    Location location;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    explicit Node(Location loc) noexcept : location(std::move(loc)) {}
  };

  std::unique_ptr<Node> root_;
  std::unordered_map<std::string, std::unordered_map<std::string, Distance>> distance_matrix_;
  std::unordered_map<std::string, std::unordered_map<std::string, Duration>> time_matrix_;

  [[nodiscard]] static std::unique_ptr<Node>
    buildTreeRecursive(std::span<Location> points, size_t depth) {
    if (points.empty()) {
      return nullptr;
    }

    const size_t axis = depth % 2;
    const size_t mid = points.size() / 2;

    std::ranges::sort(points, {}, [axis](const auto& point) {
      return axis == 0 ? point.x() : point.y();
    });

    auto node = std::make_unique<Node>(points[mid]);

    if (mid > 0) {
      node->left = buildTreeRecursive(points.subspan(0, mid), depth + 1);
    }
    if (mid + 1 < points.size()) {
      node->right = buildTreeRecursive(points.subspan(mid + 1), depth + 1);
    }

    return node;
  }

  void findNearestRecursive(
    const Node* node,
    const Location& target,
    LocationType target_type,
    size_t depth,
    std::optional<Location>& best,
    Distance& best_dist
  ) const {
    if (!node) {
      return;
    }

    const Distance dist = calculateDistance(node->location, target);

    if ((!best || dist.kilometers() < best_dist.kilometers()) &&
        (target_type == node->location.type())) {
      best = node->location;
      best_dist = dist;
    }

    const size_t axis = depth % 2;
    const double axis_dist = axis == 0 ? std::abs(node->location.x() - target.x())
                                       : std::abs(node->location.y() - target.y());

    const auto [first, second] = (axis == 0 && target.x() < node->location.x()) ||
                                     (axis == 1 && target.y() < node->location.y())
                                 ? std::pair{node->left.get(), node->right.get()}
                                 : std::pair{node->right.get(), node->left.get()};

    findNearestRecursive(first, target, target_type, depth + 1, best, best_dist);

    if (axis_dist < best_dist.kilometers()) {
      findNearestRecursive(second, target, target_type, depth + 1, best, best_dist);
    }
  }

  [[nodiscard]] static Distance calculateDistance(const Location& a, const Location& b) noexcept {
    const double dx = a.x() - b.x();
    const double dy = a.y() - b.y();
    return Distance{std::sqrt(dx * dx + dy * dy)};
  }

  [[nodiscard]] static Duration calculateTime(const Distance& distance) noexcept {
    constexpr double average_speed = 50.0;  // km/h
    return Duration{(distance.kilometers() / average_speed) * 60.0, units::TimeUnit::Minutes};
  }

 public:
  void build(std::vector<Location> locations) {
    const size_t n = locations.size();
    if (n == 0) {
      throw std::invalid_argument("Cannot build tree with empty locations");
    }

    for (const auto& loc1 : locations) {
      for (const auto& loc2 : locations) {
        const auto dist = calculateDistance(loc1, loc2);
        distance_matrix_[loc1.id()][loc2.id()] = dist;

        const auto time = calculateTime(dist);
        time_matrix_[loc1.id()][loc2.id()] = time;
      }
    }

    root_ = buildTreeRecursive(std::span{locations}, 0);
  }

  [[nodiscard]] std::optional<Location> findNearest(const Location& from, LocationType target_type)
    const {
    std::optional<Location> best;
    Distance best_dist{std::numeric_limits<double>::max()};
    findNearestRecursive(root_.get(), from, target_type, 0, best, best_dist);
    return best;
  }

  [[nodiscard]] std::vector<Location>
    findKNearest(const Location& from, LocationType target_type, size_t k) const {
    if (k == 0) {
      return {};
    }

    struct LocationWithDistance {
      Location location;
      Distance distance;
    };

    auto comp = [](const auto& a, const auto& b) {
      return a.distance.kilometers() < b.distance.kilometers();
    };

    std::vector<LocationWithDistance> nearest;
    nearest.reserve(k);

    std::function<void(const Node*)> search = [&](const Node* node) {
      if (!node) {
        return;
      }

      if (node->location.type() == target_type) {
        const auto dist = calculateDistance(from, node->location);

        if (nearest.size() < k) {
          nearest.push_back({node->location, dist});
          std::push_heap(nearest.begin(), nearest.end(), comp);
        } else if (dist.kilometers() < nearest.front().distance.kilometers()) {
          std::pop_heap(nearest.begin(), nearest.end(), comp);
          nearest.back() = {node->location, dist};
          std::push_heap(nearest.begin(), nearest.end(), comp);
        }
      }

      search(node->left.get());
      search(node->right.get());
    };

    search(root_.get());

    std::vector<Location> result;
    result.reserve(nearest.size());
    std::ranges::transform(nearest, std::back_inserter(result), [](const auto& item) {
      return item.location;
    });

    return result;
  }

  [[nodiscard]] Distance getDistance(const std::string& from_id, const std::string& to_id) const {
    return distance_matrix_.at(from_id).at(to_id);
  }

  [[nodiscard]] Duration getTravelTime(const std::string& from_id, const std::string& to_id) const {
    return time_matrix_.at(from_id).at(to_id);
  }
};

class Route {
 private:
  std::vector<std::string> sequence_;  // Changed from vector<int>
  Capacity current_load_{0.0};
  Duration total_duration_{0.0};

 public:
  [[nodiscard]] bool canAdd(
    const Location& loc,
    const KDTree& kd_tree,
    const Capacity& max_capacity,
    const Duration& max_duration
  ) const {
    if (sequence_.empty()) {
      return true;
    }

    const auto& last_id = sequence_.back();
    const auto time_to_loc = kd_tree.getTravelTime(last_id, loc.id());
    const auto new_duration = total_duration_ + time_to_loc + loc.serviceTime();
    const auto new_load = Capacity{current_load_.value() + loc.wasteAmount().value()};

    return new_load.value() <= max_capacity.value() &&
           new_duration.nanoseconds() <= max_duration.nanoseconds();
  }

  void add(const Location& loc, const KDTree& kd_tree) {
    if (!sequence_.empty()) {
      const auto& last_id = sequence_.back();
      const auto time_to_loc = kd_tree.getTravelTime(last_id, loc.id());
      total_duration_ += time_to_loc + loc.serviceTime();
    }

    sequence_.push_back(loc.id());
    current_load_ = Capacity{current_load_.value() + loc.wasteAmount().value()};
  }

  void resetLoad() { current_load_ = Capacity{0.0}; }

  [[nodiscard]] const std::vector<std::string>& sequence() const noexcept { return sequence_; }
  [[nodiscard]] const Capacity& currentLoad() const noexcept { return current_load_; }
  [[nodiscard]] const Duration& totalDuration() const noexcept { return total_duration_; }
};

class VRPTSWTSRouter {
 private:
  KDTree kd_tree_;
  std::vector<Location> locations_;
  std::unordered_map<std::string, size_t> id_to_index_;  // Changed from int to string

  [[nodiscard]] Location
    findNearestInVector(const Location& from, const std::vector<Location>& candidates) const {
    return *std::ranges::min_element(candidates, [&](const auto& a, const auto& b) {
      return kd_tree_.getDistance(from.id(), a.id()).kilometers() <
             kd_tree_.getDistance(from.id(), b.id()).kilometers();
    });
  }

 public:
  void initialize(std::vector<Location> locations) {
    if (locations.empty()) {
      throw std::invalid_argument("Cannot initialize with empty locations");
    }

    locations_ = std::move(locations);
    id_to_index_.clear();

    for (size_t i = 0; i < locations_.size(); ++i) {
      id_to_index_[locations_[i].id()] = i;
    }

    kd_tree_.build(locations_);
  }

  [[nodiscard]] std::vector<Route> buildCollectionRoutes(
    const std::string& depot_id,  // Changed parameter type
    const Capacity& vehicle_capacity,
    const Duration& max_time
  ) {
    if (!id_to_index_.contains(depot_id)) {
      throw std::invalid_argument("Invalid depot ID");
    }

    std::vector<Route> routes;
    auto unvisited_zones = std::ranges::filter_view(locations_, [](const auto& loc) {
      return loc.type() == LocationType::COLLECTION_ZONE;
    });

    std::vector<Location> remaining_zones{unvisited_zones.begin(), unvisited_zones.end()};

    const auto& depot = locations_[id_to_index_[depot_id]];

    while (!remaining_zones.empty()) {
      Route route;
      route.add(depot, kd_tree_);

      Location current = depot;

      while (!remaining_zones.empty()) {
        auto nearest = findNearestInVector(current, remaining_zones);

        if (route.canAdd(nearest, kd_tree_, vehicle_capacity, max_time)) {
          route.add(nearest, kd_tree_);
          current = nearest;

          std::erase_if(remaining_zones, [&](const auto& loc) { return loc.id() == nearest.id(); });
        } else {
          if (auto nearest_swts = kd_tree_.findNearest(current, LocationType::SWTS)) {
            if (route.canAdd(*nearest_swts, kd_tree_, vehicle_capacity, max_time)) {
              route.add(*nearest_swts, kd_tree_);
              current = *nearest_swts;
              route.resetLoad();  // Use the new resetLoad method
            } else {
              break;
            }
          } else {
            break;
          }
        }
      }

      if (route.currentLoad().value() > 0) {
        if (auto nearest_swts = kd_tree_.findNearest(current, LocationType::SWTS)) {
          if (route.canAdd(*nearest_swts, kd_tree_, vehicle_capacity, max_time)) {
            route.add(*nearest_swts, kd_tree_);
          }
        }
      }

      route.add(depot, kd_tree_);
      routes.push_back(std::move(route));
    }

    return routes;
  }
};