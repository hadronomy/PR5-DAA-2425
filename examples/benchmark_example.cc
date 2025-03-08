#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include "mitata.h"

int main() {
  mitata::runner runner;

  // Example 1: Simple benchmark with result reporting
  runner.bench("sort_vector", [](std::map<std::string, double>, mitata::BenchmarkResult& result) {
    std::vector<int> data(10000);
    std::random_device rd;
    std::mt19937 g(rd());
    std::generate(data.begin(), data.end(), g);

    auto start_size = data.size();
    std::sort(data.begin(), data.end());

    // Report metrics back to the benchmark
    result.report("sorted_elements", start_size);
    result.report("is_sorted", std::is_sorted(data.begin(), data.end()) ? 1.0 : 0.0);
  });

  // Example 2: Parameterized benchmark with custom scoring
  auto* param_bench = runner.bench(
    "sort_vector_param",
    [](std::map<std::string, double> args, mitata::BenchmarkResult& result) {
      int size = static_cast<int>(args["size"]);
      std::vector<int> data(size);
      std::random_device rd;
      std::mt19937 g(rd());
      std::generate(data.begin(), data.end(), g);

      auto start_time = std::chrono::high_resolution_clock::now();
      std::sort(data.begin(), data.end());
      auto end_time = std::chrono::high_resolution_clock::now();

      auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

      // Report metrics back to the benchmark
      result.report(
        "elements_per_microsecond",
        size / static_cast<double>(std::max(1, static_cast<int>(duration)))
      );
      result.report("vector_size", size);
    }
  );

  // Add parameters to test different sizes
  param_bench->dense_range("size", 10000, 50000, 10000);

  // Custom scoring function using the reported elements_per_microsecond (higher is better)
  param_bench->score(mitata::scores::result_value("elements_per_microsecond", true));

  // Example 3: Different algorithms comparison with custom scoring
  runner.group([&]() {
    // Algorithm 1: Standard sort
    auto* std_sort = runner.bench(
      "std::sort",
      [](std::map<std::string, double> args, mitata::BenchmarkResult& result) {
        int size = static_cast<int>(args["size"]);
        std::vector<int> data(size);
        std::random_device rd;
        std::mt19937 g(rd());
        std::generate(data.begin(), data.end(), g);

        std::sort(data.begin(), data.end());

        result.report("algorithm", 1);
        result.report("vector_size", size);
      }
    );

    // Algorithm 2: Bubble sort (deliberately inefficient)
    auto* bubble_sort = runner.bench(
      "bubble_sort",
      [](std::map<std::string, double> args, mitata::BenchmarkResult& result) {
        int size = static_cast<int>(args["size"]);
        std::vector<int> data(size);
        std::random_device rd;
        std::mt19937 g(rd());
        std::generate(data.begin(), data.end(), g);

        // Bubble sort implementation
        for (int i = 0; i < size - 1; i++) {
          for (int j = 0; j < size - i - 1; j++) {
            if (data[j] > data[j + 1]) {
              std::swap(data[j], data[j + 1]);
            }
          }
        }

        result.report("algorithm", 2);
        result.report("vector_size", size);
      }
    );

    // Set parameters for both benchmarks
    std_sort->args("size", {1000, 2000});
    bubble_sort->args("size", {1000, 2000});

    // Set std::sort as baseline for comparison
    std_sort->baseline(true);

    // Advanced custom scoring function that takes into account both timing and parameters
    auto custom_scoring = [](
                            const mitata::lib::k_stats& stats,
                            const std::map<std::string, double>& args,
                            const mitata::BenchmarkResult& result
                          ) -> double {
      // For sorting, we want to consider:
      // 1. Raw speed (lower time is better)
      // 2. How it scales with input size (lower time per element is better)
      double base_score = stats.avg > 0 ? 1000000.0 / stats.avg : 0.0;

      // Adjust score based on input size (harder to sort larger inputs)
      double size_factor = std::log10(args.at("size")) / 3.0;  // Normalize

      // Algorithm-specific adjustment
      double algo_factor = 1.0;
      if (result.has("algorithm")) {
        algo_factor = result.get("algorithm") == 1 ? 1.0 : 0.8;  // Penalize bubble sort
      }

      // Return weighted score (higher is better)
      return base_score * size_factor * algo_factor;
    };

    std_sort->score(mitata::scores::custom(custom_scoring));
    bubble_sort->score(mitata::scores::custom(custom_scoring));
  });

  // Run all benchmarks and show summary
  runner.summary([&]() {
    auto results = runner.run();

    // Access specific benchmark results after running
    auto sort_result = runner.get_result("sort_vector");
    if (sort_result.has("sorted_elements")) {
      std::cout << "\nCustom metrics from benchmarks:" << std::endl;
      std::cout << "Sorted elements: " << sort_result.get("sorted_elements") << std::endl;
      std::cout << "Is sorted: " << (sort_result.get("is_sorted") == 1.0 ? "Yes" : "No")
                << std::endl;
    }

    // Access all benchmark results
    std::cout << "\nAll benchmark result metrics:" << std::endl;
    for (const auto& [bench_name, bench_result] : runner.get_all_results()) {
      std::cout << "Benchmark: " << bench_name << std::endl;
      for (const auto& [key, value] : bench_result.values) {
        std::cout << "  " << key << ": " << value << std::endl;
      }
    }
  });

  return 0;
}
