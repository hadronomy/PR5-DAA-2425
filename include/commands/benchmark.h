#pragma once

#include <fmt/core.h>

#include "algorithm_registry.h"
#include "benchmarking.h"
#include "command_handler.h"
#include "command_registry.h"
#include "ui.h"

/**
 * Command handler for benchmark command
 * Benchmarks a single algorithm with specified options
 */
class BenchmarkCommand : public CommandHandler {
 public:
  BenchmarkCommand(
    std::string algo_name,
    int iterations,
    std::vector<int> test_sizes,
    std::vector<std::string> input_files,
    bool verbose,
    bool debug,
    int time_limit_ms = Algorithm::DEFAULT_TIME_LIMIT_MS
  )
      : CommandHandler(verbose),
        algo_name_(std::move(algo_name)),  // Ensure we properly capture the algorithm name
        iterations_(iterations),
        test_sizes_(std::move(test_sizes)),
        input_files_(std::move(input_files)),
        debug_(debug),
        time_limit_ms_(time_limit_ms) {
    // Debug output to verify algo_name_
    if (verbose_) {
      std::cout << "Debug - Algorithm name: '" << algo_name_ << "'" << std::endl;
    }
  }

  bool execute() override {
    try {
      if (!AlgorithmRegistry::exists(algo_name_)) {
        UI::error(fmt::format("Algorithm '{}' not found", algo_name_));
        return false;
      }

      if (verbose_) {
        UI::info(fmt::format("Benchmarking algorithm: {}", algo_name_));

        if (!input_files_.empty()) {
          UI::info(fmt::format(
            "Configuration: iterations={}, input_files={}, time_limit={}ms",
            iterations_,
            fmt::join(input_files_, ","),
            time_limit_ms_
          ));
        } else {
          UI::info(fmt::format(
            "Configuration: iterations={}, sizes={}, time_limit={}ms",
            iterations_,
            fmt::join(test_sizes_, ","),
            time_limit_ms_
          ));
        }
      }

      // Set the global time limit for all algorithms - now works with non-const static variable
      Algorithm::DEFAULT_TIME_LIMIT_MS = time_limit_ms_;

      // Use either files or generated data
      if (!input_files_.empty()) {
        run_benchmark_with_files(algo_name_, iterations_, input_files_, debug_, time_limit_ms_);
      } else {
        run_benchmark(algo_name_, iterations_, test_sizes_, debug_, time_limit_ms_);
      }

      if (verbose_) {
        UI::success("Benchmark completed successfully");
      }
      return true;
    } catch (const std::exception& e) {
      UI::error(fmt::format("Benchmark failed: {}", e.what()));
      return false;
    }
  }

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);

 private:
  std::string algo_name_;
  int iterations_;
  std::vector<int> test_sizes_;
  std::vector<std::string> input_files_;  // Added input files
  bool debug_;
  int time_limit_ms_;  // Time limit in milliseconds
};