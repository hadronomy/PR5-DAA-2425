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

// Speed/Velocity
constexpr double mps_to_kmph = 3.6;        // meters per second to kilometers per hour
constexpr double kmph_to_mps = 1.0 / 3.6;  // kilometers per hour to meters per second
constexpr double mps_to_mph = 2.23694;     // meters per second to miles per hour
constexpr double mph_to_mps = 0.44704;     // miles per hour to meters per second

enum class DistanceUnit { Meters, Kilometers, Miles };

enum class TimeUnit { Nanoseconds, Seconds, Minutes, Hours };

enum class SpeedUnit { MetersPerSecond, KilometersPerHour, MilesPerHour };
}  // namespace units
}  // namespace daa
