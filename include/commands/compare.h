#pragma once

#include <string>
#include <vector>

#include <fmt/core.h>

#include "algorithm_registry.h"
#include "benchmarking.h"
#include "command_handler.h"
#include "command_registry.h"
#include "ui.h"

/**
 * Command handler for compare command
 * Compares performance of multiple algorithms
 */
class CompareCommand : public CommandHandler {
 public:
  CompareCommand(
    std::vector<std::string> algo_names,
    int iterations,
    std::vector<int> test_sizes,
    std::vector<std::string> input_files,
    bool verbose,
    bool debug,
    int time_limit_ms = Algorithm::DEFAULT_TIME_LIMIT_MS
  )
      : CommandHandler(verbose),
        algo_names_(std::move(algo_names)),
        iterations_(iterations),
        test_sizes_(std::move(test_sizes)),
        input_files_(std::move(input_files)),
        debug_(debug),
        time_limit_ms_(time_limit_ms) {}

  bool execute() override {
    try {
      if (algo_names_.empty()) {
        UI::error("No algorithms specified for comparison");
        return false;
      }

      // Check for "all" keyword
      if (algo_names_.size() == 1 && algo_names_[0] == "all") {
        // Replace with all available TSP algorithms
        std::vector<std::string> tsp_algos;
        auto all_algos = AlgorithmRegistry::availableAlgorithms();

        for (const auto& algo : all_algos) {
          try {
            // Test if it's a TSP algorithm
            AlgorithmRegistry::createTyped<Graph<City, double>, Path>(algo);
            tsp_algos.push_back(algo);
          } catch (...) {
            // Not a TSP algorithm, skip it
            continue;
          }
        }

        if (tsp_algos.empty()) {
          UI::error("No TSP algorithms found");
          return false;
        }

        // Replace the "all" keyword with the list of all TSP algorithms
        algo_names_ = tsp_algos;

        if (verbose_) {
          UI::info(fmt::format("Comparing all TSP algorithms: {}", fmt::join(algo_names_, ", ")));
        }
      } else {
        // Verify all algorithm names exist
        for (const auto& name : algo_names_) {
          if (!AlgorithmRegistry::instance().exists(name)) {
            UI::error(fmt::format("Algorithm '{}' not found", name));
            return false;
          }
        }

        if (verbose_) {
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
      }

      // Use either files or generated data
      if (!input_files_.empty()) {
        compare_algorithms_with_files(
          algo_names_, iterations_, input_files_, debug_, time_limit_ms_
        );
      } else {
        compare_algorithms(algo_names_, iterations_, test_sizes_, debug_, time_limit_ms_);
      }

      if (verbose_) {
        UI::success("Comparison completed successfully");
      }
      return true;
    } catch (const std::exception& e) {
      UI::error(fmt::format("Comparison failed: {}", e.what()));
      return false;
    }
  }

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);

 private:
  std::vector<std::string> algo_names_;
  int iterations_;
  std::vector<int> test_sizes_;
  std::vector<std::string> input_files_;  // Added input files
  bool debug_;
  int time_limit_ms_;
};