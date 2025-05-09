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

  // Comparison operators
  [[nodiscard]] constexpr bool operator==(const Capacity& other) const noexcept {
    return value_ == other.value_;
  }
  [[nodiscard]] constexpr bool operator!=(const Capacity& other) const noexcept {
    return value_ != other.value_;
  }
  [[nodiscard]] constexpr bool operator<(const Capacity& other) const noexcept {
    return value_ < other.value_;
  }
  [[nodiscard]] constexpr bool operator<=(const Capacity& other) const noexcept {
    return value_ <= other.value_;
  }
  [[nodiscard]] constexpr bool operator>(const Capacity& other) const noexcept {
    return value_ > other.value_;
  }
  [[nodiscard]] constexpr bool operator>=(const Capacity& other) const noexcept {
    return value_ >= other.value_;
  }

  // Compound assignment operators
  constexpr Capacity& operator+=(const Capacity& other) noexcept {
    value_ += other.value_;
    return *this;
  }
  constexpr Capacity& operator-=(const Capacity& other) {
    if (value_ < other.value_)
      throw std::invalid_argument("Capacity cannot be negative");
    value_ -= other.value_;
    return *this;
  }
  constexpr Capacity& operator*=(double scalar) {
    if (scalar < 0.0)
      throw std::invalid_argument("Result cannot be negative");
    value_ *= scalar;
    return *this;
  }
  constexpr Capacity& operator/=(double scalar) {
    if (scalar <= 0.0)
      throw std::invalid_argument("Cannot divide by zero or negative value");
    value_ /= scalar;
    return *this;
  }

  // Arithmetic operators
  [[nodiscard]] friend constexpr Capacity operator+(Capacity lhs, const Capacity& rhs) noexcept {
    lhs += rhs;
    return lhs;
  }
  [[nodiscard]] friend constexpr Capacity operator-(Capacity lhs, const Capacity& rhs) {
    lhs -= rhs;
    return lhs;
  }
  [[nodiscard]] friend constexpr Capacity operator*(Capacity lhs, double scalar) {
    lhs *= scalar;
    return lhs;
  }
  [[nodiscard]] friend constexpr Capacity operator*(double scalar, const Capacity& rhs) {
    return rhs * scalar;
  }
  [[nodiscard]] friend constexpr Capacity operator/(Capacity lhs, double scalar) {
    lhs /= scalar;
    return lhs;
  }
};

// Forward declaration to resolve circular dependencies
class Speed;
class Distance;
class Duration;

class Duration {
 private:
  int64_t nanoseconds_;  // Store internally in nanoseconds

 public:
  // Default constructor creates duration of zero
  constexpr Duration() noexcept : nanoseconds_(0) {}

  explicit constexpr Duration(double value, units::TimeUnit unit = units::TimeUnit::Nanoseconds)
      : nanoseconds_(convertToNanoseconds(value, unit)) {}

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

  [[deprecated("Use value() instead.")]] [[nodiscard]]
  constexpr double getValue(units::TimeUnit unit) const {
    return value(unit);
  }

