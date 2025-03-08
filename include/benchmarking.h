#pragma once

#include <string>
#include <vector>

#include "algorithm_registry.h"
#include "mitata.h"
#include "parser/tsp_driver.h"
#include "tsp.h"
#include "ui.h"

// Forward declarations of debug display functions
template <typename InputType, typename OutputType>
void display_debug_info(
  const std::string& algo_name,
  const InputType& input,
  const OutputType& output
);

// Forward-declare the specialization before it's used
template <>
void display_debug_info<Graph<City, double>, Path>(
  const std::string& algo_name,
  const Graph<City, double>& graph,
  const Path& path
);

// Add TSP-specific scoring functions
namespace mitata {
namespace scores {

// Score based solely on path length - lower is better
inline std::function<
  double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
  path_cost() {
  return
    [](const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult& result) {
      // Return negative path length since lower is better but mitata expects higher scores to be
      // better
      return -result.get("path_cost", std::numeric_limits<double>::infinity());
    };
}

// Score based on a balance of path length and execution time
// Lower path length and lower time are both better
inline std::function<
  double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
  balanced_tsp_score(double path_weight = 0.7, double time_weight = 0.3) {
  return
    [path_weight, time_weight](
      const lib::k_stats& stats, const std::map<std::string, double>&, const BenchmarkResult& result
    ) {
      double path_cost = result.get("path_cost", std::numeric_limits<double>::infinity());
      double time_ms = stats.avg / 1e6;

      // Normalize path length - we need domain knowledge for this
      // For demonstration, assume typical path lengths are between 1-10000
      double normalized_path_score = 1e3 / std::max(1.0, path_cost);

      // Normalize time - milliseconds (assume 1ms-30000ms is reasonable range)
      double normalized_time_score = 1e3 / std::max(1.0, time_ms);

      // Calculate weighted score (higher is better for mitata)
      return path_weight * normalized_path_score + time_weight * normalized_time_score;
    };
}

// Quality per time unit (efficiency) scoring function
inline std::function<
  double(const lib::k_stats&, const std::map<std::string, double>&, const BenchmarkResult&)>
  tsp_efficiency() {
  return
    [](
      const lib::k_stats& stats, const std::map<std::string, double>&, const BenchmarkResult& result
    ) {
      double path_cost = result.get("path_cost", std::numeric_limits<double>::infinity());
      double time_ms = stats.avg / 1e6;

      // Check for timeouts or invalid paths
      if (path_cost == std::numeric_limits<double>::infinity() || time_ms <= 0) {
        return std::numeric_limits<double>::lowest();
      }

      // Return quality per time unit - lower path length per ms is better
      // We invert to make higher better for the scoring system
      return time_ms / (path_cost * time_ms);
    };
}

}  // namespace scores
}  // namespace mitata

/**
 * @brief Convert a graph from string,int to City,double format for TSP algorithms
 *
 * @param input The input graph with string vertices and int edges
 * @return Graph<City, double> A graph suitable for TSP algorithms
 */
inline Graph<City, double> convertTspGraph(const Graph<std::string, int>& input) {
  Graph<City, double> output(GraphType::Undirected);  // Create an undirected graph for TSP

  // Map to keep track of string IDs to new vertex IDs
  std::unordered_map<std::size_t, std::size_t> id_map;

  // Add all vertices
  const auto vertices = input.getVertexIds();
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    const auto vid = vertices[i];
    const auto* vertex = input.getVertex(vid);
    if (vertex) {
      // Create a city from the vertex name with coordinates (used as placeholder)
      const std::string& name = vertex->data();
      double x = static_cast<double>(i);  // Use index as x coordinate
      double y = 0.0;                     // Default y coordinate

      // Extract coordinates if the name contains them (format: "name x y")
      std::istringstream ss(name);
      std::string city_name;
      double city_x, city_y;
      if (ss >> city_name >> city_x >> city_y) {
        // If name contains coordinates, use them
        id_map[vid] = output.addVertex(City(city_name, city_x, city_y));
      } else {
        // Otherwise use default coordinates
        id_map[vid] = output.addVertex(City(name, x, y));
      }
    }
  }

  // Add all edges
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    const auto source_id = vertices[i];
    if (id_map.find(source_id) == id_map.end())
      continue;

    for (std::size_t j = 0; j < vertices.size(); ++j) {
      const auto target_id = vertices[j];
      if (i == j || id_map.find(target_id) == id_map.end())
        continue;

      if (input.hasEdge(source_id, target_id)) {
        int weight = input.getEdge(source_id, target_id)->data();
        output.addEdge(id_map[source_id], id_map[target_id], static_cast<double>(weight));
      }
    }
  }

  return output;
}

