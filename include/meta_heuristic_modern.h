#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <expected>
#include <format>

#include "meta_heuristic_components.h"

namespace daa::modern {

namespace meta {

// Rename the concept to avoid conflict with Algorithm in algorithm_registry.h
template <typename A, typename S, typename P>
concept MetaAlgorithm = requires(A a, const P& p) {
  { a.solve(p) } -> std::convertible_to<S>;
  { a.name() } -> std::convertible_to<std::string>;
  { a.description() } -> std::convertible_to<std::string>;
};

} // namespace meta

/**
 * @brief Modern base class for meta-heuristic algorithms
 *
 * @tparam S Solution type
 * @tparam P Problem type
 * @tparam A Algorithm base class
 */
template <typename S, typename P, typename A>
requires ::meta::Solution<S, P> && meta::MetaAlgorithm<A, S, P>
class MetaHeuristic : public A {
public:
  // Error types
  enum class Error {
    GeneratorFailed,
    SearchFailed,
    InvalidComponents
  };
  
  // Result type for operations that can fail
  using Result = std::expected<S, Error>;

  /**
   * @brief Construct a new meta-heuristic algorithm
   *
   * @param generator Strategy for generating initial solutions
   * @param localSearch Strategy for improving solutions
   */
  MetaHeuristic(
    std::unique_ptr<::meta::SolutionGenerator<S, P>> generator,
    std::unique_ptr<::meta::LocalSearch<S, P>> localSearch
  )
    : generator_(std::move(generator)), localSearch_(std::move(localSearch)) {
    
    if (!generator_ || !localSearch_) {
      throw std::invalid_argument("Generator and local search must not be null");
    }
  }
  
  /**
   * @brief Move constructor
   */
  MetaHeuristic(MetaHeuristic&&) noexcept = default;
  
  /**
   * @brief Move assignment
   */
  MetaHeuristic& operator=(MetaHeuristic&&) noexcept = default;
  
  /**
   * @brief No copying
   */
  MetaHeuristic(const MetaHeuristic&) = delete;
  MetaHeuristic& operator=(const MetaHeuristic&) = delete;

  /**
   * @brief Solve the problem using the meta-heuristic approach
   *
   * First generates an initial solution, then applies local search
   * to improve it.
   * 
   * @param problem The problem to solve
   * @return S The solution
   */
  S solve(const P& problem) override {
    // Generate the initial solution
    S initialSolution = generator_->generateSolution(problem);

    // Apply local search to improve the solution
    return localSearch_->improveSolution(problem, initialSolution);
  }
  
  /**
   * @brief Solve the problem with error handling
   * 
   * @param problem The problem to solve
   * @return Result<S> The solution or an error
   */
  Result solveWithErrorHandling(const P& problem) {
    if (!generator_ || !localSearch_) {
      return std::unexpected(Error::InvalidComponents);
    }
    
    try {
      // Generate the initial solution
      S initialSolution = generator_->generateSolution(problem);
      
      try {
        // Apply local search to improve the solution
        return localSearch_->improveSolution(problem, initialSolution);
      } catch (const std::exception&) {
        return std::unexpected(Error::SearchFailed);
      }
    } catch (const std::exception&) {
      return std::unexpected(Error::GeneratorFailed);
    }
  }

  /**
   * @brief Get the name of this meta-heuristic
   * 
   * @return std::string The name
   */
  std::string name() const override {
    return std::format("{} + {}", generator_->name(), localSearch_->name());
  }

  /**
   * @brief Get the description of this meta-heuristic
   * 
   * @return std::string The description
   */
  std::string description() const override {
    return std::format(
      "Meta-heuristic combining {} for initialization and {} for improvement",
      generator_->name(), localSearch_->name()
    );
  }
  
  /**
   * @brief Render UI components for configuring this meta-heuristic
   */
  void renderConfigurationUI() override {
    if (generator_) {
      generator_->renderConfigurationUI();
    }
    
    if (localSearch_) {
      localSearch_->renderConfigurationUI();
    }
  }
  
  /**
   * @brief Get the generator
   * 
   * @return ::meta::SolutionGenerator<S, P>* The generator
   */
  ::meta::SolutionGenerator<S, P>* getGenerator() const {
    return generator_.get();
  }
  
  /**
   * @brief Get the local search
   * 
   * @return ::meta::LocalSearch<S, P>* The local search
   */
  ::meta::LocalSearch<S, P>* getLocalSearch() const {
    return localSearch_.get();
  }

protected:
  std::unique_ptr<::meta::SolutionGenerator<S, P>> generator_;
  std::unique_ptr<::meta::LocalSearch<S, P>> localSearch_;
};

// Error formatting for std::format
template <typename S, typename P, typename A>
requires ::meta::Solution<S, P> && meta::MetaAlgorithm<A, S, P>
std::string to_string(typename MetaHeuristic<S, P, A>::Error error) {
  using Error = typename MetaHeuristic<S, P, A>::Error;
  
  switch (error) {
    case Error::GeneratorFailed:
      return "Solution generation failed";
    case Error::SearchFailed:
      return "Local search failed";
    case Error::InvalidComponents:
      return "Invalid meta-heuristic components";
    default:
      return "Unknown error";
  }
}

// Register a meta-heuristic as a regular algorithm
#define REGISTER_META_ALGORITHM_MODERN(className, name, ...) \
  REGISTER_ALGORITHM_MODERN(className, name, ##__VA_ARGS__)

} // namespace daa::modern
