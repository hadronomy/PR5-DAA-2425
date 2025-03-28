#pragma once

#include <memory>
#include <string>

#include <CLI/CLI.hpp>

#include "command_registry.h"

namespace daa {

/**
 * @class Application
 * @brief Main application class that encapsulates CLI setup and execution
 *
 * This class provides a cleaner API for setting up the application,
 * registering commands, and executing the command line interface.
 */
class Application {
 public:
  /**
   * @brief Create a new Application instance
   *
   * @param name Application name
   * @param description Application description
   * @return Application instance with basic configuration
   */
  static Application create(const std::string& name, const std::string& description);

  /**
   * @brief Set the application version
   *
   * @param version Version string
   * @return Reference to this Application for method chaining
   */
  Application& withVersion(const std::string& version);

  /**
   * @brief Enable verbose output option
   *
   * @return Reference to this Application for method chaining
   */
  Application& withVerboseOption();

  /**
   * @brief Use custom formatter for help output
   *
   * @tparam FormatterType Type of formatter to use
   * @return Reference to this Application for method chaining
   */
  template <typename FormatterType>
  Application& withFormatter() {
    auto formatter = std::make_shared<FormatterType>();
    app_.formatter(formatter);
    formatter_ = formatter;
    return *this;
  }

  /**
   * @brief Register all standard benchmark commands
   *
   * @return Reference to this Application for method chaining
   */
  Application& withStandardCommands();

  /**
   * @brief Register a specific command
   *
   * @tparam CommandType Type of command to register
   * @return Reference to this Application for method chaining
   */
  template <typename CommandType>
  Application& registerCommand() {
    CommandType::registerCommand(registry_);
    return *this;
  }

  /**
   * @brief Parse command line arguments and execute the selected command
   *
   * @param argc Argument count
   * @param argv Argument values
   * @return Exit code (0 for success, non-zero for error)
   */
  int run(int argc, char** argv);

 private:
  CLI::App app_;
  CommandRegistry& registry_;
  std::shared_ptr<CLI::FormatterBase> formatter_;
  bool verbose_ = false;

  Application(const std::string& name, const std::string& description);
  void setupCommands();
};

}  // namespace daa