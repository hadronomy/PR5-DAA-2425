#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "command_handler.h"

namespace CLI {
class App;
};

namespace daa {

class CommandHandler;

// Concepts for commands and their capabilities
template <typename T>
concept CommandHandlerType = requires(T instance) {
  { instance.execute() } -> std::convertible_to<bool>;
  { T::registerCommand(std::declval<class CommandRegistry&>()) } -> std::same_as<void>;
};

/**
 * @class CommandRegistry
 *
 * Registry for command handlers with factory methods to create them
 * at runtime based on CLI input.
 */
class CommandRegistry {
 public:
  // Type definitions for command factories and command setup functions
  using CommandFactory = std::function<std::unique_ptr<CommandHandler>(bool verbose)>;
  using CommandSetup = std::function<CLI::App*(CLI::App*)>;

  // Get singleton instance
  static CommandRegistry& instance() {
    static CommandRegistry instance;
    return instance;
  }

  // Register a command with all required metadata
  void registerCommand(
    std::string_view name,
    std::string_view description,
    CommandSetup setup_function,
    CommandFactory factory_function
  ) {
    std::string name_str(name);

    // Check if this command name is already registered
    if (command_factories_.find(name_str) != command_factories_.end()) {
      // Command already registered, skip re-registration
      return;
    }

    command_names_.push_back(name_str);
    command_descriptions_[name_str] = std::string(description);
    command_setups_[name_str] = std::move(setup_function);
    command_factories_[name_str] = std::move(factory_function);
  }

  // Check if a command exists
  [[nodiscard]] bool commandExists(std::string_view name) const {
    return command_factories_.find(std::string(name)) != command_factories_.end();
  }

  // Create a handler for a command by name
  [[nodiscard]] std::unique_ptr<CommandHandler> createHandler(std::string_view name, bool verbose)
    const {
    auto it = command_factories_.find(std::string(name));
    if (it == command_factories_.end()) {
      return nullptr;
    }
    return it->second(verbose);
  }

  // Setup all CLI commands
  [[nodiscard]] std::unordered_map<std::string, CLI::App*> setupCommands(CLI::App& app) const;

  // Type-safe registration helper for command types
  template <typename Command>
  requires CommandHandlerType<Command> void registerCommandType(
    std::string_view name,
    std::string_view description,
    CommandSetup setup_function,
    CommandFactory factory_function
  ) {
    registerCommand(name, description, std::move(setup_function), std::move(factory_function));
  }

  // Enhanced registration helper with automatic type deduction
  template <typename Command>
  requires CommandHandlerType<Command> void registerCommandType(
    std::string_view name,
    std::string_view description,
    std::function<CLI::App*(CLI::App*)> setup,
    std::function<std::unique_ptr<Command>(bool)> factory
  ) {
    registerCommand(
      name,
      description,
      std::move(setup),
      [factory = std::move(factory)](bool verbose) -> std::unique_ptr<CommandHandler> {
        return factory(verbose);
      }
    );
  }

 private:
  // Private constructor for singleton
  CommandRegistry() = default;

  // Command registration data
  std::vector<std::string> command_names_;  // Preserve order for help display
  std::unordered_map<std::string, std::string> command_descriptions_;
  std::unordered_map<std::string, CommandSetup> command_setups_;
  std::unordered_map<std::string, CommandFactory> command_factories_;
};

/**
 * @brief Helper for simplified command registration
 *
 * @tparam T Command implementation class
 */
template <typename T>
requires CommandHandlerType<T> struct CommandRegistrar {
  CommandRegistrar() { T::registerCommand(CommandRegistry::instance()); }
};

// Enhanced macros for command registration
#define REGISTER_COMMAND(className)                                    \
  namespace {                                                          \
  static const daa::CommandRegistrar<className> className##_registrar; \
  }

}  // namespace daa