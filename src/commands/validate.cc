#include "commands/validate.h"

#include <fmt/format.h>
#include <CLI/CLI.hpp>

#include "parser/tsp_driver.h"
#include "ui.h"

namespace daa {

bool ValidateCommand::execute() {
  try {
    TSPDriver driver;
    bool success = driver.parse_file(path_);
    if (!success) {
      UI::error(fmt::format("Failed to parse file: {}", path_));
      return false;
    }
    std::cout << driver.graph.serializeSimpleFormat() << std::endl;
    return true;
  } catch (const std::exception& e) {
    UI::error(fmt::format("Validation failed: {}", e.what()));
    return false;
  }
}

void ValidateCommand::registerCommand(CommandRegistry& registry) {
  static std::string input_file;

  registry.registerCommandType<ValidateCommand>(
    "validate",
    "Validate the input data",
    [](CLI::App* cmd) {
      cmd->add_option("input", input_file, "Input data file")->required();
      return cmd;
    },
    [](bool verbose) { return std::make_unique<ValidateCommand>(input_file, verbose); }
  );
}

}  // namespace daa