/**
 * @brief Helper template function to run a benchmark for any algorithm type
 *
 * @tparam InputType The input type of the algorithm
 * @tparam OutputType The output type of the algorithm
 * @param algo_name Name of the algorithm
 * @param iterations Number of iterations
 * @param test_size Size of test data
 * @param debug Whether to display debug information
 * @param time_limit Time limit for the algorithm
 */
template <typename InputType, typename OutputType>
void run_typed_benchmark(
  const std::string& algo_name,
  int iterations,
  int test_size,
  bool debug,
  int time_limit
) {
  auto runner = mitata::runner();

  // Create test data using the algorithm's data generator
  InputType data = AlgorithmRegistry::generateData<InputType>(algo_name, test_size);

  // Store the result if debugging is enabled
  OutputType result;
  bool has_result = false;
  bool timed_out = false;

  runner.bench(algo_name, [&](std::map<std::string, double> metrics) {
    auto algorithm = AlgorithmRegistry::createTyped<InputType, OutputType>(algo_name);

    // Use the provided time limit
    try {
      // Store the result only on the first iteration if debug is enabled
      if (debug && !has_result) {
        result = algorithm->solveWithTimeLimit(data, time_limit);
        has_result = true;
      } else {
        algorithm->solveWithTimeLimit(data, time_limit);
      }
    } catch (const std::exception& e) {
      std::string error = e.what();
      if (error.find("time limit") != std::string::npos) {
        timed_out = true;
        UI::warning(
          "Algorithm " + algo_name + " timed out after " + std::to_string(time_limit) + " ms"
        );
        throw std::runtime_error("Benchmark timeout");
      } else {
        throw;
      }
    }
  });

  if (!timed_out) {
    // Run the benchmark only if not timed out
    mitata::k_run opts;
    opts.colors = true;
    runner.run(opts);
  } else {
    UI::warning("Benchmark for " + algo_name + " was terminated due to timeout.");
  }

  // Show debug information if requested and not timed out
  if (debug && !timed_out) {
    display_debug_info(algo_name, data, result);
  }
}

/**
 * @brief Run a benchmark for a single algorithm
 *
 * @param algo_name Name of the algorithm to benchmark
 * @param iterations Number of iterations
 * @param test_size Size of test data
 * @param debug Whether to print debug information
 * @param time_limit Time limit for the algorithm
 */
inline void run_benchmark(
  const std::string& algo_name,
  int iterations,
  int test_size,
  bool debug = false,
  int time_limit = Algorithm::DEFAULT_TIME_LIMIT_MS
) {
  // Benchmarks are automatically registered when algorithms are registered
  if (!AlgorithmRegistry::runBenchmark(algo_name, iterations, test_size, debug, time_limit)) {
    UI::error(fmt::format("Unknown algorithm type: {}", algo_name));
  }
}

inline void run_benchmark(
  const std::string& algo_name,
  int iterations,
  const std::vector<int>& test_sizes,
  bool debug = false,
  int time_limit = Algorithm::DEFAULT_TIME_LIMIT_MS
) {
  // If no test sizes provided, use default size 10
  if (test_sizes.empty()) {
    UI::warning("No test sizes provided, using default size 10");
    return run_benchmark(algo_name, iterations, 10, debug, time_limit);
  }

  // Run benchmark for each size
  for (int test_size : test_sizes) {
    // Benchmarks are automatically registered when algorithms are registered
    if (!AlgorithmRegistry::runBenchmark(algo_name, iterations, test_size, debug, time_limit)) {
      UI::error(fmt::format("Unknown algorithm type: {}", algo_name));
      break;
    }
  }
}

