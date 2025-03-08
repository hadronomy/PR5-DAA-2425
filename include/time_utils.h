#pragma once

#include <algorithm>
#include <cctype>
#include <regex>
#include <stdexcept>
#include <string>

namespace time_utils {

/**
 * @brief Parse a time string like "1m30s" or "45s" into milliseconds
 *
 * Supported formats:
 * - "1h" (1 hour)
 * - "5m" (5 minutes)
 * - "30s" (30 seconds)
 * - "1h30m" (1 hour and 30 minutes)
 * - "5m30s" (5 minutes and 30 seconds)
 * - "1h30m45s" (1 hour, 30 minutes, and 45 seconds)
 *
 * @param time_str The time string to parse
 * @return int Time in milliseconds
 * @throws std::invalid_argument If the format is invalid
 */
inline int parseTimeToMs(const std::string& time_str) {
  if (time_str.empty()) {
    throw std::invalid_argument("Empty time string");
  }

  // Allow direct integer input (assume milliseconds)
  if (std::all_of(time_str.begin(), time_str.end(), [](char c) { return std::isdigit(c); })) {
    return std::stoi(time_str);
  }

  std::regex time_pattern("(?:([0-9]+)h)?\\s*(?:([0-9]+)m)?\\s*(?:([0-9]+)s)?");
  std::smatch matches;

  if (!std::regex_match(time_str, matches, time_pattern) || matches.size() <= 1) {
    throw std::invalid_argument(
      "Invalid time format. Use formats like '1h', '5m', '30s', '1h30m', '5m30s'"
    );
  }

  // If no units were captured, indicate error
  if (matches[1].length() == 0 && matches[2].length() == 0 && matches[3].length() == 0) {
    throw std::invalid_argument("Invalid time format. At least one unit (h, m, s) must be specified"
    );
  }

  int hours = matches[1].length() > 0 ? std::stoi(matches[1]) : 0;
  int minutes = matches[2].length() > 0 ? std::stoi(matches[2]) : 0;
  int seconds = matches[3].length() > 0 ? std::stoi(matches[3]) : 0;

  if (hours == 0 && minutes == 0 && seconds == 0) {
    throw std::invalid_argument("Time value must be greater than zero");
  }

  // Convert to milliseconds
  return ((hours * 60 + minutes) * 60 + seconds) * 1000;
}

/**
 * @brief Format milliseconds into a human-readable string like "1h 30m 45s"
 *
 * @param ms Time in milliseconds
 * @return std::string Formatted time string
 */
inline std::string formatMsToTimeString(int ms) {
  int seconds = ms / 1000;
  int minutes = seconds / 60;
  int hours = minutes / 60;

  seconds %= 60;
  minutes %= 60;

  std::string result;

  if (hours > 0) {
    result += std::to_string(hours) + "h";
  }

  if (minutes > 0) {
    if (!result.empty())
      result += " ";
    result += std::to_string(minutes) + "m";
  }

  if (seconds > 0 || (hours == 0 && minutes == 0)) {
    if (!result.empty())
      result += " ";
    result += std::to_string(seconds) + "s";
  }

  return result;
}

}  // namespace time_utils