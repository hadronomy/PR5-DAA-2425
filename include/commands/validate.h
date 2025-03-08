#pragma once

#include "command_handler.h"
#include "command_registry.h"
#include "parser/tsp_driver.h"
#include "ui.h"

/**
 * Command handler for help command
 * Displays usage information about the application
 */
class ValidateCommand : public CommandHandler {
 public:
  ValidateCommand(const std::string& path, bool verbose) : CommandHandler(verbose), path_(path) {}

  bool execute() override {

    TSPDriver driver;
    bool success = driver.parse_file(path_);
    if (!success) {
      return false;
    }
    std::cout << driver.graph.serializeSimpleFormat() << std::endl;
    return success;
  }

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);

 private:
  std::string path_;
};