#pragma once
#include <chrono>
#include <concepts>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ui.h"

namespace daa {

// Forward declaration
template <typename T>
class DataGenerator;

// Concepts for algorithms and their capabilities
template <typename T>
concept AlgorithmBase = requires(T a) {
  { a.name() } -> std::convertible_to<std::string>;
  { a.description() } -> std::convertible_to<std::string>;
};

template <typename T, typename Input, typename Output>
concept SolvableAlgorithm = AlgorithmBase<T> && requires(T a, const Input& input) {
  { a.solve(input) } -> std::convertible_to<Output>;
};

/**
 * @brief Base class for algorithms
 *
 * This serves as the common interface for all algorithm implementations.
 */
class Algorithm {
 public:
  virtual ~Algorithm() = default;

  // Common metadata methods for all algorithms
  virtual std::string name() const = 0;
  virtual std::string description() const = 0;
  virtual std::string timeComplexity() const { return "Unknown"; }
  virtual std::string spaceComplexity() const { return "Unknown"; }

  // For tracking recursion depth in algorithms
  virtual int get_max_recursion_depth() const { return 0; }

  // Default time limit in milliseconds (5 minutes)
  static inline int DEFAULT_TIME_LIMIT_MS = 5000 * 60;
};

/**
 * @brief Interface for algorithms with specific input and output types
 *
 * @tparam InputType The type of input data for the algorithm
 * @tparam OutputType The type of output data from the algorithm
 */
template <typename InputType, typename OutputType>
class TypedAlgorithm : public Algorithm {
 public:
  // Solve with time limit
  OutputType solveWithTimeLimit(const InputType& input, int timeoutMs = DEFAULT_TIME_LIMIT_MS) {
    std::packaged_task<OutputType()> task([&]() { return this->solve(input); });
    auto future = task.get_future();

    std::thread t(std::move(task));

    // Wait with timeout
    auto status = future.wait_for(std::chrono::milliseconds(timeoutMs));
    if (status == std::future_status::timeout) {
      // Need to detach because we can't join a thread that might be blocked
      t.detach();
      throw std::runtime_error(
        "Algorithm execution exceeded time limit of " + std::to_string(timeoutMs) + " ms"
      );
    } else {
      t.join();
      return future.get();
    }
  }

  virtual OutputType solve(const InputType& input) = 0;
};

/**
 * @brief Registry for algorithms
 *
 * This class provides a registry for algorithm implementations with type erasure.
 */
class AlgorithmRegistry {
 public:
  using AlgorithmCreator = std::function<std::unique_ptr<Algorithm>()>;

  // Get singleton instance
  static AlgorithmRegistry& instance() {
    static AlgorithmRegistry instance;
    return instance;
  }

  // Register an algorithm creator function - enhanced with concepts
  template <typename T>
  requires std::derived_from<T, Algorithm> static bool registerAlgorithm(std::string_view name) {
    instance().algorithms_[std::string(name)] = []() {
      return std::make_unique<T>();
    };
    return true;
  }

  // Create algorithm by name
  static std::unique_ptr<Algorithm> create(std::string_view name) {
    auto& registry = instance();
    auto it = registry.algorithms_.find(std::string(name));
    if (it == registry.algorithms_.end()) {
      throw std::runtime_error(std::string("Algorithm '") + std::string(name) + "' not found");
    }
    return it->second();
  }

  // Create a typed algorithm by name with improved type safety
  template <typename InputType, typename OutputType>
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>> createTyped(std::string_view name) {
    auto algorithm = create(name);
    auto* typed_algorithm =
      dynamic_cast<TypedAlgorithm<InputType, OutputType>*>(algorithm.release());
    if (!typed_algorithm) {
      throw std::runtime_error(
        std::string("Algorithm '") + std::string(name) + "' has incompatible types"
      );
    }
    return std::unique_ptr<TypedAlgorithm<InputType, OutputType>>(typed_algorithm);
  }

  // Check if an algorithm exists
  static bool exists(std::string_view name) {
    return instance().algorithms_.find(std::string(name)) != instance().algorithms_.end();
  }

