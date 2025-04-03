#pragma once

#include <any>
#include <chrono>
#include <concepts>
#include <expected>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "algorithms/vrpt_solution.h"
#include "meta_heuristic_factory.h"
#include "ui.h"

namespace daa {

// Forward declaration
template <typename T>
class DataGenerator;

/**
 * @brief Modern algorithm registry using C++23 design patterns
 *
 * This namespace contains the modern algorithm registry implementation
 * that supports variadic constructor arguments.
 */
namespace modern {

/**
 * @brief Concepts for algorithms and their capabilities
 */
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

  // UI configuration rendering method
  virtual void renderConfigurationUI() {}

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
  // Error types that can be returned from operations
  enum class Error { Timeout, ExecutionFailed };

  // Result type for operations that can fail
  using Result = std::expected<OutputType, Error>;

  /**
   * @brief Solve with time limit
   *
   * @param input The input data
   * @param timeoutMs Timeout in milliseconds
   * @return Result<OutputType> The result or an error
   */
  Result solveWithTimeLimit(const InputType& input, int timeoutMs = DEFAULT_TIME_LIMIT_MS) {
    std::packaged_task<OutputType()> task([&]() { return this->solve(input); });
    auto future = task.get_future();

    std::thread t(std::move(task));

    // Wait with timeout
    auto status = future.wait_for(std::chrono::milliseconds(timeoutMs));
    if (status == std::future_status::timeout) {
      // Need to detach because we can't join a thread that might be blocked
      t.detach();
      return std::unexpected(Error::Timeout);
    } else {
      t.join();
      try {
        return future.get();
      } catch (const std::exception&) {
        return std::unexpected(Error::ExecutionFailed);
      }
    }
  }

  /**
   * @brief Solve the problem
   *
   * @param input The input data
   * @return OutputType The solution
   */
  virtual OutputType solve(const InputType& input) = 0;
};

// Error formatting for std::format
template <typename InputType, typename OutputType>
std::string to_string(typename TypedAlgorithm<InputType, OutputType>::Error error) {
  using Error = typename TypedAlgorithm<InputType, OutputType>::Error;

  switch (error) {
    case Error::Timeout:
      return "Algorithm execution timed out";
    case Error::ExecutionFailed:
      return "Algorithm execution failed";
    default:
      return "Unknown error";
  }
}

/**
 * @brief Type-erased factory for creating algorithm instances
 *
 * This class provides a type-erased interface for creating algorithm instances
 * with arbitrary constructor arguments.
 */
class AlgorithmFactory {
 public:
  virtual ~AlgorithmFactory() = default;
  virtual std::unique_ptr<Algorithm> create() const = 0;
  virtual std::unique_ptr<Algorithm> createWithArgs(const std::vector<std::any>& args) const = 0;
  virtual std::string getSignature() const = 0;
};

/**
 * @brief Concrete factory for creating algorithm instances
 *
 * @tparam T The algorithm type
 * @tparam Args Constructor argument types
 */
template <typename T, typename... Args>
class ConcreteAlgorithmFactory : public AlgorithmFactory {
 public:
  explicit ConcreteAlgorithmFactory(std::tuple<Args...> default_args)
      : default_args_(std::move(default_args)) {}

  std::unique_ptr<Algorithm> create() const override {
    return createFromTuple(std::index_sequence_for<Args...>{});
  }

  std::unique_ptr<Algorithm> createWithArgs(const std::vector<std::any>& args) const override {
    if (args.size() != sizeof...(Args)) {
      throw std::invalid_argument(
        std::format("Expected {} arguments, got {}", sizeof...(Args), args.size())
      );
    }

    return createFromAny(args, std::index_sequence_for<Args...>{});
  }

  std::string getSignature() const override { return getTypeSignature<Args...>(); }

 private:
  template <size_t... Is>
  std::unique_ptr<Algorithm> createFromTuple(std::index_sequence<Is...>) const {
    return std::make_unique<T>(std::get<Is>(default_args_)...);
  }

  template <size_t... Is>
  std::unique_ptr<Algorithm>
    createFromAny(const std::vector<std::any>& args, std::index_sequence<Is...>) const {
    return std::make_unique<T>(std::any_cast<Args>(args[Is])...);
  }

  template <typename... Types>
  static std::string getTypeSignature() {
    std::vector<std::string> type_names;
    (type_names.push_back(typeid(Types).name()), ...);

    std::string result = "(";
    for (size_t i = 0; i < type_names.size(); ++i) {
      result += type_names[i];
      if (i < type_names.size() - 1) {
        result += ", ";
      }
    }
    result += ")";
    return result;
  }

  std::tuple<Args...> default_args_;
};

/**
 * @brief Modern algorithm registry
 *
 * This class provides a registry for algorithm implementations with support
 * for variadic constructor arguments.
 */
class AlgorithmRegistry {
 public:
  // Get singleton instance
  static AlgorithmRegistry& instance() {
    static AlgorithmRegistry instance;
    return instance;
  }

