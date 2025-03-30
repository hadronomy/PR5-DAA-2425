#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "meta_heuristic.h"

namespace daa {

/**
 * @brief Factory for creating meta-heuristic algorithm components and combinations
 *
 * @tparam S Solution type
 * @tparam P Problem type
 * @tparam A Algorithm base class
 */
template <typename S, typename P, typename A>
requires ::meta::Solution<S, P>&& daa::meta::MetaAlgorithm<A, S, P> class MetaHeuristicFactory {
 private:
  using Generator = ::meta::SolutionGenerator<S, P>;
  using Search = ::meta::LocalSearch<S, P>;

  using GeneratorCreator = std::function<std::unique_ptr<Generator>()>;
  using SearchCreator = std::function<std::unique_ptr<Search>()>;

  static MetaHeuristicFactory& instance() {
    static MetaHeuristicFactory factory;
    return factory;
  }

  std::unordered_map<std::string, GeneratorCreator> generatorCreators_;
  std::unordered_map<std::string, SearchCreator> searchCreators_;

 public:
  /**
   * @brief Register a solution generator type
   */
  template <typename T>
  static bool registerGenerator(const std::string& name) {
    return instance().generatorCreators_.emplace(
                                          name, []() { return std::make_unique<T>(); }
    ).second;
  }

  /**
   * @brief Register a local search type
   */
  template <typename T>
  static bool registerSearch(const std::string& name) {
    return instance().searchCreators_.emplace(name, []() { return std::make_unique<T>(); }).second;
  }

  /**
   * @brief Create a generator by name
   */
  static std::unique_ptr<Generator> createGenerator(const std::string& name) {
    auto& creators = instance().generatorCreators_;
    auto it = creators.find(name);
    if (it == creators.end()) {
      throw std::runtime_error("Unknown generator: " + name);
    }
    return it->second();
  }

  /**
   * @brief Create a local search strategy by name
   */
  static std::unique_ptr<Search> createSearch(const std::string& name) {
    auto& creators = instance().searchCreators_;
    auto it = creators.find(name);
    if (it == creators.end()) {
      throw std::runtime_error("Unknown local search: " + name);
    }
    return it->second();
  }

  /**
   * @brief Create a meta-heuristic algorithm with specified components
   */
  static std::unique_ptr<MetaHeuristic<S, P, A>>
    createMetaHeuristic(const std::string& generatorName, const std::string& searchName) {
    return std::make_unique<MetaHeuristic<S, P, A>>(
      createGenerator(generatorName), createSearch(searchName)
    );
  }

  /**
   * @brief Get list of available generators
   */
  static std::vector<std::string> availableGenerators() {
    std::vector<std::string> result;
    for (const auto& [name, _] : instance().generatorCreators_) {
      result.push_back(name);
    }
    return result;
  }

  /**
   * @brief Get list of available local searches
   */
  static std::vector<std::string> availableSearches() {
    std::vector<std::string> result;
    for (const auto& [name, _] : instance().searchCreators_) {
      result.push_back(name);
    }
    return result;
  }
};

// Helper macros for registration
// Basic versions that use class name as the registration name
#define REGISTER_META_GENERATOR(Solution, Problem, Algorithm, ClassName) \
  inline static bool ClassName##_registered_gen =                        \
    MetaHeuristicFactory<Solution, Problem, Algorithm>::registerGenerator<ClassName>(#ClassName)

#define REGISTER_META_SEARCH(Solution, Problem, Algorithm, ClassName) \
  inline static bool ClassName##_registered_search =                  \
    MetaHeuristicFactory<Solution, Problem, Algorithm>::registerSearch<ClassName>(#ClassName)

// Extended versions that allow custom names
#define REGISTER_META_GENERATOR_NAMED(Solution, Problem, Algorithm, ClassName, Name) \
  inline static bool ClassName##_registered_gen =                                    \
    MetaHeuristicFactory<Solution, Problem, Algorithm>::registerGenerator<ClassName>(Name)

#define REGISTER_META_SEARCH_NAMED(Solution, Problem, Algorithm, ClassName, Name) \
  inline static bool ClassName##_registered_search =                              \
    MetaHeuristicFactory<Solution, Problem, Algorithm>::registerSearch<ClassName>(Name)

}  // namespace daa