  // List all registered algorithms
  static std::vector<std::string> availableAlgorithms() {
    std::vector<std::string> names;
    names.reserve(instance().algorithms_.size());
    for (const auto& [name, _] : instance().algorithms_) {
      names.push_back(name);
    }
    return names;
  }

  // Get algorithm description
  static std::string getDescription(std::string_view name) {
    if (!exists(name)) {
      return "Algorithm not available";
    }
    auto algo = create(name);
    return algo->description();
  }

  // Get algorithm time complexity
  static std::string getTimeComplexity(std::string_view name) {
    if (!exists(name)) {
      return "Algorithm not available";
    }
    auto algo = create(name);
    return algo->timeComplexity();
  }

  // List all algorithms with their descriptions
  static void listAlgorithms(std::ostream& _ = std::cout) {
    // Use UI::header for the title
    UI::header("Available Algorithms");

    // Create a tabulate table for better formatting
    tabulate::Table table;

    // Add the header row
    table.add_row({"Name", "Description", "Time Complexity"});

    // Format the header row with colors and alignment
    table[0]
      .format()
      .font_style({tabulate::FontStyle::bold})
      .font_color(tabulate::Color::green)
      .font_align(tabulate::FontAlign::center);

    // Add each algorithm to the table
    for (const auto& name : availableAlgorithms()) {
      try {
        auto algo = create(name);
        table.add_row({name, algo->description(), algo->timeComplexity()});
      } catch (const std::exception& e) {
        table.add_row({name, "Error: " + std::string(e.what()), "Unknown"});
      }
    }

    // Set column alignment
    for (size_t i = 1; i < table.size(); ++i) {
      table[i][0].format().font_align(tabulate::FontAlign::left);
      table[i][1].format().font_align(tabulate::FontAlign::left);
      table[i][2].format().font_align(tabulate::FontAlign::right).font_color(tabulate::Color::cyan);
    }

    // Set column widths
    table.column(0).format().width(25);
    table.column(1).format().width(50);
    table.column(2).format().width(50);

    // Print the table
    std::cout << table << std::endl;
  }

  // Get algorithms of specific types
  static std::vector<std::string> getAvailableGenerators() {
    std::vector<std::string> generators;
    for (const auto& [name, _] : instance().algorithms_) {
      if (name.find("Generator") != std::string::npos) {
        generators.push_back(name);
      }
    }
    return generators;
  }

  static std::vector<std::string> getAvailableSearches() {
    std::vector<std::string> searches;
    for (const auto& [name, _] : instance().algorithms_) {
      if (name.find("Search") != std::string::npos) {
        searches.push_back(name);
      }
    }
    return searches;
  }

 private:
  // Private constructor for singleton
  AlgorithmRegistry() = default;

  // Store algorithm creators
  std::unordered_map<std::string, AlgorithmCreator> algorithms_;
};

/**
 * @brief Helper for simplified algorithm registration with type deduction
 *
 * @tparam T Algorithm implementation class
 */
template <typename T>
requires std::derived_from<T, Algorithm> struct AlgorithmRegistrar {
  explicit AlgorithmRegistrar(std::string_view name) {
    AlgorithmRegistry::registerAlgorithm<T>(name);
  }
};

/**
 * @brief Factory for creating algorithms with specific input/output types
 *
 * @tparam InputType Algorithm input type
 * @tparam OutputType Algorithm output type
 */
template <typename InputType, typename OutputType>
class TypedAlgorithmFactory {
 public:
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>> create(std::string_view name) {
    return AlgorithmRegistry::createTyped<InputType, OutputType>(name);
  }

  template <typename T>
  requires SolvableAlgorithm<T, InputType, OutputType>&&
    std::derived_from<T, TypedAlgorithm<InputType, OutputType>> static bool
    registerAlgorithm(std::string_view name) {
    return AlgorithmRegistry::registerAlgorithm<T>(name);
  }
};

// Enhanced macro for algorithm registration
#define REGISTER_ALGORITHM(className, name)                                    \
  namespace {                                                                  \
  static const daa::AlgorithmRegistrar<className> className##_registrar(name); \
  }

}  // namespace daa