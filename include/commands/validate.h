#pragma once

#include <fmt/core.h>

#include "command_handler.h"
#include "command_registry.h"

/**
 * Command handler for help command
 * Displays usage information about the application
 */
class ValidateCommand : public CommandHandler {
 public:
  ValidateCommand(const std::string& path, bool verbose) : CommandHandler(verbose), path_(path) {}

  bool execute() override;

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);

 private:
  std::string path_;
};