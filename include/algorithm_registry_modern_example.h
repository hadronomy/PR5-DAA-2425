#pragma once

#include <format>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "algorithm_registry_modern.h"
#include "algorithms/vrpt_solution.h"
#include "meta_heuristic_factory_modern.h"
#include "problem/vrpt_problem.h"

namespace daa::modern::examples {

/**
 * @brief Example of a modern algorithm implementation with constructor parameters
 */
class ModernGRASPCVGenerator
    : public ::meta::SolutionGenerator<algorithm::VRPTSolution, VRPTProblem> {
 public:
  /**
   * @brief Constructor with parameters
   *
   * @param alpha Greediness parameter (0.0 = pure greedy, 1.0 = pure random)
   * @param rcl_size Maximum size of restricted candidate list
   * @param seed Random seed (optional)
   */
  ModernGRASPCVGenerator(
    double alpha = 0.3,
    std::size_t rcl_size = 5,
    std::optional<unsigned> seed = std::nullopt
  )
      : alpha_(alpha), rcl_size_(rcl_size), seed_(seed) {

    // Initialize random number generator with seed if provided
    if (seed_) {
      rng_.seed(*seed_);
    } else {
      std::random_device rd;
      rng_.seed(rd());
    }
  }

  /**
   * @brief Generate a solution using GRASP approach
   *
   * @param problem The problem to solve
   * @return algorithm::VRPTSolution The solution
   */
  algorithm::VRPTSolution generateSolution(const VRPTProblem& problem) override {
    // This is just a placeholder implementation
    algorithm::VRPTSolution solution;

    // In a real implementation, this would use alpha_ and rcl_size_ to generate a solution
    // using the GRASP approach
    (void)problem;  // Suppress unused parameter warning

    return solution;
  }

  /**
   * @brief Get the name of this generator
   *
   * @return std::string The name
   */
  std::string name() const override {
    return std::format(
      "Modern GRASP CV Generator (alpha={:.2f}, rcl_size={}, seed={})",
      alpha_,
      rcl_size_,
      seed_ ? std::to_string(*seed_) : "random"
    );
  }

  /**
   * @brief Render UI components for configuring this generator
   */
  void renderConfigurationUI() override {
    // In a real implementation, this would render ImGui controls for adjusting
    // alpha_, rcl_size_, and seed_
  }

 private:
  double alpha_;                  // Greediness parameter
  std::size_t rcl_size_;          // Maximum size of restricted candidate list
  std::optional<unsigned> seed_;  // Random seed
  std::mt19937 rng_;              // Random number generator
};

/**
 * @brief Example of a modern local search implementation with constructor parameters
 */
class ModernTaskExchangeSearch : public ::meta::LocalSearch<algorithm::VRPTSolution, VRPTProblem> {
 public:
  /**
   * @brief Constructor with parameters
   *
   * @param max_iterations Maximum number of iterations
   * @param strategy Search strategy (0 = first improvement, 1 = best improvement)
   */
  ModernTaskExchangeSearch(int max_iterations = 100, int strategy = 0)
      : max_iterations_(max_iterations), strategy_(strategy) {}

  /**
   * @brief Improve a solution using task exchange
   *
   * @param problem The problem
   * @param initial_solution The initial solution
   * @return algorithm::VRPTSolution The improved solution
   */
  algorithm::VRPTSolution improveSolution(
    const VRPTProblem& problem,
    const algorithm::VRPTSolution& initial_solution
  ) override {
    // This is just a placeholder implementation
    algorithm::VRPTSolution solution = initial_solution;

    // In a real implementation, this would use max_iterations_ and strategy_ to improve
    // the solution using task exchange
    (void)problem;  // Suppress unused parameter warning

    return solution;
  }

  /**
   * @brief Get the name of this search
   *
   * @return std::string The name
   */
  std::string name() const override {
    return std::format(
      "Modern Task Exchange Search (max_iterations={}, strategy={})",
      max_iterations_,
      strategy_ == 0 ? "first improvement" : "best improvement"
    );
  }

  /**
   * @brief Render UI components for configuring this search
   */
  void renderConfigurationUI() override {
    // In a real implementation, this would render ImGui controls for adjusting
    // max_iterations_ and strategy_
  }

 private:
  int max_iterations_;  // Maximum number of iterations
  int strategy_;        // Search strategy (0 = first improvement, 1 = best improvement)
};

/**
 * @brief Example of a modern meta-heuristic algorithm with constructor parameters
 */
class ModernGVNS : public TypedAlgorithm<VRPTProblem, algorithm::VRPTSolution> {
 public:
  /**
   * @brief Constructor with parameters
   *
   * @param max_iterations Maximum number of iterations
   * @param generator_name Generator name
   * @param neighborhood_names Neighborhood names
   */
  ModernGVNS(
    int max_iterations = 50,
    std::string generator_name = "ModernGRASPCVGenerator",
    std::vector<std::string> neighborhood_names = {"ModernTaskExchangeSearch"}
  )
      : max_iterations_(max_iterations),
        generator_name_(std::move(generator_name)),
        neighborhood_names_(std::move(neighborhood_names)) {

    // Initialize components
    initializeComponents();
  }

