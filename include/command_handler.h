#pragma once

namespace daa {

class CommandRegistry;

/**
 * @brief Base class for command handlers
 *
 * This class defines the interface for command handlers that execute CLI commands.
 */
class CommandHandler {
 public:
  /**
   * Constructor
   *
   * @param verbose Whether to enable verbose output
   */
  CommandHandler(bool verbose = false) : verbose_(verbose) {}

  /**
   * Virtual destructor
   */
  virtual ~CommandHandler() = default;

  /**
   * Execute the command
   *
   * @return true if command executed successfully, false otherwise
   */
  virtual bool execute() = 0;

  /**
   * Register this command type with the command registry
   *
   * @param registry The command registry to register with
   */
  static void registerCommand(CommandRegistry& _) {}

 protected:
  /**
   * Whether verbose output is enabled
   */
  bool verbose_;
};

}  // namespace daa