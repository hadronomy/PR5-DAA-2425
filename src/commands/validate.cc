#include "commands/validate.h"

#include <fmt/format.h>
#include <CLI/CLI.hpp>

#include "parser/vrpt_driver.h"
#include "problem/vrpt_problem.h"
#include "ui.h"

namespace daa {

bool ValidateCommand::execute() {
  try {
    const auto& problem = VRPTProblem::loadFile(path_);
    bool success = problem.has_value();
    if (!success) {
      UI::error(fmt::format("Failed to parse file: {}", path_));
      return false;
    }
    std::cout << "File parsed successfully." << std::endl;
    const auto& vrpt_problem = problem.value();
    if (!vrpt_problem.isLoaded()) {
      UI::error("Problem data is not loaded.");
      return false;
    }
    std::cout << "Problem data loaded successfully." << std::endl;
    std::cout << problem->toString() << std::endl;
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