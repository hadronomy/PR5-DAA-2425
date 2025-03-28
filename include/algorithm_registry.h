#pragma once
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ui.h"

namespace daa {

// Forward declaration
template <typename T>
class DataGenerator;

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
  using BenchmarkFunction = std::function<void(const std::string&, int, int, bool, int timeLimit)>;

  // Get singleton instance
  static AlgorithmRegistry& instance() {
    static AlgorithmRegistry instance;
    return instance;
  }

  // Register an algorithm creator function
  template <typename T>
  static bool registerAlgorithm(const std::string& name) {
    instance().algorithms_[name] = []() {
      return std::make_unique<T>();
    };
    return true;
  }

  // Create algorithm by name
  static std::unique_ptr<Algorithm> create(const std::string& name) {
    auto& registry = instance();
    auto it = registry.algorithms_.find(name);
    if (it == registry.algorithms_.end()) {
      throw std::runtime_error("Algorithm '" + name + "' not found");
    }
    return it->second();
  }

  // Create a typed algorithm by name
  template <typename InputType, typename OutputType>
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>> createTyped(const std::string& name
  ) {
    auto algorithm = create(name);
    auto* typed_algorithm =
      dynamic_cast<TypedAlgorithm<InputType, OutputType>*>(algorithm.release());
    if (!typed_algorithm) {
      throw std::runtime_error("Algorithm '" + name + "' has incompatible types");
    }
    return std::unique_ptr<TypedAlgorithm<InputType, OutputType>>(typed_algorithm);
  }

  // Check if an algorithm exists
  static bool exists(const std::string& name) {
    return instance().algorithms_.find(name) != instance().algorithms_.end();
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
  static std::string getDescription(const std::string& name) {
    if (!exists(name)) {
      return "Algorithm not available";
    }
    auto algo = create(name);
    return algo->description();
  }

  // Get algorithm time complexity
  static std::string getTimeComplexity(const std::string& name) {
    if (!exists(name)) {
      return "Algorithm not available";
    }
    auto algo = create(name);
    return algo->timeComplexity();
  }

  // Register a benchmark function
  static void registerBenchmarkFunction(const std::string& name, BenchmarkFunction function) {
    instance().benchmark_functions_[name] = function;
  }

  // Register a benchmark function with input/output types
  template <typename InputType, typename OutputType>
  static void register_benchmark_function(const std::string& name, BenchmarkFunction function) {
    instance().benchmark_functions_[name] = function;
  }

  // Run a benchmark for an algorithm
  static bool runBenchmark(
    const std::string& name,
    int iterations,
    int test_size,
    bool debug,
    int time_limit = Algorithm::DEFAULT_TIME_LIMIT_MS
  ) {
    auto& benchmark_functions = instance().benchmark_functions_;
    auto it = benchmark_functions.find(name);
    if (it != benchmark_functions.end()) {
      it->second(name, iterations, test_size, debug, time_limit);
      return true;
    }
    return false;
  }

  // Register a data generator
  template <typename InputType>
  static void registerDataGenerator(
    const std::string& name,
    std::unique_ptr<DataGenerator<InputType>> generator
  ) {
    instance().register_data_generator(name, std::move(generator));
  }

  // Register a data generator (instance method)
  template <typename InputType>
  void register_data_generator(
    const std::string& name,
    std::unique_ptr<DataGenerator<InputType>> generator
  ) {
    auto shared_gen = std::shared_ptr<DataGenerator<InputType>>(generator.release());
    data_generators_[name] = [shared_gen](int size) -> void* {
      return new InputType(shared_gen->generate(size));
    };
  }

  // Generate test data for an algorithm
  template <typename InputType>
  static InputType generateData(const std::string& name, int size) {
    auto& data_generators = instance().data_generators_;
    auto it = data_generators.find(name);

    if (it == data_generators.end()) {
      throw std::runtime_error("Data generator not found for: " + name);
    }

    void* raw_ptr = it->second(size);
    InputType* data_ptr = static_cast<InputType*>(raw_ptr);
    InputType result = *data_ptr;
    delete data_ptr;
    return result;
  }

  // List all algorithms with their descriptions
  static void listAlgorithms(std::ostream& os = std::cout) {
    auto& registry = instance();

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

 private:
  // Private constructor for singleton
  AlgorithmRegistry() = default;

  // Store algorithm creators
  std::unordered_map<std::string, AlgorithmCreator> algorithms_;

  // Store benchmark functions
  std::unordered_map<std::string, BenchmarkFunction> benchmark_functions_;

  // Store data generators
  std::unordered_map<std::string, std::function<void*(int)>> data_generators_;
};

// Helper macro for algorithm registration
#define REGISTER_ALGORITHM(className, name) \
  static bool className##_registered = AlgorithmRegistry::registerAlgorithm<className>(name)

}  // namespace daa