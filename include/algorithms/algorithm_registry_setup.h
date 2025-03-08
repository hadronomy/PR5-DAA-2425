#pragma once

#include "algorithm_registry.h"
#include "benchmarking.h"
#include "data_generator.h"
#include "graph.h"  // Include graph.h first
#include "tsp.h"    // Then include tsp.h

// Forward declaration of benchmark function for TSP algorithms
template <typename InputType, typename OutputType>
void run_typed_benchmark(
  const std::string& algo_name,
  int iterations,
  int test_size,
  bool debug,
  int time_limit
);

/**
 * @brief Set up algorithm registry with TSP algorithms and benchmark functions
 */
inline void setup_algorithm_registry() {
  auto& registry = AlgorithmRegistry::instance();

  // Algorithms are already registered at compile time
  // Just set up benchmark functions

  // Register benchmark functions for TSP algorithms
  registry.register_benchmark_function<Graph<City, double>, Path>(
    "nearest_neighbor",
    [](const std::string& name, int iterations, int test_size, bool debug, int time_limit) {
      run_typed_benchmark<Graph<City, double>, Path>(
        name, iterations, test_size, debug, time_limit
      );
    }
  );

  registry.register_benchmark_function<Graph<City, double>, Path>(
    "brute_force",
    [](const std::string& name, int iterations, int test_size, bool debug, int time_limit) {
      run_typed_benchmark<Graph<City, double>, Path>(
        name, iterations, test_size, debug, time_limit
      );
    }
  );

  registry.register_benchmark_function<Graph<City, double>, Path>(
    "dynamic_programming",
    [](const std::string& name, int iterations, int test_size, bool debug, int time_limit) {
      run_typed_benchmark<Graph<City, double>, Path>(
        name, iterations, test_size, debug, Algorithm::DEFAULT_TIME_LIMIT_MS
      );
    }
  );

  registry.register_benchmark_function<Graph<City, double>, Path>(
    "two_opt",
    [](const std::string& name, int iterations, int test_size, bool debug, int time_limit) {
      run_typed_benchmark<Graph<City, double>, Path>(
        name, iterations, test_size, debug, Algorithm::DEFAULT_TIME_LIMIT_MS
      );
    }
  );

  registry.register_benchmark_function<Graph<City, double>, Path>(
    "simulated_annealing",
    [](const std::string& name, int iterations, int test_size, bool debug, int time_limit) {
      run_typed_benchmark<Graph<City, double>, Path>(
        name, iterations, test_size, debug, Algorithm::DEFAULT_TIME_LIMIT_MS
      );
    }
  );

  // Register data generators for TSP
  registry.registerDataGenerator<Graph<City, double>>(
    "nearest_neighbor", std::make_unique<RandomTSPGenerator>(GraphType::Undirected)
  );

  registry.registerDataGenerator<Graph<City, double>>(
    "brute_force", std::make_unique<RandomTSPGenerator>(GraphType::Undirected)
  );

  registry.registerDataGenerator<Graph<City, double>>(
    "dynamic_programming", std::make_unique<RandomTSPGenerator>(GraphType::Undirected)
  );

  registry.registerDataGenerator<Graph<City, double>>(
    "two_opt", std::make_unique<RandomTSPGenerator>(GraphType::Undirected)
  );

  registry.registerDataGenerator<Graph<City, double>>(
    "simulated_annealing", std::make_unique<RandomTSPGenerator>(GraphType::Undirected)
  );
}
