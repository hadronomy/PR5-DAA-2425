#include "commands/list.h"

#include <fmt/format.h>

#include "algorithm_factory.h"
#include "ui.h"

namespace daa {

bool ListAlgorithmsCommand::execute() {
  try {
    if (verbose_) {
      UI::info("Listing all available algorithms");
    }

    // Make sure we have algorithms registered before listing
    if (AlgorithmFactory::availableAlgorithms().empty()) {
      UI::warning("No algorithms are currently registered.");
      return true;
    }

    // List algorithms using the registry service
    AlgorithmRegistry::listAlgorithms();

    return true;
  } catch (const std::exception& e) {
    UI::error(fmt::format("Failed to list algorithms: {}", e.what()));
    return false;
  }
}

// Registration is handled by the REGISTER_COMMAND macro in the header

}  // namespace daa