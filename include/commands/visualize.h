#pragma once

#include <string>

#include <fmt/core.h>

#include "command_handler.h"
#include "command_registry.h"

/**
 * Command handler for visualize command
 * Opens a visualization window using Dear ImGui
 */
class VisualizeCommand : public CommandHandler {
 public:
  VisualizeCommand(bool verbose) : CommandHandler(verbose) {}

  bool execute() override;

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);

 private:
  std::string path_;
};
