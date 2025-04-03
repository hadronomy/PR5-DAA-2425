#pragma once

#include <any>
#include <concepts>
#include <expected>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "meta_heuristic_components.h"
#include "meta_heuristic_modern.h"

namespace daa::modern {

/**
 * @brief Modern factory for creating meta-heuristic algorithm components
 *
 * @tparam S Solution type
 * @tparam P Problem type
 * @tparam A Algorithm base class
 */
template <typename S, typename P, typename A>
requires ::meta::Solution<S, P>&& meta::MetaAlgorithm<A, S, P> class MetaHeuristicFactory {
 public:
  // Error types
  enum class Error { UnknownGenerator, UnknownSearch, CreationFailed };

  // Result type for operations that can fail
  template <typename T>
  using Result = std::expected<T, Error>;

 private:
  using Generator = ::meta::SolutionGenerator<S, P>;
  using Search = ::meta::LocalSearch<S, P>;

  // Type-erased factory for creating generator instances
  class GeneratorFactory {
   public:
    virtual ~GeneratorFactory() = default;
    virtual std::unique_ptr<Generator> create() const = 0;
    virtual std::unique_ptr<Generator> createWithArgs(const std::vector<std::any>& args) const = 0;
    virtual std::string getSignature() const = 0;
  };

  // Type-erased factory for creating search instances
  class SearchFactory {
   public:
    virtual ~SearchFactory() = default;
    virtual std::unique_ptr<Search> create() const = 0;
    virtual std::unique_ptr<Search> createWithArgs(const std::vector<std::any>& args) const = 0;
    virtual std::string getSignature() const = 0;
  };

  // Concrete factory for creating generator instances
  template <typename T, typename... Args>
  class ConcreteGeneratorFactory : public GeneratorFactory {
   public:
    explicit ConcreteGeneratorFactory(std::tuple<Args...> default_args)
        : default_args_(std::move(default_args)) {}

    std::unique_ptr<Generator> create() const override {
      return createFromTuple(std::index_sequence_for<Args...>{});
    }

    std::unique_ptr<Generator> createWithArgs(const std::vector<std::any>& args) const override {
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
    std::unique_ptr<Generator> createFromTuple(std::index_sequence<Is...>) const {
      return std::make_unique<T>(std::get<Is>(default_args_)...);
    }

    template <size_t... Is>
    std::unique_ptr<Generator>
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

  // Concrete factory for creating search instances
  template <typename T, typename... Args>
  class ConcreteSearchFactory : public SearchFactory {
   public:
    explicit ConcreteSearchFactory(std::tuple<Args...> default_args)
        : default_args_(std::move(default_args)) {}

    std::unique_ptr<Search> create() const override {
      return createFromTuple(std::index_sequence_for<Args...>{});
    }

    std::unique_ptr<Search> createWithArgs(const std::vector<std::any>& args) const override {
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
    std::unique_ptr<Search> createFromTuple(std::index_sequence<Is...>) const {
      return std::make_unique<T>(std::get<Is>(default_args_)...);
    }

    template <size_t... Is>
    std::unique_ptr<Search>
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

  static MetaHeuristicFactory& instance() {
    static MetaHeuristicFactory factory;
    return factory;
  }

  std::unordered_map<std::string, std::unique_ptr<GeneratorFactory>> generatorFactories_;
  std::unordered_map<std::string, std::unique_ptr<SearchFactory>> searchFactories_;
  std::unordered_map<std::string, std::string> generatorSignatures_;
  std::unordered_map<std::string, std::string> searchSignatures_;

 public:
  /**
   * @brief Register a solution generator type with constructor arguments
   *
   * @tparam T Generator type
   * @tparam Args Constructor argument types
   * @param name Generator name
   * @param args Default constructor arguments
   * @return bool True if registration succeeded
   */
  template <typename T, typename... Args>
  static bool registerGenerator(const std::string& name, Args&&... args) {
    auto& factory = instance();
    auto generatorFactory = std::make_unique<ConcreteGeneratorFactory<T, std::decay_t<Args>...>>(
      std::make_tuple(std::forward<Args>(args)...)
    );

    auto signature = generatorFactory->getSignature();

    factory.generatorFactories_[name] = std::move(generatorFactory);
    factory.generatorSignatures_[name] = signature;

    return true;
  }

  /**
   * @brief Register a local search type with constructor arguments
   *
   * @tparam T Search type
   * @tparam Args Constructor argument types
   * @param name Search name
   * @param args Default constructor arguments
   * @return bool True if registration succeeded
   */
  template <typename T, typename... Args>
  static bool registerSearch(const std::string& name, Args&&... args) {
    auto& factory = instance();
    auto searchFactory = std::make_unique<ConcreteSearchFactory<T, std::decay_t<Args>...>>(
      std::make_tuple(std::forward<Args>(args)...)
    );

    auto signature = searchFactory->getSignature();

    factory.searchFactories_[name] = std::move(searchFactory);
    factory.searchSignatures_[name] = signature;

    return true;
  }

  /**
   * @brief Create a generator by name
   *
   * @param name Generator name
   * @return Result<std::unique_ptr<Generator>> The generator or an error
   */
  static Result<std::unique_ptr<Generator>> createGenerator(const std::string& name) {
    auto& factory = instance();
    auto it = factory.generatorFactories_.find(name);
    if (it == factory.generatorFactories_.end()) {
      return std::unexpected(Error::UnknownGenerator);
    }

    try {
      return it->second->create();
    } catch (const std::exception&) {
      return std::unexpected(Error::CreationFailed);
    }
  }

  /**
   * @brief Create a generator by name with specific arguments
   *
   * @param name Generator name
   * @param args Constructor arguments
   * @return Result<std::unique_ptr<Generator>> The generator or an error
   */
  static Result<std::unique_ptr<Generator>>
    createGeneratorWithArgs(const std::string& name, const std::vector<std::any>& args) {
    auto& factory = instance();
    auto it = factory.generatorFactories_.find(name);
    if (it == factory.generatorFactories_.end()) {
      return std::unexpected(Error::UnknownGenerator);
    }

    try {
      return it->second->createWithArgs(args);
    } catch (const std::exception&) {
      return std::unexpected(Error::CreationFailed);
    }
  }

  /**
   * @brief Create a local search strategy by name
   *
   * @param name Search name
   * @return Result<std::unique_ptr<Search>> The search or an error
   */
  static Result<std::unique_ptr<Search>> createSearch(const std::string& name) {
    auto& factory = instance();
    auto it = factory.searchFactories_.find(name);
    if (it == factory.searchFactories_.end()) {
      return std::unexpected(Error::UnknownSearch);
    }

    try {
      return it->second->create();
    } catch (const std::exception&) {
      return std::unexpected(Error::CreationFailed);
    }
  }

  /**
   * @brief Create a local search strategy by name with specific arguments
   *
   * @param name Search name
   * @param args Constructor arguments
   * @return Result<std::unique_ptr<Search>> The search or an error
   */
  static Result<std::unique_ptr<Search>>
    createSearchWithArgs(const std::string& name, const std::vector<std::any>& args) {
    auto& factory = instance();
    auto it = factory.searchFactories_.find(name);
    if (it == factory.searchFactories_.end()) {
      return std::unexpected(Error::UnknownSearch);
    }

    try {
      return it->second->createWithArgs(args);
    } catch (const std::exception&) {
      return std::unexpected(Error::CreationFailed);
    }
  }

  /**
   * @brief Create a meta-heuristic algorithm with specified components
   *
   * @param generatorName Generator name
   * @param searchName Search name
   * @return Result<std::unique_ptr<MetaHeuristic<S, P, A>>> The meta-heuristic or an error
   */
  static Result<std::unique_ptr<MetaHeuristic<S, P, A>>>
    createMetaHeuristic(const std::string& generatorName, const std::string& searchName) {
    auto generatorResult = createGenerator(generatorName);
    if (!generatorResult) {
      return std::unexpected(generatorResult.error());
    }

    auto searchResult = createSearch(searchName);
    if (!searchResult) {
      return std::unexpected(searchResult.error());
    }

    return std::make_unique<MetaHeuristic<S, P, A>>(
      std::move(*generatorResult), std::move(*searchResult)
    );
  }

  /**
   * @brief Get the constructor signature for a generator
   *
   * @param name Generator name
   * @return std::string The constructor signature
   */
  static std::string getGeneratorSignature(const std::string& name) {
    auto& factory = instance();
    auto it = factory.generatorSignatures_.find(name);
    if (it == factory.generatorSignatures_.end()) {
      return "Unknown generator";
    }
    return it->second;
  }

  /**
   * @brief Get the constructor signature for a search
   *
   * @param name Search name
   * @return std::string The constructor signature
   */
  static std::string getSearchSignature(const std::string& name) {
    auto& factory = instance();
    auto it = factory.searchSignatures_.find(name);
    if (it == factory.searchSignatures_.end()) {
      return "Unknown search";
    }
    return it->second;
  }

  /**
   * @brief Get list of available generators
   *
   * @return std::vector<std::string> List of generator names
   */
  static std::vector<std::string> availableGenerators() {
    std::vector<std::string> result;
    auto& factory = instance();
    result.reserve(factory.generatorFactories_.size());
    for (const auto& [name, _] : factory.generatorFactories_) {
      result.push_back(name);
    }
    return result;
  }

  /**
   * @brief Get list of available local searches
   *
   * @return std::vector<std::string> List of search names
   */
  static std::vector<std::string> availableSearches() {
    std::vector<std::string> result;
    auto& factory = instance();
    result.reserve(factory.searchFactories_.size());
    for (const auto& [name, _] : factory.searchFactories_) {
      result.push_back(name);
    }
    return result;
  }
};

// Error formatting for std::format
template <typename S, typename P, typename A>
requires ::meta::Solution<S, P>&& meta::MetaAlgorithm<A, S, P> std::string to_string(
  typename MetaHeuristicFactory<S, P, A>::Error error
) {
  using Error = typename MetaHeuristicFactory<S, P, A>::Error;

  switch (error) {
    case Error::UnknownGenerator:
      return "Unknown generator";
    case Error::UnknownSearch:
      return "Unknown search";
    case Error::CreationFailed:
      return "Component creation failed";
    default:
      return "Unknown error";
  }
}

// Helper macros for registration with variadic arguments
// Directly use the types without typedefs to handle namespaced types
#define REGISTER_META_GENERATOR_MODERN(Solution, Problem, Algorithm, ClassName, Name, ...)         \
  inline static bool ClassName##_registered_gen =                                                  \
    daa::modern::MetaHeuristicFactory<Solution, Problem, Algorithm>::registerGenerator<ClassName>( \
      Name, ##__VA_ARGS__                                                                          \
    )

#define REGISTER_META_SEARCH_MODERN(Solution, Problem, Algorithm, ClassName, Name, ...)         \
  inline static bool ClassName##_registered_search =                                            \
    daa::modern::MetaHeuristicFactory<Solution, Problem, Algorithm>::registerSearch<ClassName>( \
      Name, ##__VA_ARGS__                                                                       \
    )

}  // namespace daa::modern
