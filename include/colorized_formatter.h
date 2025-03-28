#pragma once

#include <string>
#include <vector>

#include <fmt/color.h>
#include <fmt/core.h>
#include <CLI/CLI.hpp>

#include "config.h"

namespace daa {

/**
 * @class ColorizedFormatter
 * @brief Custom formatter for CLI11 help text with enhanced colorization
 */
class ColorizedFormatter : public CLI::Formatter {
 public:
  ColorizedFormatter() : CLI::Formatter() {
    CLI::Formatter::column_width(35);
    label("OPTIONS", "");
    label("COMMANDS", "");  // Changed from SUBCOMMANDS to COMMANDS
  }

  // Add const version of column_width() getter
  std::size_t column_width() const { return column_width_; }

  std::string make_option(const CLI::Option* opt, bool is_positional) const override {
    // Get the base formatting from CLI11
    auto result = CLI::Formatter::make_option(opt, is_positional);

    if (result.empty())
      return result;

    // First try to find the standard delimiter
    size_t help_pos = result.find(" - ");

    // If standard delimiter not found, look for multiple spaces instances
    if (help_pos == std::string::npos) {
      // Find positions where multiple spaces begin
      std::vector<size_t> space_positions;
      bool in_spaces = false;

      for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] == ' ') {
          if (in_spaces)
            continue;
          in_spaces = true;
          size_t j = i;
          while (j < result.size() && result[j] == ' ')
            j++;
          if (j - i >= 3) {  // At least 3 spaces indicates separation
            space_positions.push_back(i);
          }
        } else {
          in_spaces = false;
        }
      }

      // Use the second instance if available, otherwise use the first
      if (space_positions.size() >= 2) {
        help_pos = space_positions[1];
      } else if (!space_positions.empty()) {
        help_pos = space_positions[0];
      }
    }

    // If we found a position to split at
    if (help_pos != std::string::npos) {
      std::string option_part = result.substr(0, help_pos);
      std::string desc_part = result.substr(help_pos);

      return fmt::format(fg(config::colors::option_name), "  {}", option_part) +
             fmt::format(fg(config::colors::info), "{}", desc_part);
    }

    // Fallback if no suitable split position found
    return fmt::format(fg(config::colors::info), "  {}", result);
  }

  std::string make_subcommand(const CLI::App* app) const override {
    // Match format with options - use two spaces then option name with fixed width
    std::string name = app->get_name();
    std::string desc = app->get_description();

    // Match the spacing seen in the example
    return fmt::format(fg(config::colors::command_name), "  {:<25}", name) + desc + "\n";
  }

  std::string make_group(
    std::string group,
    bool is_positional,
    std::vector<const CLI::Option*> opts
  ) const override {
    if (group.empty()) {
      group = is_positional ? "Positional Arguments" : "Options";
    }

    // Custom section header format with underline
    std::string result =
      fmt::format(fg(config::colors::section_heading) | fmt::emphasis::bold, "\n{}\n", group);
    result +=
      fmt::format(fg(config::colors::section_heading), "{}\n\n", std::string(group.length(), '-'));

    for (const CLI::Option* opt : opts) {
      result += make_option(opt, is_positional);
    }

    return result;
  }

  // Override the subcommand group to match options formatting
  std::string make_subcommands(const CLI::App* app, CLI::AppFormatMode mode) const override {
    std::string result;
    std::vector<const CLI::App*> subcommands = app->get_subcommands({});

    if (!subcommands.empty()) {
      // Format exactly like options group with the same styling
      std::string group = "COMMANDS";
      result =
        fmt::format(fg(config::colors::section_heading) | fmt::emphasis::bold, "\n{}\n", group);
      result += fmt::format(
        fg(config::colors::section_heading), "{}\n\n", std::string(group.length(), '-')
      );

      for (const CLI::App* subcom : subcommands) {
        result += make_subcommand(subcom);
      }
    }

    return result;
  }

  // Override to control section order, colorize usage and include examples
  std::string make_help(const CLI::App* app, std::string name, CLI::AppFormatMode mode)
    const override {
    std::string out = "\n";

    const auto title_style = fg(config::colors::banner_text) | fmt::emphasis::bold;
    const auto version_style = fg(config::colors::info);

    out += fmt::format(title_style, "{}", root_name(app));
    out += " is " + root(app)->get_description();
    out += fmt::format(version_style, " ({})\n", config::app_version);

    std::string usage_str = "Usage: ";
    std::string app_name = app->get_name().empty() ? "app" : app->get_name();

    if (!app->get_parent()) {
      usage_str += app_name;
    } else {
      std::string parent_name =
        app->get_parent()->get_name().empty() ? "app" : app->get_parent()->get_name();
      usage_str += parent_name + " " + app_name;
    }

    // Add options if any - but with proper formatting
    std::string options = make_usage(app, "");
    if (!options.empty()) {
      std::string cleaned_options;
      bool in_bracket = false;
      bool append = false;

      for (size_t i = 0; i < options.length(); ++i) {
        if (options[i] == '[') {
          in_bracket = true;
          append = true;
        }
        if (append) {
          cleaned_options += options[i];
        }
        if (options[i] == ']') {
          in_bracket = false;
        }
      }
      if (!cleaned_options.empty()) {
        usage_str += " " + cleaned_options;
      }
    }

    // Add colorized usage with proper newline spacing
    out += "\n" + fmt::format(fg(config::colors::usage), "{}", usage_str);

    // Put subcommands section first
    out += make_subcommands(app, mode);

    // Then the options
    out += make_groups(app, mode);

    // Add examples section if we have subcommands
    if (!app->get_subcommands({}).empty()) {
      std::string group = "EXAMPLES";
      out +=
        fmt::format(fg(config::colors::section_heading) | fmt::emphasis::bold, "\n{}\n", group);
      out += fmt::format(
        fg(config::colors::section_heading), "{}\n\n", std::string(group.length(), '-')
      );

      out += fmt::format(fg(config::colors::example), "  TODO \n");

      // TODO: Add examples here
    }

    // Add learn more link at the bottom
    out += "\nLearn more: ";
    out += fmt::format(fg(config::colors::banner_text), "{}\n", config::repo_url);

    return out;
  }

  const CLI::App* root(const CLI::App* app) const {
    const CLI::App* current = app;
    while (current->get_parent()) {
      current = current->get_parent();
    }
    return current;
  }

  const std::string root_name(const CLI::App* app) const { return root(app)->get_name(); }
};

}  // namespace daa