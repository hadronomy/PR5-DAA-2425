#include <iostream>
#include <string>
#include <vector>

#include "benchmarking.h"
#include "mitata.h"
#include "tsp.h"
#include "ui.h"

// Example TSP algorithm implementations would go here in practice
// For this example, we'll use existing algorithms from your registry

int main(int argc, char** argv) {
  // Get available TSP algorithms from registry
  std::vector<std::string> available_algorithms;
  for (const auto& name : AlgorithmRegistry::availableAlgorithms()) {
    try {
      auto algorithm = AlgorithmRegistry::createTyped<Graph<City, double>, Path>(name);
      available_algorithms.push_back(name);
    } catch (...) {
      // Not a TSP algorithm, skip it
    }
  }

  if (available_algorithms.empty()) {
    UI::error("No TSP algorithms found in registry. Please register some algorithms first.");
    return 1;
  }

  UI::info("Available TSP algorithms:");
  for (const auto& algo : available_algorithms) {
    UI::info("  - " + algo);
  }

  // Example files - replace with actual file paths
  std::vector<std::string> test_files = {"examples/4.txt"};

  // Example 1: Run a single algorithm benchmark with path length reporting
  UI::header("Example 1: Single Algorithm Benchmark with Path Length Reporting");

  if (!available_algorithms.empty()) {
    run_benchmark_with_files(
      available_algorithms[0],  // Use first available algorithm
      1,                        // iterations
      test_files,
      true,  // debug info
      5000   // timeout in ms
    );
  }

  // Example 2: Compare algorithms using path length as the scoring metric
  UI::header("Example 2: Compare Algorithms Using Path Length Scoring");

  compare_algorithms_with_files(
    available_algorithms, 1, test_files, true, 5000, mitata::scores::path_length()
  );

  // Example 3: Compare algorithms using balanced scoring (path length and time)
  UI::header("Example 3: Compare Algorithms Using Balanced Scoring");

  compare_algorithms_with_files(
    available_algorithms,
    1,
    test_files,
    true,
    5000,
    mitata::scores::balanced_tsp_score(0.8, 0.2)  // 80% weight on path length, 20% on time
  );

  // Example 4: Compare algorithms using efficiency scoring (path quality per time unit)
  UI::header("Example 4: Compare Algorithms Using Efficiency Scoring");

  compare_algorithms_with_files(
    available_algorithms, 1, test_files, true, 5000, mitata::scores::tsp_efficiency()
  );

  return 0;
}
