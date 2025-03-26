#include "commands/visualize.h"

#include "visualization/application.h"

bool VisualizeCommand::execute() {
  VisApplication gui;

  if (!gui.Initialize()) {
    if (verbose_) {
      fmt::print(stderr, "Failed to initialize GUI\n");
    }
    return false;
  }

  gui.Run();
  return true;
}

void VisualizeCommand::registerCommand(CommandRegistry& registry) {
  static bool verbose = false;

  registry.registerCommandType<VisualizeCommand>(
    "visualize",
    "WIP",
    [](CLI::App* cmd) {
      cmd->add_option("--verbose", verbose, "Enable verbose output");
      return cmd;
    },
    [](bool verbose) { return std::make_unique<VisualizeCommand>(verbose); }
  );
}