#include "parser/tsp_driver.h"

#include "commands/validate.h"

namespace daa {

bool ValidateCommand::execute() {
  TSPDriver driver;
  bool success = driver.parse_file(path_);
  if (!success) {
    return false;
  }
  std::cout << driver.graph.serializeSimpleFormat() << std::endl;
  return success;
}

void ValidateCommand::registerCommand(CommandRegistry& registry) {
  static std::string input_file;
  static bool verbose = false;

  registry.registerCommandType<ValidateCommand>(
    "validate",
    "Validate the input data",
    [](CLI::App* cmd) {
      cmd->add_option("input", input_file, "Input data file")->required();
      cmd->add_option("--verbose", verbose, "Enable verbose output");
      return cmd;
    },
    [](bool verbose) { return std::make_unique<ValidateCommand>(input_file, verbose); }
  );
}

}  // namespace daa