  /**
   * @brief Solve the problem using GVNS
   *
   * @param problem The problem to solve
   * @return algorithm::VRPTSolution The solution
   */
  algorithm::VRPTSolution solve(const VRPTProblem& problem) override {
    // This is just a placeholder implementation
    using MetaFactory = MetaHeuristicFactory<
      algorithm::VRPTSolution,
      VRPTProblem,
      TypedAlgorithm<VRPTProblem, algorithm::VRPTSolution>>;

    // Create generator if not already created
    if (!generator_) {
      auto result = MetaFactory::createGenerator(generator_name_);
      if (result) {
        generator_ = std::move(*result);
      } else {
        throw std::runtime_error("Failed to create generator");
      }
    }

    // Create neighborhoods if not already created
    if (neighborhoods_.empty()) {
      for (const auto& name : neighborhood_names_) {
        auto result = MetaFactory::createSearch(name);
        if (result) {
          neighborhoods_.push_back(std::move(*result));
        } else {
          throw std::runtime_error("Failed to create search");
        }
      }
    }

    // No neighborhoods defined
    if (neighborhoods_.empty()) {
      return generator_->generateSolution(problem);
    }

    // In a real implementation, this would use max_iterations_, generator_, and neighborhoods_
    // to solve the problem using GVNS

    // For now, just generate an initial solution
    return generator_->generateSolution(problem);
  }

  /**
   * @brief Get the name of this algorithm
   *
   * @return std::string The name
   */
  std::string name() const override {
    return std::format("Modern GVNS (max_iterations={})", max_iterations_);
  }

  /**
   * @brief Get the description of this algorithm
   *
   * @return std::string The description
   */
  std::string description() const override {
    return "Modern General Variable Neighborhood Search for VRPT problem";
  }

  /**
   * @brief Render UI components for configuring this algorithm
   */
  void renderConfigurationUI() override {
    // In a real implementation, this would render ImGui controls for adjusting
    // max_iterations_, generator_name_, and neighborhood_names_
  }

 private:
  int max_iterations_;                           // Maximum number of iterations
  std::string generator_name_;                   // Generator name
  std::vector<std::string> neighborhood_names_;  // Neighborhood names

  std::unique_ptr<::meta::SolutionGenerator<algorithm::VRPTSolution, VRPTProblem>> generator_;
  std::vector<std::unique_ptr<::meta::LocalSearch<algorithm::VRPTSolution, VRPTProblem>>>
    neighborhoods_;

  /**
   * @brief Initialize components
   */
  void initializeComponents() {
    // Components will be created lazily in solve()
  }
};

// Register algorithms using macros
// Register the generator
REGISTER_ALGORITHM_MODERN(ModernGRASPCVGenerator, "ModernGRASPCVGenerator", 0.3, 5, std::nullopt);

// Register the search
REGISTER_ALGORITHM_MODERN(ModernTaskExchangeSearch, "ModernTaskExchangeSearch", 100, 0);

// Register the meta-heuristic algorithm
REGISTER_ALGORITHM_MODERN(
  ModernGVNS,
  "ModernGVNS",
  50,
  "ModernGRASPCVGenerator",
  std::vector<std::string>{"ModernTaskExchangeSearch"}
);

// Function to ensure registrations are initialized
inline void registerModernAlgorithms() {
  // The registrations are done at global scope, so this function is just a placeholder
  // to ensure the code is linked properly
}

// Example of how to use the modern registry
inline void modernRegistryExample() {
  // Register the modern algorithms
  registerModernAlgorithms();

  // Create a generator with default parameters
  auto generator =
    AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>("ModernGRASPCVGenerator");

  // Create a generator with custom parameters - IMPROVED API!
  auto customGenerator = AlgorithmRegistry::createTypedWith<VRPTProblem, algorithm::VRPTSolution>(
    "ModernGRASPCVGenerator",
    0.5,                         // alpha
    size_t(10),                  // rcl_size
    std::optional<unsigned>(42)  // seed
  );

  // Create a search with default parameters
  auto search =
    AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>("ModernTaskExchangeSearch"
    );

  // Create a search with custom parameters - IMPROVED API!
  auto customSearch = AlgorithmRegistry::createTypedWith<VRPTProblem, algorithm::VRPTSolution>(
    "ModernTaskExchangeSearch",
    200,  // max_iterations
    1     // strategy
  );

  // Create a meta-heuristic algorithm with default parameters
  auto algorithm =
    AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>("ModernGVNS");

  // Create a meta-heuristic algorithm with custom parameters - IMPROVED API!
  auto customAlgorithm = AlgorithmRegistry::createTypedWith<VRPTProblem, algorithm::VRPTSolution>(
    "ModernGVNS",
    100,                                                  // max_iterations
    std::string("ModernGRASPCVGenerator"),                // generator_name
    std::vector<std::string>{"ModernTaskExchangeSearch"}  // neighborhood_names
  );

  // You can also use the TypedAlgorithmFactory for a cleaner API
  using Factory = TypedAlgorithmFactory<VRPTProblem, algorithm::VRPTSolution>;

  // Create with default parameters
  auto generator2 = Factory::create("ModernGRASPCVGenerator");

  // Create with custom parameters - IMPROVED API!
  auto customGenerator2 = Factory::createWith(
    "ModernGRASPCVGenerator",
    0.5,                         // alpha
    size_t(10),                  // rcl_size
    std::optional<unsigned>(42)  // seed
  );
}

}  // namespace daa::modern::examples
