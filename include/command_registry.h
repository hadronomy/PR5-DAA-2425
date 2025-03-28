#pragma once

#include <CLI/CLI.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "command_handler.h"

namespace daa {

/**
 * @class CommandRegistry
 * @brief Registry for CLI commands with factory methods
 */
class CommandRegistry {
 public:
  // Command handler factory function type
  using CommandHandlerFactory = std::function<std::unique_ptr<CommandHandler>(bool verbose)>;

  // Simple command function type (for legacy support)
  using SimpleCommandHandler = std::function<int(const std::vector<std::string>&)>;

  // Singleton access
  static CommandRegistry& instance() {
    static CommandRegistry instance;
    return instance;
  }

  /**
   * Register a command with the registry
   *
   * @param name Command name
   * @param description Command description
   * @param factory Factory function to create command handler
   * @param setupFunc Function to setup command options
   */
  void registerCommand(
    const std::string& name,
    const std::string& description,
    CommandHandlerFactory factory,
    std::function<void(CLI::App*)> setupFunc
  ) {
    factories_[name] = std::move(factory);
    setupFunctions_[name] = std::move(setupFunc);
    commandInfo_[name] = description;
  }

  /**
   * Register a simple command function
   */
  void register_command(const std::string& name, SimpleCommandHandler handler) {
    simpleCommands_[name] = handler;
  }

  /**
   * Execute a simple command
   */
  int execute(const std::string& command, const std::vector<std::string>& args) {
    auto it = simpleCommands_.find(command);
    if (it == simpleCommands_.end()) {
      std::cerr << "Unknown command: " << command << std::endl;
      std::cerr << "Run 'help' for a list of available commands" << std::endl;
      return 1;
    }

    return it->second(args);
  }

  /**
   * Set up all registered commands for a CLI::App
   *
   * @param app CLI::App instance to set up
   * @return Map of command name to CLI::App subcommand
   */
  std::map<std::string, CLI::App*> setupCommands(CLI::App& app) {
    std::map<std::string, CLI::App*> commands;

    for (const auto& [name, description] : commandInfo_) {
      auto cmd = app.add_subcommand(name, description);
      commands[name] = cmd;

      // Call the setup function for this command
      if (auto it = setupFunctions_.find(name); it != setupFunctions_.end()) {
        it->second(cmd);
      }
    }

    return commands;
  }

  /**
   * Create a command handler based on the parsed command
   *
   * @param commandName Name of the command to create
   * @param verbose Whether verbose mode is enabled
   * @return Unique pointer to command handler
   */
  std::unique_ptr<CommandHandler> createHandler(const std::string& commandName, bool verbose) {
    auto it = factories_.find(commandName);
    if (it != factories_.end()) {
      return it->second(verbose);
    }
    return nullptr;
  }

  /**
   * Template method for easy command registration
   *
   * @tparam CommandType Type of command handler class
   * @param name Command name
   * @param description Command description
   * @param setupFunc Function to set up command options
   */
  template <typename CommandType>
  void registerCommandType(
    const std::string& name,
    const std::string& description,
    std::function<CLI::App*(CLI::App*)> setupFunc,
    std::function<std::unique_ptr<CommandType>(bool)> factory
  ) {

    static_assert(
      std::is_base_of<CommandHandler, CommandType>::value,
      "Command type must inherit from CommandHandler"
    );

    registerCommand(
      name,
      description,
      [factory](bool verbose) -> std::unique_ptr<CommandHandler> { return factory(verbose); },
      [setupFunc](CLI::App* app) { setupFunc(app); }
    );
  }

 private:
  // Private constructor for singleton
  CommandRegistry() = default;

  // Maps to store command data
  std::unordered_map<std::string, CommandHandlerFactory> factories_;
  std::unordered_map<std::string, std::function<void(CLI::App*)>> setupFunctions_;
  std::unordered_map<std::string, std::string> commandInfo_;
  std::unordered_map<std::string, SimpleCommandHandler> simpleCommands_;
};

}  // namespace daa