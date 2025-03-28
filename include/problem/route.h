#pragma once

#include <ranges>
#include <string>
#include <vector>

#include "kdtree.h"
#include "location.h"
#include "strong_types.h"

namespace daa {

class Route {
 private:
  std::vector<std::string> sequence_;
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

  [[nodiscard]] std::vector<const Location*> getLocations(const KDTree& kdtree) const {
    if (sequence_.empty())
      return {};

    const auto& locations = kdtree.getLocations();

    return sequence_ |
           std::ranges::views::transform([&locations](const auto& id) -> const Location* {
             auto it = locations.find(id);
             return it != locations.end() ? &it->second : nullptr;
           }) |
           std::ranges::views::filter([](const Location* loc) { return loc != nullptr; }) |
           std::ranges::to<std::vector>();
  }
};

}  // namespace daa
