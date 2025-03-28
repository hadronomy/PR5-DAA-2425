#pragma once

#include <fmt/core.h>

#include "algorithm_registry.h"
#include "command_handler.h"
#include "command_registry.h"

namespace daa {

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

  bool execute() override;

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

}  // namespace daa