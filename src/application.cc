#include "application.h"

#include "algorithms.h"  // Include this first to trigger static registration
#include "commands.h"
#include "ui.h"

namespace daa {

Application Application::create(const std::string& name, const std::string& description) {
  // setup_algorithm_registry();

  auto algorithms = AlgorithmRegistry::availableAlgorithms();
  if (algorithms.empty()) {
    UI::warning("No algorithms were registered during initialization.");
  }
  return Application(name, description);
}

Application::Application(const std::string& name, const std::string& description)
    : app_(description, name), registry_(CommandRegistry::instance()) {}

Application& Application::withVersion(const std::string& version) {
  app_.set_version_flag("--version", version, "Print version information and exit");
  return *this;
}

Application& Application::withVerboseOption() {
  app_.add_flag("-v,--verbose", verbose_, "Enable detailed output for debugging");
  return *this;
}

Application& Application::withStandardCommands() {
  registerCommand<BenchmarkCommand>()
    .registerCommand<CompareCommand>()
    .registerCommand<ListAlgorithmsCommand>()
    .registerCommand<ValidateCommand>()
    .registerCommand<VisualizeCommand>();
  return *this;
}

int Application::run(int argc, char** argv) {
  // Setup algorithm registry
  // setup_algorithm_registry();

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