/**
 * @brief Run a benchmark for a single algorithm using input files
 *
 * @param algo_name Name of the algorithm to benchmark
 * @param iterations Number of iterations
 * @param files List of input files to use instead of generated data
 * @param debug Whether to print debug information
 * @param time_limit Time limit for the algorithm
 * @param scoring_function Optional custom scoring function for comparison
 */
inline void run_benchmark_with_files(
  const std::string& algo_name,
  int iterations,
  const std::vector<std::string>& files,
  bool debug = false,
  int time_limit = Algorithm::DEFAULT_TIME_LIMIT_MS,
  std::function<
    double(const mitata::lib::k_stats&, const std::map<std::string, double>&, const mitata::BenchmarkResult&)>
    scoring_function = nullptr
) {
  if (files.empty()) {
    UI::warning("No input files provided");
    return;
  }

  // Run benchmark for each file
  for (const auto& filename : files) {
    UI::info(fmt::format("Benchmarking with file: {}", filename));

    // Parse the file
    TSPDriver driver;
    if (!driver.parse_file(filename)) {
      UI::error(fmt::format("Failed to parse file: {}", filename));
      continue;
    }

    // Convert the graph to the format expected by TSP algorithms
    auto tsp_graph = convertTspGraph(driver.graph);
    auto runner = mitata::runner();

    // Store the result if debugging is enabled
    Path result;
    bool has_result = false;
    bool timed_out = false;

    auto* bench =
      runner.bench(algo_name, [&](std::map<std::string, double>, mitata::BenchmarkResult& result) {
        auto algorithm = AlgorithmRegistry::createTyped<Graph<City, double>, Path>(algo_name);

        try {
          Path path = algorithm->solveWithTimeLimit(tsp_graph, time_limit);
          // Store path for debug display if needed
          // if (debug && !has_result) {
          //   result.report("path", path);
          //   has_result = true;
          // }

          result.report("path_cost", tsp_graph.getPathCost(path));
          result.report("cities_visited", path.size());
          result.report("path_is_complete", path.size() == tsp_graph.vertexCount() ? 1.0 : 0.0);

        } catch (const std::exception& e) {
          std::string error = e.what();
          if (error.find("time limit") != std::string::npos) {
            timed_out = true;
            UI::warning(
              "Algorithm " + algo_name + " timed out after " + std::to_string(time_limit) + " ms"
            );

            throw std::runtime_error("Benchmark timeout");
          } else {
            throw;
          }
        }
      });

    // Apply scoring function if provided
    if (scoring_function) {
      bench->score(scoring_function);
    }

    if (!timed_out) {
      // Run the benchmark only if not timed out
      mitata::k_run opts;
      opts.colors = true;
      auto stats = runner.run(opts);

      // Display collected metrics
      UI::info("Benchmark results for " + algo_name + " on " + filename + ":");
      auto bench_result = runner.get_result(algo_name);
      if (bench_result.has("path_cost")) {
        UI::info(fmt::format("  - Path length: {:.2f}", bench_result.get("path_cost")));
      }
      if (bench_result.has("execution_time_ms")) {
        UI::info(fmt::format("  - Execution time: {:.2f} ms", bench_result.get("execution_time_ms"))
        );
      }
      if (bench_result.has("cities_visited")) {
        UI::info(fmt::format("  - Cities visited: {}", bench_result.get("cities_visited")));
      }
      if (bench_result.has("path_is_complete")) {
        UI::info(fmt::format(
          "  - Complete path: {}", bench_result.get("path_is_complete") > 0.5 ? "Yes" : "No"
        ));
      }
    } else {
      UI::warning("Benchmark for " + algo_name + " was terminated due to timeout.");
    }
  }
}

/**
 * @brief Compare multiple algorithms
 *
 * @param algo_names Names of algorithms to compare (use "all" for all available algorithms)
 * @param iterations Number of iterations
 * @param test_sizes Sizes of test data
 * @param debug Whether to print debug information
 * @param time_limit Time limit for the algorithm
 */
