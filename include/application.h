#pragma once

#include <memory>
#include <string_view>

#include <CLI/CLI.hpp>

#include "algorithms.h"
#include "command_registry.h"

namespace daa {

/**
 * @brief Main application class that sets up and runs the CLI app
 */
class Application {
 public:
  /**
   * @brief Create a new application instance
   *
   * @param name Application name
   * @param description Application description
   * @return Application instance
   */
  static Application create(std::string_view name, std::string_view description);

  /**
   * @brief Set application version
   *
   * @param version Version string
   * @return Reference to this for method chaining
   */
  Application& withVersion(std::string_view version);

  /**
   * @brief Add verbose flag to the application
   *
   * @return Reference to this for method chaining
   */
  Application& withVerboseOption();

  /**
   * @brief Register standard commands with the application
   *
   * @return Reference to this for method chaining
   */
  Application& withStandardCommands();

  /**
   * @brief Set a custom formatter for the CLI
   *
   * @tparam FormatterType Type of formatter to use (must derive from CLI::Formatter)
   * @return Reference to this for method chaining
   */
  template <typename FormatterType>
  requires std::derived_from<FormatterType, CLI::Formatter> Application& withFormatter() {
    formatter_ = std::make_shared<FormatterType>();
    app_.formatter(formatter_);
    return *this;
  }

  /**
   * @brief Set a custom formatter instance for the CLI
   *
   * @param formatter Shared pointer to an existing formatter instance
   * @return Reference to this for method chaining
   */
  Application& withFormatter(std::shared_ptr<CLI::Formatter> formatter) {
    formatter_ = std::move(formatter);
    app_.formatter(formatter_);
    return *this;
  }

  /**
   * @brief Register a command with the application
   *
   * @tparam CommandType Type of command to register
   * @return Reference to this for method chaining
   */
  template <typename CommandType>
  requires CommandHandlerType<CommandType> Application& registerCommand() {
    CommandType::registerCommand(registry_);
    return *this;
  }

  /**
   * @brief Run the application with command line arguments
   *
   * @param argc Argument count
   * @param argv Argument values
   * @return Exit code
   */
  int run(int argc, char** argv);

 private:
  Application(std::string_view name, std::string_view description);

  // Setup all registered commands
  void setupCommands();

  // Main CLI app
  CLI::App app_;

  // Command registry reference
  CommandRegistry& registry_;

  // Verbose flag
  bool verbose_ = false;

  // Custom formatter if set
  std::shared_ptr<CLI::Formatter> formatter_;
};

}  // namespace daa