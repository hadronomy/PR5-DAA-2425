#include "algorithm_registry.h"
#include "ui.h"

#include "commands/list.h"

bool ListAlgorithmsCommand::execute() {
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

void ListAlgorithmsCommand::registerCommand(CommandRegistry& registry) {
  registry.registerCommandType<ListAlgorithmsCommand>(
    "list",
    "List all available algorithms",
    [](CLI::App* cmd) { return cmd; },  // No additional CLI options needed
    [](bool verbose) { return std::make_unique<ListAlgorithmsCommand>(verbose); }
  );
}
