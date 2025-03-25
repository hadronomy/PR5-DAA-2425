#pragma once

#include <string>
#include <thread>

#include <fmt/core.h>

#include "command_handler.h"
#include "command_registry.h"
#include "visualization/vulkan_renderer.h"

/**
 * Command handler for visualize command
 * Opens a visualization window using Dear ImGui
 */
class VisualizeCommand : public CommandHandler {
 public:
  VisualizeCommand(bool verbose) : CommandHandler(verbose) {}
  ~VisualizeCommand();

  bool execute() override;

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);

 private:
  std::string path_;
  std::unique_ptr<VulkanRenderer> renderer_;
};

inline bool VisualizeCommand::execute() {
  try {
    // Create the Vulkan renderer instance
    renderer_ = std::make_unique<VulkanRenderer>();

    // Initialize and run the renderer
    if (!renderer_->initialize()) {
      if (verbose_) {
        fmt::print(stderr, "Failed to initialize Vulkan renderer\n");
      }
      return false;
    }

    // Start the main rendering loop
    renderer_->run();

    return true;
  } catch (const std::exception& e) {
    if (verbose_) {
      fmt::print(stderr, "Error in visualization: {}\n", e.what());
    }
    return false;
  }
}

inline VisualizeCommand::~VisualizeCommand() {
  // Make sure the renderer is properly cleaned up
  if (renderer_) {
    renderer_->shutdown();
  }
}