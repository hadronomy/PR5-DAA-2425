#pragma once

#include "algorithm_registry.h"
#include "command_handler.h"
#include "command_registry.h"
#include "ui.h"

/**
 * Command handler for listing available algorithms
 */
class ListAlgorithmsCommand : public CommandHandler {
 public:
  ListAlgorithmsCommand(bool verbose) : CommandHandler(verbose) {}

  bool execute() override {
    try {
      if (verbose_) {
        UI::info("Listing all available algorithms");
      }

      // Make sure we have algorithms registered before listing
      if (AlgorithmRegistry::availableAlgorithms().empty()) {
        UI::warning("No algorithms are currently registered.");
        return true;
      }

      // Call the correct method to list algorithms
      AlgorithmRegistry::listAlgorithms();

      return true;
    } catch (const std::exception& e) {
      UI::error(fmt::format("Failed to list algorithms: {}", e.what()));
      return false;
    }
  }

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);
};