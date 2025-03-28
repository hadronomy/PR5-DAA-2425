#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <unordered_map>
#include <vector>

#include "concepts.h"
#include "location.h"
#include "strong_types.h"

namespace daa {

// Adapter to make Location compatible with KDTreePoint concept
class LocationAdapter {
 private:
  const Location* location_;

 public:
  explicit LocationAdapter(const Location& location) noexcept : location_(&location) {}

  // Add default constructor for containers
  LocationAdapter() noexcept : location_(nullptr) {}

  // Add copy constructor and assignment operator
  LocationAdapter(const LocationAdapter& other) noexcept : location_(other.location_) {}

  LocationAdapter& operator=(const LocationAdapter& other) noexcept {
    location_ = other.location_;
    return *this;
  }

  [[nodiscard]] size_t dimensions() const noexcept { return 2; }

  [[nodiscard]] double coordinate(size_t dim) const {
    if (!location_)
      throw std::runtime_error("Null location pointer");

    if (dim == 0)
      return location_->x();  // Changed to use pointer
    if (dim == 1)
      return location_->y();  // Changed to use pointer
    throw std::out_of_range("Invalid dimension");
  }

  [[nodiscard]] const Location& getLocation() const {
    if (!location_)
      throw std::runtime_error("Null location pointer");
    return *location_;
  }
};

// Generic distance calculator for KDTreePoints
template <KDTreePoint P>
class EuclideanDistanceCalculator {
 public:
  [[nodiscard]] double calculate(const P& a, const P& b) const {
    double sum = 0.0;
    const size_t dims = std::min(a.dimensions(), b.dimensions());

    for (size_t i = 0; i < dims; ++i) {
      const double diff = a.coordinate(i) - b.coordinate(i);
      sum += diff * diff;
    }

    return std::sqrt(sum);
  }
};

// Generic KDTree implementation
template <
  typename PointType,
  typename DistanceCalculator = EuclideanDistanceCalculator<PointType>,
  typename IdType = std::string>
requires KDTreePoint<PointType> class GenericKDTree {
 public:
  // Point container to store point with its identifier
  struct PointContainer {
    PointType point;
    IdType id;

    // Explicitly add default constructor
    PointContainer() = default;

    // Explicitly add copy constructor and assignment operator
    PointContainer(const PointContainer&) = default;
    PointContainer& operator=(const PointContainer&) = default;

    // Add constructor for initialization
    PointContainer(PointType p, IdType i) : point(std::move(p)), id(std::move(i)) {}
  };

 private:
  struct Node {
    PointContainer data;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    explicit Node(PointContainer pc) : data(std::move(pc)) {}
  };

  std::unique_ptr<Node> root_;
  size_t dimensions_{0};
  std::unordered_map<IdType, std::unordered_map<IdType, double>> distance_cache_;
  DistanceCalculator distance_calculator_{};

  // Helper function to build the tree recursively
  [[nodiscard]] static std::unique_ptr<Node>
    buildTreeRecursive(std::span<PointContainer> points, size_t depth) {
    if (points.empty()) {
      return nullptr;
    }

    const size_t axis = depth % (points.front().point.dimensions());
    const size_t mid = points.size() / 2;

    std::ranges::sort(points, {}, [axis](const auto& point_container) {
      return point_container.point.coordinate(axis);
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

  // Recursive nearest neighbor search
  void findNearestRecursive(
    const Node* node,
    const PointType& target,
    std::function<bool(const PointType&)> filter,
    size_t depth,
    std::optional<PointContainer>& best,
    double& best_dist
  ) const {
    if (!node) {
      return;
    }

    const double dist = distance_calculator_.calculate(node->data.point, target);

    if ((!best || dist < best_dist) && (!filter || filter(node->data.point))) {
      best = node->data;
      best_dist = dist;
    }

    const size_t dims = target.dimensions();
    const size_t axis = depth % dims;
    const double axis_dist = std::abs(node->data.point.coordinate(axis) - target.coordinate(axis));

    // Determine which subtree to search first
    const bool go_left = axis < dims && target.coordinate(axis) < node->data.point.coordinate(axis);

    const auto [first, second] = go_left ? std::pair{node->left.get(), node->right.get()}
                                         : std::pair{node->right.get(), node->left.get()};

    // Search first subtree
    findNearestRecursive(first, target, filter, depth + 1, best, best_dist);

    // Only search second subtree if it could contain a closer point
    if (axis_dist < best_dist) {
      findNearestRecursive(second, target, filter, depth + 1, best, best_dist);
    }
  }

  // Helper function for k-nearest neighbors search
  void findKNearestRecursive(
    const Node* node,
    const PointType& target,
    std::function<bool(const PointType&)> filter,
    size_t k,
    std::vector<std::pair<PointContainer, double>>& result,
    size_t depth
  ) const {
    if (!node)
      return;

    const double dist = distance_calculator_.calculate(node->data.point, target);

    // If the point passes the filter (or no filter), consider it
    if (!filter || filter(node->data.point)) {
      if (result.size() < k) {
        result.emplace_back(node->data, dist);
        std::push_heap(result.begin(), result.end(), [](const auto& a, const auto& b) {
          return a.second < b.second;
        });
      } else if (dist < result.front().second) {
        // Modified approach that avoids direct assignment
        std::pop_heap(result.begin(), result.end(), [](const auto& a, const auto& b) {
          return a.second < b.second;
        });
        result.pop_back();                      // Remove the element that was moved to the back
        result.emplace_back(node->data, dist);  // Add new element
        std::push_heap(result.begin(), result.end(), [](const auto& a, const auto& b) {
          return a.second < b.second;
        });
      }
    }

    const size_t dims = target.dimensions();
    const size_t axis = depth % dims;
    const double axis_dist = std::abs(node->data.point.coordinate(axis) - target.coordinate(axis));

    // Determine which subtree to search first
    const bool go_left = target.coordinate(axis) < node->data.point.coordinate(axis);
    const auto [first, second] = go_left ? std::pair{node->left.get(), node->right.get()}
                                         : std::pair{node->right.get(), node->left.get()};

    // Search first subtree
    findKNearestRecursive(first, target, filter, k, result, depth + 1);

    // Check if we need to search the other subtree
    if (result.size() < k || axis_dist < result.front().second) {
      findKNearestRecursive(second, target, filter, k, result, depth + 1);
    }
  }

 public:
  GenericKDTree() = default;

  // Build tree from a vector of points with IDs
  void build(std::vector<PointContainer> point_containers) {
    if (point_containers.empty()) {
      throw std::invalid_argument("Cannot build tree with empty point set");
    }

    dimensions_ = point_containers.front().point.dimensions();

    // Pre-compute all pairwise distances for efficient lookup
    for (const auto& pc1 : point_containers) {
      for (const auto& pc2 : point_containers) {
        if (pc1.id == pc2.id) {
          distance_cache_[pc1.id][pc2.id] = 0.0;
        } else {
          const double dist = distance_calculator_.calculate(pc1.point, pc2.point);
          distance_cache_[pc1.id][pc2.id] = dist;
        }
      }
    }

    root_ = buildTreeRecursive(std::span{point_containers}, 0);
  }

  // Find nearest neighbor with optional filter
  [[nodiscard]] std::optional<PointContainer> findNearest(
    const PointType& target,
    std::function<bool(const PointType&)> filter = nullptr
  ) const {
    if (!root_) {
      return std::nullopt;
    }

    std::optional<PointContainer> best;
    double best_dist = std::numeric_limits<double>::max();

    findNearestRecursive(root_.get(), target, filter, 0, best, best_dist);

    return best;
  }

  // Find k nearest neighbors with optional filter
  [[nodiscard]] std::vector<PointContainer> findKNearest(
    const PointType& target,
    size_t k,
    std::function<bool(const PointType&)> filter = nullptr
  ) const {
    if (!root_ || k == 0) {
      return {};
    }

    std::vector<std::pair<PointContainer, double>> nearest;
    nearest.reserve(k);

    findKNearestRecursive(root_.get(), target, filter, k, nearest, 0);

    std::vector<PointContainer> result;
    result.reserve(nearest.size());

    std::ranges::transform(nearest, std::back_inserter(result), [](const auto& pair) {
      return pair.first;
    });

    return result;
  }

  // Get cached distance between two points
  [[nodiscard]] double getDistance(const IdType& id1, const IdType& id2) const {
    try {
      return distance_cache_.at(id1).at(id2);
    } catch (const std::out_of_range&) {
      throw std::out_of_range("Distance not found in cache");
    }
  }

  // Clear the tree and cache
  void clear() {
    root_.reset();
    distance_cache_.clear();
    dimensions_ = 0;
  }
};

// Specialized KDTree for Locations
class KDTree {
 private:
  GenericKDTree<LocationAdapter, EuclideanDistanceCalculator<LocationAdapter>> tree_;
  std::unordered_map<std::string, Location> locations_;
  std::unordered_map<std::string, std::unordered_map<std::string, Duration>> time_matrix_;

  // Calculate travel time based on distance
  [[nodiscard]] static Duration calculateTime(double distance_meters) noexcept {
    constexpr double average_speed = 50.0;  // km/h
    return Duration{
      (distance_meters * units::meters_to_kilometers / average_speed) * 60.0,
      units::TimeUnit::Minutes
    };
  }

 public:
  // Build KDTree from locations
  void build(std::vector<Location> locations) {
    if (locations.empty()) {
      throw std::invalid_argument("Cannot build tree with empty locations");
    }

    locations_.clear();
    time_matrix_.clear();

    std::vector<typename GenericKDTree<LocationAdapter>::PointContainer> points;
    points.reserve(locations.size());

    for (const auto& loc : locations) {
      locations_[loc.id()] = loc;
      points.push_back({LocationAdapter(loc), loc.id()});
    }

    tree_.build(std::move(points));

    // Pre-calculate time matrix
    for (const auto& [id1, loc1] : locations_) {
      for (const auto& [id2, loc2] : locations_) {
        if (id1 != id2) {
          const double dist = tree_.getDistance(id1, id2);
          time_matrix_[id1][id2] = calculateTime(dist);
        } else {
          time_matrix_[id1][id2] = Duration{0.0};
        }
      }
    }
  }

  // Find nearest location of a specific type
  [[nodiscard]] std::optional<Location> findNearest(const Location& from, LocationType target_type)
    const {
    const LocationAdapter adapter(from);

    auto result = tree_.findNearest(adapter, [target_type](const LocationAdapter& adapter) {
      return adapter.getLocation().type() == target_type;
    });

    if (result) {
      return locations_.at(result->id);
    }

    return std::nullopt;
  }

  // Find k nearest locations of a specific type
  [[nodiscard]] std::vector<Location>
    findKNearest(const Location& from, LocationType target_type, size_t k) const {
    if (k == 0)
      return {};

    const LocationAdapter adapter(from);

    auto results = tree_.findKNearest(adapter, k, [target_type](const LocationAdapter& adapter) {
      return adapter.getLocation().type() == target_type;
    });

    std::vector<Location> locations;
    locations.reserve(results.size());

    for (const auto& result : results) {
      locations.push_back(locations_.at(result.id));
    }

    return locations;
  }

  // Get distance between two locations
  [[nodiscard]] Distance getDistance(const std::string& from_id, const std::string& to_id) const {
    const double dist = tree_.getDistance(from_id, to_id);
    return Distance{dist};
  }

  // Get travel time between two locations
  [[nodiscard]] Duration getTravelTime(const std::string& from_id, const std::string& to_id) const {
    return time_matrix_.at(from_id).at(to_id);
  }

  // Check if the tree contains locations
  [[nodiscard]] bool empty() const noexcept { return locations_.empty(); }

  // Get all locations
  [[nodiscard]] const std::unordered_map<std::string, Location>& getLocations() const noexcept {
    return locations_;
  }
};

}  // namespace daa