inline void compare_algorithms(
  const std::vector<std::string>& algo_names,
  int iterations,
  const std::vector<int>& test_sizes = {10},
  bool debug = false,
  int time_limit = Algorithm::DEFAULT_TIME_LIMIT_MS,
  std::function<
    double(const mitata::lib::k_stats&, const std::map<std::string, double>&, const mitata::BenchmarkResult&)>
    scoring_function = mitata::scores::balanced_tsp_score(0.8, 0.2)
) {
  std::vector<std::string> tsp_algos;
  std::vector<double> test_sizes_d(test_sizes.begin(), test_sizes.end());

  // Validate algorithms first
  for (const auto& name : algo_names) {
    if (!AlgorithmRegistry::exists(name)) {
      UI::error(fmt::format("Algorithm '{}' not found", name));
      return;
    }

    // Check if it's a TSP algorithm by trying to cast
    try {
      auto algorithm = AlgorithmRegistry::createTyped<Graph<City, double>, Path>(name);
      tsp_algos.push_back(name);
    } catch (const std::exception& e) {
      UI::error(fmt::format("Algorithm '{}' is not a TSP algorithm", name));
    }
  }

  if (tsp_algos.empty()) {
    UI::error("No valid TSP algorithms to compare");
    return;
  }

  // Create a mitata runner for benchmarking
  auto runner = mitata::runner();

  std::unordered_map<double, Graph<City, double>> test_graphs;
  for (double size : test_sizes) {
    test_graphs[size] = AlgorithmRegistry::generateData<Graph<City, double>>(tsp_algos[0], size);
  }

  // Store results for debug display
  std::unordered_map<std::string, Path> results;
  std::unordered_set<std::string> timed_out_algos;

  runner.lineplot([&] {
    runner.summary([&] {
      // Add each algorithm to the benchmark
      for (const auto& algo_name : tsp_algos) {
        std::string bench_name = algo_name + "($N)";
        auto* bench = runner.bench(
          bench_name,
          [&](std::map<std::string, double> metrics, mitata::BenchmarkResult& result) {
            auto algorithm = AlgorithmRegistry::createTyped<Graph<City, double>, Path>(algo_name);
            Graph<City, double> graph = test_graphs[metrics.at("N")];

            // Use the provided time limit
            try {
              Path path = algorithm->solveWithTimeLimit(graph, time_limit);

              result.report("path_cost", graph.getPathCost(path));
              result.report("cities_visited", path.size());
              result.report("path_is_complete", path.size() == graph.vertexCount() ? 1.0 : 0.0);
            } catch (const std::exception& e) {
              std::string error = e.what();
              if (error.find("time limit") != std::string::npos) {
                timed_out_algos.insert(algo_name);
                UI::warning(
                  "Algorithm " + algo_name + " timed out after " + std::to_string(time_limit) +
                  " ms"
                );
                throw std::runtime_error("Benchmark timeout");
              } else {
                throw;
              }
            }
          }
        );
        bench->args("N", test_sizes_d);

        if (scoring_function) {
          bench->score(scoring_function);
        }
      }
    });
  });

  // Run the benchmarks
  mitata::k_run opts;
  opts.timelimit_ns = time_limit * 1e6;
  opts.colors = true;
  auto all_stats = runner.run(opts);

  if (debug) {
    std::cout << "\n=== Algorithm Comparison Results ===\n";
    std::cout << std::left << std::setw(25) << "Algorithm" << std::setw(15) << "Path Length"
              << std::setw(15) << "Time (ms)" << std::endl;
    std::cout << std::string(75, '-') << std::endl;

    for (const auto& algo : tsp_algos) {
      for (const auto& [bench_name, stats] : all_stats) {
        auto bench_result = runner.get_result(bench_name);

        std::cout << std::left << std::setw(25) << algo;

        if (bench_result.has("path_cost")) {
          double path_cost = bench_result.get("path_cost");
          if (path_cost == std::numeric_limits<double>::infinity()) {
            std::cout << std::setw(15) << "TIMEOUT";
          } else {
            std::cout << std::setw(15) << std::fixed << std::setprecision(2) << path_cost;
          }
        } else {
          std::cout << std::setw(15) << "N/A";
        }

        if (stats.avg >= 0) {
          std::cout << std::setw(15) << std::fixed << std::setprecision(2) << stats.avg / 1e6;
        } else {
          std::cout << std::setw(15) << "N/A";
        }

        std::cout << std::endl;
      }
    }
  }
}

