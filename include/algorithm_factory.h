#pragma once

#include <memory>
#include <string_view>
#include <type_traits>
#include "algorithm_registry.h"

namespace daa {

/**
 * @brief Helper class for creating and managing algorithm instances
 */
class AlgorithmFactory {
 public:
  // Create any algorithm by name
  static std::unique_ptr<Algorithm> create(std::string_view name) {
    return AlgorithmRegistry::create(name);
  }

  // Create a typed algorithm with specific input/output types
  template <typename InputType, typename OutputType>
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>> createTyped(std::string_view name) {
    return AlgorithmRegistry::createTyped<InputType, OutputType>(name);
  }

  // Check if an algorithm exists
  static bool exists(std::string_view name) { return AlgorithmRegistry::exists(name); }

  // Get all available algorithm names
  static std::vector<std::string> availableAlgorithms() {
    return AlgorithmRegistry::availableAlgorithms();
  }

  // Create an algorithm and execute it directly
  template <typename InputType, typename OutputType>
  static OutputType execute(
    std::string_view name,
    const InputType& input,
    int timeoutMs = Algorithm::DEFAULT_TIME_LIMIT_MS
  ) {
    auto algorithm = createTyped<InputType, OutputType>(name);
    return algorithm->solveWithTimeLimit(input, timeoutMs);
  }
};

/**
 * @brief Base template for CRTP-based algorithm implementation
 *
 * @tparam Derived The concrete algorithm class
 * @tparam InputType Input data type
 * @tparam OutputType Output data type
 */
template <typename Derived, typename InputType, typename OutputType>
class AlgorithmImpl : public TypedAlgorithm<InputType, OutputType> {
 public:
  // Automatically register the algorithm on static initialization
  static bool registerThis(std::string_view name) {
    return AlgorithmRegistry::registerAlgorithm<Derived>(name);
  }

  // Default implementation for name() using class name
  std::string name() const override { return typeid(Derived).name(); }
};

}  // namespace daa
