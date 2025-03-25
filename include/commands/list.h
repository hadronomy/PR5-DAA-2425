#pragma once

#include <fmt/core.h>

#include "command_handler.h"
#include "command_registry.h"

/**
 * Command handler for listing available algorithms
 */
class ListAlgorithmsCommand : public CommandHandler {
 public:
  ListAlgorithmsCommand(bool verbose) : CommandHandler(verbose) {}

  bool execute() override;

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);
};