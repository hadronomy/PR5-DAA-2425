#pragma once

#include <concepts>
#include <string>
#include <type_traits>

namespace daa {

// Concepts
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

template <typename T>
concept Point2D = requires(T p) {
  { p.x() } -> Numeric;
  { p.y() } -> Numeric;
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
  { l.id() } -> std::convertible_to<const std::string&>;
  { l.x() } -> std::convertible_to<double>;
  { l.y() } -> std::convertible_to<double>;
  { l.type() } -> std::convertible_to<LocationType>;
};

// KDTree point concept
template <typename T>
concept KDTreePoint = requires(T p) {
  { p.dimensions() } -> std::convertible_to<size_t>;
  { p.coordinate(size_t{}) } -> std::convertible_to<double>;
};

}  // namespace daa
