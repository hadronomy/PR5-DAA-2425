#pragma once

#include <bitset>
#include <cassert>
#include <random>
#include <stdexcept>

namespace daa {
namespace algorithm {

/**
 * @brief Efficient bitmap representation for tracking available neighborhoods
 *
 * Uses std::bitset for efficient storage and operations on neighborhood availability.
 * Provides methods for setting/unsetting neighborhoods, checking if any are available,
 * and selecting a random available neighborhood.
 */
template <size_t MaxNeighborhoods = 32>
class NeighborhoodBitmap {
 public:
  /**
   * @brief Construct a new Neighborhood Bitmap with all neighborhoods available
   *
   * @param count Number of neighborhoods to track (must be <= MaxNeighborhoods)
   */
  explicit NeighborhoodBitmap(size_t count) : count_(count) {
    assert(count <= MaxNeighborhoods && "Number of neighborhoods exceeds maximum");
    // Set all bits to 1 (available) up to count
    for (size_t i = 0; i < count; ++i) {
      bitmap_.set(i);
    }
  }

  /**
   * @brief Check if any neighborhoods are available
   *
   * @return true if at least one neighborhood is available
   */
  [[nodiscard]] bool hasAvailable() const { return bitmap_.any(); }

  /**
   * @brief Get the number of available neighborhoods
   *
   * @return size_t Number of available neighborhoods
   */
  [[nodiscard]] size_t availableCount() const { return bitmap_.count(); }

  /**
   * @brief Check if a specific neighborhood is available
   *
   * @param index Neighborhood index to check
   * @return true if the neighborhood is available
   */
  [[nodiscard]] bool isAvailable(size_t index) const {
    assert(index < count_ && "Neighborhood index out of bounds");
    return bitmap_[index];
  }

  /**
   * @brief Mark a neighborhood as unavailable
   *
   * @param index Neighborhood index to mark as unavailable
   */
  void markUnavailable(size_t index) {
    assert(index < count_ && "Neighborhood index out of bounds");
    bitmap_.reset(index);
  }

  /**
   * @brief Mark all neighborhoods as available
   */
  void resetAll() {
    for (size_t i = 0; i < count_; ++i) {
      bitmap_.set(i);
    }
  }

  /**
   * @brief Select a random available neighborhood
   *
   * Uses an optimized approach that avoids creating a temporary vector
   *
   * @param gen Random number generator
   * @return size_t Index of the selected neighborhood
   * @throws std::runtime_error if no neighborhoods are available
   */
  template <typename RNG>
  size_t selectRandom(RNG& gen) {
    if (!hasAvailable()) {
      throw std::runtime_error("No neighborhoods available to select");
    }

    // Count available neighborhoods
    size_t available_count = bitmap_.count();

    // Generate a random index between 0 and available_count-1
    std::uniform_int_distribution<size_t> dist(0, available_count - 1);
    size_t random_idx = dist(gen);

    // Find the nth available neighborhood
    size_t current_idx = 0;
    for (size_t i = 0; i < count_; ++i) {
      if (bitmap_[i]) {
        if (current_idx == random_idx) {
          return i;
        }
        ++current_idx;
      }
    }

    // This should never happen if bitmap_.count() is correct
    throw std::runtime_error("Failed to select random neighborhood");
  }

 private:
  std::bitset<MaxNeighborhoods> bitmap_;
  size_t count_;
};

}  // namespace algorithm
}  // namespace daa
