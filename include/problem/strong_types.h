#pragma once

#include <cstdint>
#include <stdexcept>
#include "concepts.h"
#include "units.h"

namespace daa {

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

}  // namespace daa