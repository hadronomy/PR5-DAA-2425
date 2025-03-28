#pragma once

#include <concepts>
#include <string>

namespace meta {

// Concept for problem types
template <typename P>
concept Problem = requires {
  typename P::SolutionType;  // Problem must define its solution type
};

// Concept for solution types
template <typename S, typename P>
concept Solution = requires(S s, P p) {
  typename P::SolutionType;  // Access the solution type from the problem
  {
    p.evaluateSolution(s)
  } -> std::convertible_to<double>;  // Problem can evaluate a solution's quality
};

/**
 * @brief Interface for solution generation strategies
 *
 * @tparam S Solution type
 * @tparam P Problem type
 */
template <typename S, typename P>
requires Solution<S, P> class SolutionGenerator {
 public:
  virtual ~SolutionGenerator() = default;

  /**
   * @brief Generate an initial solution for the problem
   * @param problem The problem instance
   * @return A valid solution
   */
  virtual S generateSolution(const P& problem) = 0;

  /**
   * @brief Get the name of this generator
   */
  virtual std::string name() const = 0;
};

/**
 * @brief Interface for local search strategies
 *
 * @tparam S Solution type
 * @tparam P Problem type
 */
template <typename S, typename P>
requires Solution<S, P> class LocalSearch {
 public:
  virtual ~LocalSearch() = default;

  /**
   * @brief Improve an existing solution
   * @param problem The problem instance
   * @param initialSolution The starting solution to improve
   * @return An improved solution
   */
  virtual S improveSolution(const P& problem, const S& initialSolution) = 0;

  /**
   * @brief Get the name of this local search strategy
   */
  virtual std::string name() const = 0;
};

}  // namespace meta