  /**
   * @brief Register an algorithm with constructor arguments
   *
   * @tparam T Algorithm type
   * @tparam Args Constructor argument types
   * @param name Algorithm name
   * @param default_args Default constructor arguments
   * @return bool True if registration succeeded
   */
  template <typename T, typename... Args>
  static bool registerAlgorithm(std::string_view name, Args&&... default_args) {
    auto& registry = instance();
    auto factory = std::make_unique<ConcreteAlgorithmFactory<T, std::decay_t<Args>...>>(
      std::make_tuple(std::forward<Args>(default_args)...)
    );

    auto signature = factory->getSignature();
    auto key = std::string(name);

    registry.factories_[key] = std::move(factory);
    registry.signatures_[key] = signature;

    return true;
  }

  /**
   * @brief Create an algorithm instance
   *
   * @param name Algorithm name
   * @return std::unique_ptr<Algorithm> The algorithm instance
   */
  static std::unique_ptr<Algorithm> create(std::string_view name) {
    auto& registry = instance();
    auto it = registry.factories_.find(std::string(name));
    if (it == registry.factories_.end()) {
      throw std::runtime_error(std::format("Algorithm '{}' not found", name));
    }
    return it->second->create();
  }

  /**
   * @brief Create an algorithm instance with specific arguments
   *
   * @param name Algorithm name
   * @param args Constructor arguments
   * @return std::unique_ptr<Algorithm> The algorithm instance
   */
  static std::unique_ptr<Algorithm>
    createWithArgs(std::string_view name, const std::vector<std::any>& args) {
    auto& registry = instance();
    auto it = registry.factories_.find(std::string(name));
    if (it == registry.factories_.end()) {
      throw std::runtime_error(std::format("Algorithm '{}' not found", name));
    }
    return it->second->createWithArgs(args);
  }

  /**
   * @brief Create a typed algorithm instance
   *
   * @tparam InputType Algorithm input type
   * @tparam OutputType Algorithm output type
   * @param name Algorithm name
   * @return std::unique_ptr<TypedAlgorithm<InputType, OutputType>> The typed algorithm instance
   */
  template <typename InputType, typename OutputType>
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>> createTyped(std::string_view name) {
    auto algorithm = create(name);
    auto* typed_algorithm =
      dynamic_cast<TypedAlgorithm<InputType, OutputType>*>(algorithm.release());
    if (!typed_algorithm) {
      throw std::runtime_error(std::format("Algorithm '{}' has incompatible types", name));
    }
    return std::unique_ptr<TypedAlgorithm<InputType, OutputType>>(typed_algorithm);
  }

  /**
   * @brief Create a typed algorithm instance with specific arguments
   *
   * @tparam InputType Algorithm input type
   * @tparam OutputType Algorithm output type
   * @tparam Args Constructor argument types
   * @param name Algorithm name
   * @param args Constructor arguments
   * @return std::unique_ptr<TypedAlgorithm<InputType, OutputType>> The typed algorithm instance
   */
  template <typename InputType, typename OutputType, typename... Args>
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>>
    createTypedWith(std::string_view name, Args&&... args) {
    // Convert the arguments to a vector of std::any
    std::vector<std::any> any_args = {std::any(std::forward<Args>(args))...};

    // Create the algorithm with the arguments
    auto algorithm = createWithArgs(name, any_args);
    auto* typed_algorithm =
      dynamic_cast<TypedAlgorithm<InputType, OutputType>*>(algorithm.release());
    if (!typed_algorithm) {
      throw std::runtime_error(std::format("Algorithm '{}' has incompatible types", name));
    }
    return std::unique_ptr<TypedAlgorithm<InputType, OutputType>>(typed_algorithm);
  }

  /**
   * @brief Create a typed algorithm instance with specific arguments (legacy version)
   *
   * @tparam InputType Algorithm input type
   * @tparam OutputType Algorithm output type
   * @param name Algorithm name
   * @param args Constructor arguments
   * @return std::unique_ptr<TypedAlgorithm<InputType, OutputType>> The typed algorithm instance
   */
  template <typename InputType, typename OutputType>
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>>
    createTypedWithArgs(std::string_view name, const std::vector<std::any>& args) {
    auto algorithm = createWithArgs(name, args);
    auto* typed_algorithm =
      dynamic_cast<TypedAlgorithm<InputType, OutputType>*>(algorithm.release());
    if (!typed_algorithm) {
      throw std::runtime_error(std::format("Algorithm '{}' has incompatible types", name));
    }
    return std::unique_ptr<TypedAlgorithm<InputType, OutputType>>(typed_algorithm);
  }

  /**
   * @brief Check if an algorithm exists
   *
   * @param name Algorithm name
   * @return bool True if the algorithm exists
   */
  static bool exists(std::string_view name) {
    return instance().factories_.find(std::string(name)) != instance().factories_.end();
  }