/**
 * @brief Compare multiple algorithms using input files
 *
 * @param algo_names Names of algorithms to compare
 * @param iterations Number of iterations
 * @param files List of input files to use instead of generated data
 * @param debug Whether to print debug information
 * @param time_limit Time limit for the algorithm
 * @param scoring_function Optional custom scoring function for comparison
 */
inline void compare_algorithms_with_files(
  const std::vector<std::string>& algo_names,
  int iterations,
  const std::vector<std::string>& files,
  bool debug = false,
  int time_limit = Algorithm::DEFAULT_TIME_LIMIT_MS,
  std::function<
    double(const mitata::lib::k_stats&, const std::map<std::string, double>&, const mitata::BenchmarkResult&)>
    scoring_function = mitata::scores::balanced_tsp_score(0.8, 0.2)
) {
  if (files.empty()) {
    UI::warning("No input files provided");
    return;
  }

  std::vector<std::string> tsp_algos;
  std::vector<Graph<City, double>> test_graphs;
  std::vector<std::string> file_labels;

  // Validate algorithms first
  for (const auto& name : algo_names) {
    if (!AlgorithmRegistry::exists(name)) {
      UI::error(fmt::format("Algorithm '{}' not found", name));
      return;
    }

    // Check if it's a TSP algorithm by trying to cast
    try {
      auto algorithm = AlgorithmRegistry::createTyped<Graph<City, double>, Path>(name);
      tsp_algos.push_back(name);
    } catch (const std::exception& e) {
      UI::error(fmt::format("Algorithm '{}' is not a TSP algorithm", name));
    }
  }

  if (tsp_algos.empty()) {
    UI::error("No valid TSP algorithms to compare");
    return;
  }

  // Parse each file and store the graph
  for (const auto& filename : files) {
    TSPDriver driver;
    if (!driver.parse_file(filename)) {
      UI::error(fmt::format("Failed to parse file: {}", filename));
      continue;
    }

    // Convert the graph to the format expected by TSP algorithms
    auto tsp_graph = convertTspGraph(driver.graph);
    test_graphs.push_back(tsp_graph);
    file_labels.push_back(filename);
  }

  if (test_graphs.empty()) {
    UI::error("No valid files to use for comparison");
    return;
  }

  // Create a mitata runner for benchmarking
  auto runner = mitata::runner();

  // Store results for debug display
  std::unordered_map<std::string, Path> results;
  std::unordered_set<std::string> timed_out_algos;

  runner.lineplot([&] {
    runner.summary([&] {
      // Add each algorithm to the benchmark
      for (const auto& algo_name : tsp_algos) {
        std::string bench_name = algo_name + " (file: $FILE)";
        auto* bench = runner.bench(
          bench_name,
          [&, algo_name](std::map<std::string, double> metrics, mitata::BenchmarkResult& result) {
            size_t file_index = static_cast<size_t>(metrics.at("FILE"));
            if (file_index >= test_graphs.size()) {
              throw std::runtime_error("Invalid file index");
            }

            Graph<City, double>& graph = test_graphs[file_index];
            std::string result_key = algo_name + ":" + file_labels[file_index];

            try {
              auto algorithm = AlgorithmRegistry::createTyped<Graph<City, double>, Path>(algo_name);
              Path path = algorithm->solveWithTimeLimit(graph, time_limit);

              double path_cost = graph.getPathCost(path);

              // Store the path for debug display
              if (debug && results.find(result_key) == results.end()) {
                results[result_key] = path;
              }

              // Report metrics
              result.report("path_cost", path_cost);
              result.report("cities_visited", path.size());
              result.report("path_is_complete", path.size() == graph.vertexCount() ? 1.0 : 0.0);
              result.report("timed_out", 0.0);

            } catch (const std::exception& e) {
              std::string error = e.what();
              if (error.find("time limit") != std::string::npos) {
                timed_out_algos.insert(algo_name);
                UI::warning(
                  "Algorithm " + algo_name + " timed out after " + std::to_string(time_limit) +
                  " ms"
                );
                // Mark timeout in results
                result.report("timed_out", 1.0);
                result.report("path_cost", std::numeric_limits<double>::infinity());
                result.report("cities_visited", 0);
                result.report("path_is_complete", 0.0);

                throw std::runtime_error("Benchmark timeout");  // Stop this algorithm's benchmark
              } else {
                throw;  // Re-throw other errors
              }
            }
          }
        );

        // Set file indices as parameters
        bench->args("FILE", std::vector<double>(test_graphs.size()));

        // Apply scoring function if provided
        if (scoring_function) {
          bench->score(scoring_function);
        }
      }
    });
  });

  // Run the benchmarks
  mitata::k_run opts;
  opts.timelimit_ns = time_limit * 1e6;
  opts.colors = true;
  auto all_stats = runner.run(opts);

  if (debug) {
    // Print a summary of the collected metrics
    std::cout << "\n=== Algorithm Comparison Results ===\n";
    std::cout << std::left << std::setw(25) << "Algorithm" << std::setw(20) << "File"
              << std::setw(15) << "Path Length" << std::setw(15) << "Time (ms)" << std::endl;
    std::cout << std::string(75, '-') << std::endl;

    for (const auto& algo : tsp_algos) {
      for (size_t i = 0; i < file_labels.size(); i++) {
        std::string bench_name = algo + " (file: " + std::to_string(i) + ")";
        auto bench_result = runner.get_result(bench_name);
        auto stats = all_stats[bench_name];

        std::cout << std::left << std::setw(25) << algo << std::setw(20) << file_labels[i];

        if (bench_result.has("path_cost")) {
          double path_cost = bench_result.get("path_cost");
          if (path_cost == std::numeric_limits<double>::infinity()) {
            std::cout << std::setw(15) << "TIMEOUT";
          } else {
            std::cout << std::setw(15) << std::fixed << std::setprecision(2) << path_cost;
          }
        } else {
          std::cout << std::setw(15) << "N/A";
        }

        if (stats.avg >= 0) {
          std::cout << std::setw(15) << std::fixed << std::setprecision(2) << stats.avg / 1e6;
        } else {
          std::cout << std::setw(15) << "N/A";
        }

        std::cout << std::endl;
      }
    }

    // Display debug information if requested
    for (auto it = results.begin(); it != results.end(); ++it) {
      const auto& key = it->first;
      const auto& path = it->second;

      // Extract algo_name and file from key
      size_t separator = key.find(":");
      if (separator != std::string::npos) {
        std::string algo_name = key.substr(0, separator);
        std::string file = key.substr(separator + 1);

        // Find corresponding graph
        for (size_t i = 0; i < file_labels.size(); i++) {
          if (file_labels[i] == file) {
            if (timed_out_algos.find(algo_name) == timed_out_algos.end()) {
              display_debug_info(algo_name, test_graphs[i], path);
            }
            break;
          }
        }
      }
    }
  }
}