  [[nodiscard]] constexpr double value(units::TimeUnit unit = units::TimeUnit::Seconds) const {
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

  // Subtraction operators
  Duration& operator-=(const Duration& other) {
    nanoseconds_ -= other.nanoseconds_;
    return *this;
  }

  template <TimeConvertible T>
  Duration& operator-=(const T& other) {
    nanoseconds_ -= other.nanoseconds();
    return *this;
  }

  [[nodiscard]] friend Duration operator-(Duration lhs, const Duration& rhs) {
    lhs -= rhs;
    return lhs;
  }

  template <TimeConvertible T>
  [[nodiscard]] friend Duration operator-(Duration lhs, const T& rhs) {
    lhs -= rhs;
    return lhs;
  }

  // Scalar multiplication and division
  Duration& operator*=(double scalar) {
    nanoseconds_ = static_cast<int64_t>(static_cast<double>(nanoseconds_) * scalar);
    return *this;
  }

  Duration& operator/=(double scalar) {
    if (scalar == 0.0)
      throw std::invalid_argument("Cannot divide by zero");
    nanoseconds_ = static_cast<int64_t>(static_cast<double>(nanoseconds_) / scalar);
    return *this;
  }

  [[nodiscard]] friend Duration operator*(Duration lhs, double scalar) {
    lhs *= scalar;
    return lhs;
  }

  [[nodiscard]] friend Duration operator*(double scalar, const Duration& rhs) {
    return rhs * scalar;
  }

  [[nodiscard]] friend Duration operator/(Duration lhs, double scalar) {
    lhs /= scalar;
    return lhs;
  }

  // Comparison operators
  [[nodiscard]] constexpr bool operator==(const Duration& other) const noexcept {
    return nanoseconds_ == other.nanoseconds_;
  }
  [[nodiscard]] constexpr bool operator!=(const Duration& other) const noexcept {
    return nanoseconds_ != other.nanoseconds_;
  }
  [[nodiscard]] constexpr bool operator<(const Duration& other) const noexcept {
    return nanoseconds_ < other.nanoseconds_;
  }
  [[nodiscard]] constexpr bool operator<=(const Duration& other) const noexcept {
    return nanoseconds_ <= other.nanoseconds_;
  }
  [[nodiscard]] constexpr bool operator>(const Duration& other) const noexcept {
    return nanoseconds_ > other.nanoseconds_;
  }
  [[nodiscard]] constexpr bool operator>=(const Duration& other) const noexcept {
    return nanoseconds_ >= other.nanoseconds_;
  }

  // Calculate distance when multiplied by speed
  friend Distance operator*(const Duration& duration, const Speed& speed);
  friend Distance operator*(const Speed& speed, const Duration& duration);
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

  [[deprecated("Use value() instead.")]] [[nodiscard]]
  constexpr double getValue(units::DistanceUnit unit) const {
    return value(unit);
  }

  [[nodiscard]] constexpr double value(units::DistanceUnit unit = units::DistanceUnit::Meters)
    const {
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

  // Comparison operators
  [[nodiscard]] constexpr bool operator==(const Distance& other) const noexcept {
    return meters_ == other.meters_;
  }
  [[nodiscard]] constexpr bool operator!=(const Distance& other) const noexcept {
    return meters_ != other.meters_;
  }
  [[nodiscard]] constexpr bool operator<(const Distance& other) const noexcept {
    return meters_ < other.meters_;
  }
  [[nodiscard]] constexpr bool operator<=(const Distance& other) const noexcept {
    return meters_ <= other.meters_;
  }
  [[nodiscard]] constexpr bool operator>(const Distance& other) const noexcept {
    return meters_ > other.meters_;
  }
  [[nodiscard]] constexpr bool operator>=(const Distance& other) const noexcept {
    return meters_ >= other.meters_;
  }

  // Addition and subtraction operators
  Distance& operator+=(const Distance& other) noexcept {
    meters_ += other.meters_;
    return *this;
  }

  Distance& operator-=(const Distance& other) {
    if (meters_ < other.meters_)
      throw std::invalid_argument("Distance cannot be negative");
    meters_ -= other.meters_;
    return *this;
  }

  [[nodiscard]] friend Distance operator+(Distance lhs, const Distance& rhs) noexcept {
    lhs += rhs;
    return lhs;
  }

  [[nodiscard]] friend Distance operator-(Distance lhs, const Distance& rhs) {
    lhs -= rhs;
    return lhs;
  }

  // Scalar multiplication and division
  Distance& operator*=(double scalar) {
    if (scalar < 0.0)
      throw std::invalid_argument("Result cannot be negative");
    meters_ *= scalar;
    return *this;
  }

  Distance& operator/=(double scalar) {
    if (scalar <= 0.0)
      throw std::invalid_argument("Cannot divide by zero or negative value");
    meters_ /= scalar;
    return *this;
  }

  [[nodiscard]] friend Distance operator*(Distance lhs, double scalar) {
    lhs *= scalar;
    return lhs;
  }

  [[nodiscard]] friend Distance operator*(double scalar, const Distance& rhs) {
    return rhs * scalar;
  }

  [[nodiscard]] friend Distance operator/(Distance lhs, double scalar) {
    lhs /= scalar;
    return lhs;
  }

  // Calculate speed when divided by duration
  [[nodiscard]] Speed operator/(const Duration& duration) const;

  // Calculate duration when divided by speed
  [[nodiscard]] Duration operator/(const Speed& speed) const;
};

class Speed {
 private:
  double mps_;  // Still store internally in meters per second

 public:
  // Constructor that takes separate distance and time units
  explicit constexpr Speed(
    double value,
    units::DistanceUnit distUnit = units::DistanceUnit::Meters,
    units::TimeUnit timeUnit = units::TimeUnit::Seconds
  )
      : mps_(convertToMetersPerSecond(value, distUnit, timeUnit)) {
    if (mps_ < 0.0)
      throw std::invalid_argument("Speed cannot be negative");
  }

  // Legacy constructor using SpeedUnit (for backward compatibility)
  explicit constexpr Speed(double value, units::SpeedUnit unit)
      : mps_(convertToMetersPerSecond(value, unit)) {
    if (mps_ < 0.0)
      throw std::invalid_argument("Speed cannot be negative");
  }

  // Convert any distance/time unit combination to meters per second
  [[nodiscard]] static constexpr double
    convertToMetersPerSecond(double value, units::DistanceUnit distUnit, units::TimeUnit timeUnit) {
    // First convert to the source distance unit in meters
    double distanceInMeters = 0.0;
    switch (distUnit) {
      case units::DistanceUnit::Meters:
        distanceInMeters = value;
        break;
      case units::DistanceUnit::Kilometers:
        distanceInMeters = value * units::kilometers_to_meters;
        break;
      case units::DistanceUnit::Miles:
        distanceInMeters = value * units::miles_to_meters;
        break;
      default:
        throw std::invalid_argument("Unknown distance unit");
    }

    // Then divide by the time in seconds
    double timeInSeconds = 0.0;
    switch (timeUnit) {
      case units::TimeUnit::Seconds:
        timeInSeconds = 1.0;  // Already in seconds
        break;
      case units::TimeUnit::Nanoseconds:
        timeInSeconds = units::ns_to_seconds;
        break;
      case units::TimeUnit::Minutes:
        timeInSeconds = 60.0;
        break;
      case units::TimeUnit::Hours:
        timeInSeconds = 3600.0;
        break;
      default:
        throw std::invalid_argument("Unknown time unit");
    }

    return distanceInMeters / timeInSeconds;
  }

  // Legacy conversion method (for backward compatibility)
  [[nodiscard]] static constexpr double
    convertToMetersPerSecond(double value, units::SpeedUnit from) {
    switch (from) {
      case units::SpeedUnit::MetersPerSecond:
        return value;
      case units::SpeedUnit::KilometersPerHour:
        return value * units::kmph_to_mps;
      case units::SpeedUnit::MilesPerHour:
        return value * units::mph_to_mps;
      default:
        throw std::invalid_argument("Unknown speed unit");
    }
  }

  // Get value in any distance/time unit combination
  [[deprecated("Use value() instead.")]] [[nodiscard]]
  constexpr double getValue(units::DistanceUnit distUnit, units::TimeUnit timeUnit) const {
    return value(distUnit, timeUnit);
  }

  [[nodiscard]] constexpr double value(
    units::DistanceUnit distUnit = units::DistanceUnit::Meters,
    units::TimeUnit timeUnit = units::TimeUnit::Seconds
  ) const {
    // First get the distance conversion factor
    double distConversion = 1.0;
    switch (distUnit) {
      case units::DistanceUnit::Meters:
        distConversion = 1.0;
        break;
      case units::DistanceUnit::Kilometers:
        distConversion = units::meters_to_kilometers;
        break;
      case units::DistanceUnit::Miles:
        distConversion = units::meters_to_miles;
        break;
      default:
        throw std::invalid_argument("Unknown distance unit");
    }

    // Then get the time conversion factor
    double timeConversion = 1.0;
    switch (timeUnit) {
      case units::TimeUnit::Seconds:
        timeConversion = 1.0;
        break;
      case units::TimeUnit::Nanoseconds:
        timeConversion = units::seconds_to_ns;
        break;
      case units::TimeUnit::Minutes:
        timeConversion = 1.0 / 60.0;
        break;
      case units::TimeUnit::Hours:
        timeConversion = 1.0 / 3600.0;
        break;
      default:
        throw std::invalid_argument("Unknown time unit");
    }

    return mps_ * distConversion / timeConversion;
  }

  // Legacy getValue method (for backward compatibility)
  [[deprecated("Use value() with SpeedUnit parameter instead.")]] [[nodiscard]]
  constexpr double getValue(units::SpeedUnit unit) const {
    return value(unit);
  }

  [[nodiscard]] constexpr double value(units::SpeedUnit unit = units::SpeedUnit::MetersPerSecond)
    const {
    switch (unit) {
      case units::SpeedUnit::MetersPerSecond:
        return mps_;
      case units::SpeedUnit::KilometersPerHour:
        return mps_ * units::mps_to_kmph;
      case units::SpeedUnit::MilesPerHour:
        return mps_ * units::mps_to_mph;
      default:
        throw std::invalid_argument("Unknown speed unit");
    }
  }

  // Common unit accessors
  [[nodiscard]] constexpr double metersPerSecond() const noexcept { return mps_; }
  [[nodiscard]] constexpr double kilometersPerHour() const noexcept {
    return mps_ * units::mps_to_kmph;
  }
  [[nodiscard]] constexpr double milesPerHour() const noexcept { return mps_ * units::mps_to_mph; }

  // Comparison operators
  [[nodiscard]] constexpr bool operator==(const Speed& other) const noexcept {
    return mps_ == other.mps_;
  }
  [[nodiscard]] constexpr bool operator!=(const Speed& other) const noexcept {
    return mps_ != other.mps_;
  }
  [[nodiscard]] constexpr bool operator<(const Speed& other) const noexcept {
    return mps_ < other.mps_;
  }
  [[nodiscard]] constexpr bool operator<=(const Speed& other) const noexcept {
    return mps_ <= other.mps_;
  }
  [[nodiscard]] constexpr bool operator>(const Speed& other) const noexcept {
    return mps_ > other.mps_;
  }
  [[nodiscard]] constexpr bool operator>=(const Speed& other) const noexcept {
    return mps_ >= other.mps_;
  }

  // Addition and subtraction operators
  Speed& operator+=(const Speed& other) noexcept {
    mps_ += other.mps_;
    return *this;
  }

  Speed& operator-=(const Speed& other) {
    if (mps_ < other.mps_)
      throw std::invalid_argument("Speed cannot be negative");
    mps_ -= other.mps_;
    return *this;
  }

  [[nodiscard]] friend Speed operator+(Speed lhs, const Speed& rhs) noexcept {
    lhs += rhs;
    return lhs;
  }

  [[nodiscard]] friend Speed operator-(Speed lhs, const Speed& rhs) {
    lhs -= rhs;
    return lhs;
  }

  // Scalar multiplication and division
  Speed& operator*=(double scalar) {
    if (scalar < 0.0)
      throw std::invalid_argument("Result cannot be negative");
    mps_ *= scalar;
    return *this;
  }

  Speed& operator/=(double scalar) {
    if (scalar <= 0.0)
      throw std::invalid_argument("Cannot divide by zero or negative value");
    mps_ /= scalar;
    return *this;
  }

  [[nodiscard]] friend Speed operator*(Speed lhs, double scalar) {
    lhs *= scalar;
    return lhs;
  }

  [[nodiscard]] friend Speed operator*(double scalar, const Speed& rhs) { return rhs * scalar; }

  [[nodiscard]] friend Speed operator/(Speed lhs, double scalar) {
    lhs /= scalar;
    return lhs;
  }

  // Calculate distance when multiplied by duration
  [[nodiscard]] Distance operator*(const Duration& duration) const;
  friend Distance operator*(const Duration& duration, const Speed& speed);
};

// Implementation of cross-type operations

inline Distance Speed::operator*(const Duration& duration) const {
  // Speed (m/s) * Duration (ns) = Distance (m)
  // Convert nanoseconds to seconds first
  double seconds = duration.seconds();
  double meters = mps_ * seconds;
  return Distance(meters, units::DistanceUnit::Meters);
}

inline Distance operator*(const Duration& duration, const Speed& speed) {
  return speed.operator*(duration);
}

inline Speed Distance::operator/(const Duration& duration) const {
  // Distance (m) / Duration (ns) = Speed (m/s)
  double seconds = duration.seconds();
  if (seconds <= 0) {
    throw std::invalid_argument("Cannot divide by zero or negative duration");
  }
  double mps = meters_ / seconds;
  return Speed(mps, units::SpeedUnit::MetersPerSecond);
}

inline Duration Distance::operator/(const Speed& speed) const {
  // Distance (m) / Speed (m/s) = Duration (s)
  double mps = speed.metersPerSecond();
  if (mps <= 0) {
    throw std::invalid_argument("Cannot divide by zero or negative speed");
  }
  double seconds = meters_ / mps;
  return Duration(seconds, units::TimeUnit::Seconds);
}

}  // namespace daa