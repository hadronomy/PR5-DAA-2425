#pragma once

#include "command_handler.h"
#include "command_registry.h"

namespace daa {

/**
 * Command handler for listing available algorithms
 */
class ListAlgorithmsCommand : public CommandHandlerBase<ListAlgorithmsCommand> {
 public:
  explicit ListAlgorithmsCommand(bool verbose) : CommandHandlerBase(verbose) {}

  bool execute() override;

  // Register this command with the registry
  static void registerCommand(CommandRegistry& registry) {
    registry.registerCommandType<ListAlgorithmsCommand>(
      "list",
      "List all available algorithms",
      [](CLI::App* cmd) { return cmd; },  // No additional CLI options needed
      [](bool verbose) { return std::make_unique<ListAlgorithmsCommand>(verbose); }
    );
  }
};

// Auto-register the command
REGISTER_COMMAND(ListAlgorithmsCommand);

}  // namespace daa