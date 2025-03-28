#include "command_registry.h"

#include <CLI/CLI.hpp>

std::unordered_map<std::string, CLI::App*> daa::CommandRegistry::setupCommands(CLI::App& app) const {
  std::unordered_map<std::string, CLI::App*> commands;

  for (const auto& name : command_names_) {
    auto description_it = command_descriptions_.find(name);
    auto setup_it = command_setups_.find(name);

    if (description_it == command_descriptions_.end() || setup_it == command_setups_.end()) {
      continue;
    }

    auto* command = app.add_subcommand(name, description_it->second);
    commands[name] = setup_it->second(command);
  }

  return commands;
}