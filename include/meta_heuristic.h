#pragma once

#include <concepts>
#include <memory>
#include <string>
#include "meta_heuristic_components.h"

namespace daa {

// Concept for algorithm interfaces - more concise with using the refined concept
template <typename A, typename S, typename P>
concept Algorithm = requires(A a, const P& p) {
  { a.solve(p) } -> std::convertible_to<S>;
  { a.name() } -> std::convertible_to<std::string>;
  { a.description() } -> std::convertible_to<std::string>;
};

/**
 * @brief Base class for meta-heuristic algorithms
 *
 * @tparam S Solution type
 * @tparam P Problem type
 * @tparam A Algorithm base class
 */
template <typename S, typename P, typename A>
requires meta::Solution<S, P>&& Algorithm<A, S, P> class MetaHeuristic : public A {
 protected:
  std::unique_ptr<meta::SolutionGenerator<S, P>> generator_;
  std::unique_ptr<meta::LocalSearch<S, P>> localSearch_;

 public:
  /**
   * @brief Construct a new meta-heuristic algorithm
   *
   * @param generator Strategy for generating initial solutions
   * @param localSearch Strategy for improving solutions
   */
  MetaHeuristic(
    std::unique_ptr<meta::SolutionGenerator<S, P>> generator,
    std::unique_ptr<meta::LocalSearch<S, P>> localSearch
  )
      : generator_(std::move(generator)), localSearch_(std::move(localSearch)) {}

  /**
   * @brief Solve the problem using the meta-heuristic approach
   *
   * First generates an initial solution, then applies local search
   * to improve it.
   */
  S solve(const P& problem) override {
    // Generate the initial solution
    S initialSolution = generator_->generateSolution(problem);

    // Apply local search to improve the solution
    return localSearch_->improveSolution(problem, initialSolution);
  }

  std::string name() const override { return generator_->name() + " + " + localSearch_->name(); }

  std::string description() const override {
    return "Meta-heuristic combining " + generator_->name() +
           " for initialization "
           "and " +
           localSearch_->name() + " for improvement";
  }
};

// Register a meta-heuristic as a regular algorithm
#define REGISTER_META_ALGORITHM(className, name) REGISTER_ALGORITHM(className, name)

}  // namespace daa