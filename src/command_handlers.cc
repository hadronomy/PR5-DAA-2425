#include <functional>
#include <memory>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "config.h"
#include "time_utils.h"

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

void CompareCommand::registerCommand(CommandRegistry& registry) {
  // Static variables for command options
  static std::vector<std::string> compare_algo_names;
  static int compare_iterations = config::default_iterations;
  static std::vector<int> compare_test_sizes(
    config::default_test_size.begin(), config::default_test_size.end()
  );
  static std::vector<std::string> compare_input_files;
  static bool debug = false;
  static std::string time_limit_str = "30s";
  static int time_limit_ms = Algorithm::DEFAULT_TIME_LIMIT_MS;

  // Create a validator for algorithm names
  auto algoNamesValidator = CLI::Validator(
    [](std::string& val) {
      // Validate individual algorithm names
      if (val == "all") {
        return std::string();
      }
      // Add your algorithm name validation logic here
      // Return empty string for valid names
      // Return error message string for invalid names
      return std::string();
    },
    "Valid algorithm name required",
    "Algorithm validator"
  );

  registry.registerCommandType<CompareCommand>(
    "compare",
    "Compare multiple algorithms",
    [&](CLI::App* cmd) {
      // Allow a single algorithm name (for "all") or multiple algorithms
      auto algo_option =
        cmd
          ->add_option(
            "algorithms", compare_algo_names, "Algorithms to compare (use 'all' for all algorithms)"
          )
          ->required()
          ->expected(1, -1);

      // Add a final callback to validate the entire vector after parsing
      cmd->parse_complete_callback([&]() {
        if (compare_algo_names.size() == 1 && compare_algo_names[0] != "all") {
          throw CLI::ValidationError(
            "algorithms",
            "When specifying a single algorithm, use 'all' to compare all algorithms or specify at "
            "least two algorithm names"
          );
        }
        try {
          time_limit_ms = time_utils::parseTimeToMs(time_limit_str);
        } catch (const std::exception& e) {
          throw CLI::ValidationError("--time-limit", e.what());
        }
      });

      cmd->add_option("--iterations", compare_iterations, "Number of iterations");

      // Create mutually exclusive options group for -N and --file
      auto size_option =
        cmd
          ->add_option(
            "-N,--size", compare_test_sizes, "Size(s) of test data (can specify multiple)"
          )
          ->delimiter(',');

      auto file_option = cmd
                           ->add_option(
                             "-f,--file",
                             compare_input_files,
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

      return cmd;
    },
    [&](bool verbose) {
      return std::make_unique<CompareCommand>(
        compare_algo_names,
        compare_iterations,
        compare_test_sizes,
        compare_input_files,
        verbose,
        debug,
        time_limit_ms
      );
    }
  );
}

void HelpCommand::registerCommand(CommandRegistry& registry) {
  registry.registerCommandType<HelpCommand>(
    "help",
    "Show help information",
    [](CLI::App* cmd) { return cmd; },
    [](bool verbose) { return std::make_unique<HelpCommand>(verbose); }
  );
}

// Add the implementation for ListAlgorithmsCommand::registerCommand
void ListAlgorithmsCommand::registerCommand(CommandRegistry& registry) {
  registry.registerCommandType<ListAlgorithmsCommand>(
    "list",
    "List all available algorithms",
    [](CLI::App* cmd) { return cmd; },  // No additional CLI options needed
    [](bool verbose) { return std::make_unique<ListAlgorithmsCommand>(verbose); }
  );
}

void ValidateCommand::registerCommand(CommandRegistry& registry) {
  static std::string input_file;
  static bool verbose = false;

  registry.registerCommandType<ValidateCommand>(
    "validate",
    "Validate the input data",
    [](CLI::App* cmd) {
      cmd->add_option("input", input_file, "Input data file")->required();
      cmd->add_option("--verbose", verbose, "Enable verbose output");
      return cmd;
    },
    [](bool verbose) { return std::make_unique<ValidateCommand>(input_file, verbose); }
  );
}

void VisualizeCommand::registerCommand(CommandRegistry& registry) {
  static bool verbose = false;

  registry.registerCommandType<VisualizeCommand>(
    "visualize",
    "WIP",
    [](CLI::App* cmd) {
      cmd->add_option("--verbose", verbose, "Enable verbose output");
      return cmd;
    },
    [](bool verbose) { return std::make_unique<VisualizeCommand>(verbose); }
  );
}
