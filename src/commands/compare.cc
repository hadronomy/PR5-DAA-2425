#include <functional>
#include <memory>

#include <CLI/CLI.hpp>

#include "benchmarking.h"
#include "commands.h"
#include "config.h"
#include "time_utils.h"
#include "tsp.h"

#include "commands/compare.h"

bool CompareCommand::execute() {
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
      compare_algorithms_with_files(algo_names_, iterations_, input_files_, debug_, time_limit_ms_);
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