// Debug display functions for different algorithm types
template <typename InputType, typename OutputType>
void display_debug_info(
  const std::string& algo_name,
  const InputType& input,
  const OutputType& output
) {
  // Default implementation - show basic info
  UI::info(fmt::format("Algorithm: {}", algo_name));
}

// Only keep the TSP specialization and remove others
template <>
inline void display_debug_info<Graph<City, double>, Path>(
  const std::string& algo_name,
  const Graph<City, double>& graph,
  const Path& path
) {
  std::string title = algo_name;
  UI::header(fmt::format("=== {} ===", title));

  // Create a table to display the TSP solution
  tabulate::Table table;
  table.add_row({"Algorithm", "Graph Vertices", "Path Length", "Path Valid"});

  double path_cost = graph.getPathCost(path);
  bool path_valid = !path.empty() && path.size() == graph.vertexCount();

  table.add_row(
    {algo_name,
     std::to_string(graph.vertexCount()),
     std::to_string(path_cost),
     path_valid ? "Yes" : "No"}
  );

  // Format the table
  table[0].format().font_style({tabulate::FontStyle::bold}).font_align(tabulate::FontAlign::center);
  table[1].format().font_align(tabulate::FontAlign::center);

  // Print the table
  std::cout << table << std::endl;

  // Print path details
  std::cout << "Path details:" << std::endl;
  TSPSolver::printPath(path, graph);
}
