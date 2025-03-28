#pragma once

#include <fmt/core.h>
#include <string>

#include "command_handler.h"
#include "command_registry.h"

namespace daa {

/**
 * Command handler for validate command
 * Validates input data files
 */
class ValidateCommand : public CommandHandlerBase<ValidateCommand> {
 public:
  ValidateCommand(const std::string& path, bool verbose)
      : CommandHandlerBase(verbose), path_(path) {}

  bool execute() override;

  // Register this command with the registry
  static void registerCommand(CommandRegistry& registry);

 private:
  std::string path_;
};

// Auto-register the command
REGISTER_COMMAND(ValidateCommand);

}  // namespace daa