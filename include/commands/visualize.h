#pragma once

#include <fmt/core.h>

#include "command_handler.h"
#include "command_registry.h"

namespace daa {

/**
 * Command handler for visualize command
 * Opens a visualization window using Dear ImGui
 */
class VisualizeCommand : public CommandHandlerBase<VisualizeCommand> {
 public:
  explicit VisualizeCommand(bool verbose) : CommandHandlerBase(verbose) {}

  bool execute() override;

  // Register this command with the registry
  static void registerCommand(CommandRegistry& registry);
};

// Auto-register the command
REGISTER_COMMAND(VisualizeCommand);

}  // namespace daa