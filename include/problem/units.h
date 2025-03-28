#pragma once

namespace daa {
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
}  // namespace daa
