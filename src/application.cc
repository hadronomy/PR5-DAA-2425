#include "application.h"

#include <fmt/format.h>

#include "algorithm_factory.h"
#include "algorithms.h"  // Include this first to trigger static registration
#include "ui.h"

namespace daa {

Application Application::create(std::string_view name, std::string_view description) {
  auto algorithms = AlgorithmFactory::availableAlgorithms();
  if (algorithms.empty()) {
    UI::warning("No algorithms were registered during initialization.");
  }
  return Application(name, description);
}

Application::Application(std::string_view name, std::string_view description)
    : app_(std::string(description), std::string(name)), registry_(CommandRegistry::instance()) {}

Application& Application::withVersion(std::string_view version) {
  app_.set_version_flag("--version", std::string(version), "Print version information and exit");
  return *this;
}

Application& Application::withVerboseOption() {
  app_.add_flag("-v,--verbose", verbose_, "Enable detailed output for debugging");
  return *this;
}

Application& Application::withStandardCommands() {
  // Commands are now auto-registered via the REGISTER_COMMAND macro
  // But we need to make sure the command headers are included
  return *this;
}

int Application::run(int argc, char** argv) {
  // Set up help flags
  app_.set_help_flag("-h,--help", "Display help information");
  app_.set_help_all_flag("--help-all", "Show help for all subcommands");

  // Allow fallthrough for help formatting and flag handling
  app_.fallthrough();

  // Set up all registered commands
  setupCommands();

  // Require a subcommand
  app_.require_subcommand(1);

  try {
    // Parse command line
    app_.parse(argc, argv);

    // Find which command was parsed
    for (const auto& cmd : app_.get_subcommands()) {
      if (cmd->parsed()) {
        auto handler = registry_.createHandler(cmd->get_name(), verbose_);
        if (handler) {
          return handler->execute() ? 0 : 1;
        }
        break;
      }
    }

    return 0;
  } catch (const CLI::ParseError& e) {
    return app_.exit(e);
  } catch (const std::exception& e) {
    UI::error(e.what());
    return 1;
  }
}

void Application::setupCommands() {
  auto commands = registry_.setupCommands(app_);

  // Configure each subcommand with formatter and other options
  for (const auto& [name, subcommand] : commands) {
    subcommand->prefix_command(false);
    if (formatter_) {
      subcommand->formatter(formatter_);
    }
  }
}

}  // namespace daa
