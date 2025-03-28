#include <string>

#include "application.h"
#include "colorized_formatter.h"
#include "config.h"
#include "ui.h"

using namespace daa;

int main(int argc, char** argv) {
  try {
    return Application::create(config::app_name, config::app_description)
      .withFormatter<ColorizedFormatter>()
      .withVersion(config::app_version)
      .withVerboseOption()
      .withStandardCommands()
      .run(argc, argv);
  } catch (const std::exception& e) {
    UI::error(fmt::format("Fatal error: {}", e.what()));
    return 1;
  }
}
