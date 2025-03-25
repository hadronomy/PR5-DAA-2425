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