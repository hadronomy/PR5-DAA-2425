#pragma once

#include "command_handler.h"
#include "command_registry.h"
#include "ui.h"

/**
 * Command handler for help command
 * Displays usage information about the application
 */
class HelpCommand : public CommandHandler {
 public:
  HelpCommand(bool verbose) : CommandHandler(verbose) {}

  bool execute() override {
    UI::header("Divide and Conquer Algorithm Benchmarking CLI");

    UI::subheader("Commands");
    UI::commandEntry("bench", "<algorithm> [options]", "Benchmark a specific algorithm");
    UI::commandEntry(
      "compare", "<algorithm1> <algorithm2> [options]", "Compare multiple algorithms"
    );
    UI::commandEntry("list", "", "List all available algorithms");
    UI::commandEntry("help", "", "Show this help message");

    UI::subheader("Options");
    UI::commandEntry("--memo", "", "Enable memoization");
    UI::commandEntry("--iterations=N", "", "Set number of iterations (default: 100)");
    UI::commandEntry("--size=N", "", "Set test data size (default: 10000)");
    UI::commandEntry("-v, --verbose", "", "Enable verbose output");

    UI::subheader("Examples");
    UI::exampleCommand("dac bench merge_sort", "Benchmark merge sort algorithm");
    UI::exampleCommand("dac bench merge_sort --memo", "Benchmark with memoization enabled");
    UI::exampleCommand("dac bench merge_sort --size=20000", "Benchmark with larger dataset");
    UI::exampleCommand("dac bench merge_sort --memo --iterations=50", "Combined options");
    UI::exampleCommand("dac compare merge_sort quick_sort", "Compare two algorithms");
    UI::exampleCommand(
      "dac compare merge_sort quick_sort binary_search --memo", "Compare multiple algorithms"
    );
    UI::exampleCommand("dac list", "List all available algorithms");
    UI::exampleCommand("dac --verbose bench merge_sort", "Enable verbose output");

    return true;
  }

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);
};