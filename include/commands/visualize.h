#pragma once

#include <string>

#include <fmt/core.h>

#include "command_handler.h"
#include "command_registry.h"
#include "visualization/gui_manager.h"

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

inline bool VisualizeCommand::execute() {
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
