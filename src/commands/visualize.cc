#include "commands/visualize.h"

#include "visualization/gui_manager.h"

bool VisualizeCommand::execute() {
  GuiManager gui;

  if (!gui.initialize()) {
    if (verbose_) {
      fmt::print(stderr, "Failed to initialize GUI\n");
    }
    return false;
  }

  gui.run();
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