  /**
   * @brief Get the constructor signature for an algorithm
   *
   * @param name Algorithm name
   * @return std::string The constructor signature
   */
  static std::string getSignature(std::string_view name) {
    auto& registry = instance();
    auto it = registry.signatures_.find(std::string(name));
    if (it == registry.signatures_.end()) {
      throw std::runtime_error(std::format("Algorithm '{}' not found", name));
    }
    return it->second;
  }

  /**
   * @brief Get all available algorithm names
   *
   * @return std::vector<std::string> The algorithm names
   */
  static std::vector<std::string> availableAlgorithms() {
    std::vector<std::string> names;
    auto& registry = instance();
    names.reserve(registry.factories_.size());
    for (const auto& [name, _] : registry.factories_) {
      names.push_back(name);
    }
    return names;
  }

  /**
   * @brief Get algorithm description
   *
   * @param name Algorithm name
   * @return std::string The algorithm description
   */
  static std::string getDescription(std::string_view name) {
    if (!exists(name)) {
      return "Algorithm not available";
    }
    auto algo = create(name);
    return algo->description();
  }

  /**
   * @brief Get algorithm time complexity
   *
   * @param name Algorithm name
   * @return std::string The algorithm time complexity
   */
  static std::string getTimeComplexity(std::string_view name) {
    if (!exists(name)) {
      return "Algorithm not available";
    }
    auto algo = create(name);
    return algo->timeComplexity();
  }

 private:
  // Private constructor for singleton
  AlgorithmRegistry() = default;

  // Store algorithm factories and signatures
  std::unordered_map<std::string, std::unique_ptr<AlgorithmFactory>> factories_;
  std::unordered_map<std::string, std::string> signatures_;
};

/**
 * @brief Helper for simplified algorithm registration with type deduction
 *
 * @tparam T Algorithm implementation class
 */
template <typename T, typename... Args>
struct AlgorithmRegistrar {
  explicit AlgorithmRegistrar(std::string_view name, Args&&... default_args) {
    AlgorithmRegistry::registerAlgorithm<T>(name, std::forward<Args>(default_args)...);
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
  /**
   * @brief Create a typed algorithm instance
   *
   * @param name Algorithm name
   * @return std::unique_ptr<TypedAlgorithm<InputType, OutputType>> The typed algorithm instance
   */
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>> create(std::string_view name) {
    return AlgorithmRegistry::createTyped<InputType, OutputType>(name);
  }

  /**
   * @brief Create a typed algorithm instance with specific arguments
   *
   * @tparam Args Constructor argument types
   * @param name Algorithm name
   * @param args Constructor arguments
   * @return std::unique_ptr<TypedAlgorithm<InputType, OutputType>> The typed algorithm instance
   */
  template <typename... Args>
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>>
    createWith(std::string_view name, Args&&... args) {
    return AlgorithmRegistry::createTypedWith<InputType, OutputType>(
      name, std::forward<Args>(args)...
    );
  }

  /**
   * @brief Create a typed algorithm instance with specific arguments (legacy version)
   *
   * @param name Algorithm name
   * @param args Constructor arguments
   * @return std::unique_ptr<TypedAlgorithm<InputType, OutputType>> The typed algorithm instance
   */
  static std::unique_ptr<TypedAlgorithm<InputType, OutputType>>
    createWithArgs(std::string_view name, const std::vector<std::any>& args) {
    return AlgorithmRegistry::createTypedWithArgs<InputType, OutputType>(name, args);
  }

  /**
   * @brief Register an algorithm
   *
   * @tparam T Algorithm type
   * @tparam Args Constructor argument types
   * @param name Algorithm name
   * @param default_args Default constructor arguments
   * @return bool True if registration succeeded
   */
  template <typename T, typename... Args>
  requires SolvableAlgorithm<T, InputType, OutputType>&&
    std::derived_from<T, TypedAlgorithm<InputType, OutputType>> static bool
    registerAlgorithm(std::string_view name, Args&&... default_args) {
    return AlgorithmRegistry::registerAlgorithm<T>(name, std::forward<Args>(default_args)...);
  }
};

}  // namespace modern

// Enhanced macro for algorithm registration with variadic arguments
// Directly use the types without typedefs to handle namespaced types
#define REGISTER_ALGORITHM_MODERN(className, name, ...)                                \
  namespace {                                                                          \
  static const bool className##_registered =                                           \
    daa::modern::AlgorithmRegistry::registerAlgorithm<className>(name, ##__VA_ARGS__); \
  }

// Enhanced macro for typed algorithm registration with variadic arguments
// Directly use the types without typedefs to handle namespaced types
#define REGISTER_TYPED_ALGORITHM_MODERN(className, inputType, outputType, name, ...)         \
  namespace {                                                                                \
  static const bool className##_registered =                                                 \
    daa::modern::TypedAlgorithmFactory<inputType, outputType>::registerAlgorithm<className>( \
      name,                                                                                  \
      ##__VA_ARGS__                                                                          \
    );                                                                                       \
  }

}  // namespace daa
