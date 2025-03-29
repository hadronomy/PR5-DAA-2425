#pragma once

#include <fmt/core.h>
#include <string>
#include <vector>

#include "algorithm_registry.h"
#include "command_handler.h"
#include "command_registry.h"

namespace daa {

/**
 * Command handler for compare command
 * Compares performance of multiple algorithms
 */
class CompareCommand : public CommandHandlerBase<CompareCommand> {
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
      : CommandHandlerBase(verbose),
        algo_names_(std::move(algo_names)),
        iterations_(iterations),
        test_sizes_(std::move(test_sizes)),
        input_files_(std::move(input_files)),
        debug_(debug),
        time_limit_ms_(time_limit_ms) {}

  bool execute() override;

  // Register this command with the registry
  static void registerCommand(CommandRegistry& registry);

 private:
  std::vector<std::string> algo_names_;
  int iterations_;
  std::vector<int> test_sizes_;
  std::vector<std::string> input_files_;
  bool debug_;
  int time_limit_ms_;
};

// Auto-register the command
REGISTER_COMMAND(CompareCommand);

}  // namespace daa