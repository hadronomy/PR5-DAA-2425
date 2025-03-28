
#include "ui.h"

#include "commands/benchmark.h"

#include <CLI/CLI.hpp>

#include "commands.h"
#include "config.h"
#include "time_utils.h"

namespace daa {

bool BenchmarkCommand::execute() {
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
      throw new std::runtime_error("TODO");
      // run_benchmark_with_files(algo_name_, iterations_, input_files_, debug_, time_limit_ms_);
    } else {
      throw new std::runtime_error("TODO");
      // run_benchmark(algo_name_, iterations_, test_sizes_, debug_, time_limit_ms_);
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

void BenchmarkCommand::registerCommand(CommandRegistry& registry) {
  // Static variables for command options
  static std::string bench_algo_name;
  static int bench_iterations = config::default_iterations;
  static std::vector<int> bench_test_sizes(
    config::default_test_size.begin(), config::default_test_size.end()
  );
  static std::vector<std::string> bench_input_files;
  static bool debug = false;
  static std::string time_limit_str = "30s";
  static int time_limit_ms = Algorithm::DEFAULT_TIME_LIMIT_MS;

  registry.registerCommandType<BenchmarkCommand>(
    "bench",
    "Benchmark a specific algorithm",
    [&](CLI::App* cmd) {
      cmd->add_option("algorithm", bench_algo_name, "Algorithm to benchmark")->required();
      cmd->add_option("--iterations", bench_iterations, "Number of iterations");

      // Create mutually exclusive options group for -N and --file
      auto size_option =
        cmd
          ->add_option("-N,--size", bench_test_sizes, "Size(s) of test data (can specify multiple)")
          ->delimiter(',');

      auto file_option = cmd
                           ->add_option(
                             "-f,--file",
                             bench_input_files,
                             "Input file(s) with graphs to benchmark (can specify multiple)"
                           )
                           ->delimiter(',');

      // Make -N and --file options mutually exclusive
      size_option->excludes(file_option);
      file_option->excludes(size_option);

      cmd->add_flag("--debug", debug, "Enable debug output");
      cmd->add_option(
        "-t,--time-limit",
        time_limit_str,
        "Time limit per algorithm run (e.g. '30s', '1m30s', '1h', or milliseconds)"
      );

      // Parse the time limit string after command line parsing
      cmd->parse_complete_callback([&]() {
        try {
          time_limit_ms = time_utils::parseTimeToMs(time_limit_str);
        } catch (const std::exception& e) {
          throw CLI::ValidationError("--time-limit", e.what());
        }
      });

      return cmd;
    },
    [&](bool verbose) {
      return std::make_unique<BenchmarkCommand>(
        bench_algo_name,
        bench_iterations,
        bench_test_sizes,
        bench_input_files,
        verbose,
        debug,
        time_limit_ms
      );
    }
  );
}
}  // namespace daa