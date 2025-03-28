#pragma once

#include <functional>
#include <memory>

namespace daa {

// Forward declaration
class CommandRegistry;

/**
 * @brief Base class for command handlers
 *
 * All command handlers derive from this to provide
 * a consistent interface for command execution.
 */
class CommandHandler {
 public:
  explicit CommandHandler(bool verbose = false) : verbose_(verbose) {}
  virtual ~CommandHandler() = default;

  // Execute the command - returns true if successful
  virtual bool execute() = 0;

  // Get verbose flag
  [[nodiscard]] bool isVerbose() const { return verbose_; }

  // Static registration for derived classes (default implementation)
  static void registerCommand(CommandRegistry& _) {}

 protected:
  bool verbose_ = false;
};

/**
 * @brief CRTP base template for command handlers with standard registration
 *
 * @tparam Derived The concrete command handler type
 */
template <typename Derived>
class CommandHandlerBase : public CommandHandler {
 public:
  using CommandHandler::CommandHandler;

  static void registerCommand(CommandRegistry& _) {
    // Derived classes should implement this method to register with the registry
    // This provides a base implementation to avoid pure virtual behavior
  }
};

/**
 * @brief Helper to create a command factory function for a specific command type
 *
 * @tparam Command The command type to create
 * @param args Constructor arguments for the command
 * @return Factory function that creates the command
 */
template <typename Command, typename... Args>
std::function<std::unique_ptr<CommandHandler>(bool)> makeCommandFactory(Args&&... args) {
  return [... captured_args =
            std::forward<Args>(args)](bool verbose) -> std::unique_ptr<CommandHandler> {
    return std::make_unique<Command>(
      std::forward<decltype(captured_args)>(captured_args)..., verbose
    );
  };
}

}  // namespace daa