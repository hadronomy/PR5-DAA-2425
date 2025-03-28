#pragma once

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <tabulate/color.hpp>
#include <tabulate/column.hpp>
#include <tabulate/table.hpp>
#include <thread>

#include "config.h"

namespace daa {

/**
 * @class UI
 * @brief Contains methods for formatted UI output
 */
class UI {
 private:
  // Helper function to repeat a string n times
  static std::string repeat(const std::string& str, size_t n) {
    std::string result;
    result.reserve(str.size() * n);
    for (size_t i = 0; i < n; ++i) {
      result += str;
    }
    return result;
  }

 public:
  // Display the application banner with stylized text
  static void displayBanner() {
    const auto title_style = fg(config::colors::banner_text) | fmt::emphasis::bold;
    const auto version_style = fg(config::colors::info);

    fmt::print(title_style, "\n{}", config::app_name);
    fmt::print(" is {}", config::app_description);
    fmt::print(version_style, " ({})\n\n", config::app_version);
  }

  // Print a section heading
  static void sectionHeading(std::string_view title) {
    fmt::print(fg(config::colors::section_heading) | fmt::emphasis::bold, "\n{}:\n", title);
  }

  // Print a command with description in column format
  static void
    commandEntry(std::string_view command, std::string_view description, int padding = 20) {
    fmt::print(fg(config::colors::command_name) | fmt::emphasis::bold, "  {}", command);

    // Calculate padding to align descriptions in a column
    int spaces = padding - command.length();
    if (spaces < 2)
      spaces = 2;

    fmt::print("{}", std::string(spaces, ' '));
    fmt::print("{}\n", description);
  }

  // Print a subcommand with description in column format
  static void subcommandEntry(
    std::string_view command,
    std::string_view args,
    std::string_view description,
    int cmd_padding = 10,
    int args_padding = 20
  ) {
    fmt::print(fg(config::colors::command_name) | fmt::emphasis::bold, "  {}", command);

    // Calculate padding for command
    int cmd_spaces = cmd_padding - command.length();
    if (cmd_spaces < 2)
      cmd_spaces = 2;
    fmt::print("{}", std::string(cmd_spaces, ' '));

    // Print args with its own padding if provided
    if (!args.empty()) {
      fmt::print(fg(config::colors::option_name), "{}", args);
      int args_spaces = args_padding - args.length();
      if (args_spaces < 2)
        args_spaces = 2;
      fmt::print("{}", std::string(args_spaces, ' '));
    } else {
      fmt::print("{}", std::string(args_padding, ' '));
    }

    fmt::print("{}\n", description);
  }

  /**
   * Print a command with description in column format
   * @param command The command to display
   * @param args The args the command takes
   * @param description Description of what the command does
   */
  static void
    commandEntry(std::string_view command, std::string_view args, std::string_view description) {
    fmt::print(fg(fmt::color::light_salmon) | fmt::emphasis::bold, "  {}", command);

    if (!args.empty()) {
      fmt::print(" ");
      fmt::print(fg(fmt::color::light_sky_blue), "{}", args);
    }

    fmt::print("\n      {}\n", description);
  }

  // Print an example command
  static void exampleCommand(std::string_view example, std::string_view description = "") {
    fmt::print(fg(config::colors::example), "  {}", example);
    if (!description.empty()) {
      fmt::print(" - {}", description);
    }
    fmt::print("\n");
  }

  // Print a success message
  static void success(std::string_view message) {
    auto now = std::chrono::system_clock::now();
    fmt::print(
      fg(config::colors::success) | fmt::emphasis::bold,
      "[{}] {} {}\n",
      fmt::format(config::formats::timestamp, now),
      config::symbols::success,
      message
    );
  }

  // Print an info message
  static void info(std::string_view message) {
    fmt::print(fg(config::colors::info), "{}  {}\n", config::symbols::info, message);
  }

  // Print a warning message
  static void warning(std::string_view message) {
    fmt::print(
      fg(config::colors::warning) | fmt::emphasis::bold,
      "{} {}\n",
      config::symbols::warning,
      message
    );
  }

  // Print an error message
  static void error(std::string_view message) {
    fmt::print(
      fg(config::colors::error) | fmt::emphasis::bold,
      "{} Error: {}\n",
      config::symbols::error,
      message
    );
  }

  // Show a progress animation with optional completion message
  static void showProgress(
    std::string_view action,
    size_t steps = 5,
    std::chrono::milliseconds delay = std::chrono::milliseconds(200),
    std::optional<std::string_view> completion_message = std::nullopt
  ) {
    fmt::print(fg(config::colors::progress), "  {} ", action);
    for (size_t i = 0; i < steps; ++i) {
      fmt::print(".");
      std::cout.flush();
      std::this_thread::sleep_for(delay);
    }

    if (completion_message) {
      fmt::print(" {}!\n", *completion_message);
    } else {
      fmt::print(" Done!\n");
    }
  }

  /**
   * Display a regular message
   */
  static void message(const std::string& message) { fmt::print("{}\n", message); }

  /**
   * Display a section header
   */
  static void header(const std::string& message) {
    fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::light_blue), "\n=== {} ===\n\n", message);
  }

  /**
   * Display a section subheader
   */
  static void subheader(const std::string& message) {
    fmt::print(fmt::emphasis::bold, "\n--- {} ---\n\n", message);
  }

  /**
   * Display a divider line
   */
  static void divider() { fmt::print("----------------------------------------\n"); }

  /**
   * @brief Display plain text without any prefix
   *
   * @param message The text to display
   */
  static void text(const std::string& message) { fmt::print("{}\n", message); }

  /**
   * @brief Display formatted plain text without any prefix
   *
   * @tparam Args Format argument types
   * @param format Format string
   * @param args Format arguments
   */
  template <typename... Args>
  static void text(fmt::format_string<Args...> format, Args&&... args) {
    text(fmt::format(format, std::forward<Args>(args)...));
  }

  /**
   * @brief Print debug box with algorithm input/output using tabulate
   *
   * @tparam InputType Type of input
   * @tparam OutputType Type of output
   * @param title Box title
   * @param input Algorithm input
   * @param output Algorithm output (optional)
   * @param max_items Maximum number of items to show
   * @param width Width of the box
   */
  template <typename InputType, typename OutputType>
  static void debug_box(
    const std::string& title,
    const InputType& input,
    const OutputType* output = nullptr,
    size_t max_items = 50,
    size_t width = 70
  ) {
    // Number of data values to display per row
    const size_t cells_per_row = 8;

    // Create a single main table
    tabulate::Table table;

    // Add title with formatting
    table.add_row({title});
    table[0]
      .format()
      .font_align(tabulate::FontAlign::center)
      .font_style({tabulate::FontStyle::bold})
      .font_color(tabulate::Color::green)
      .width(width);

    // Add input header
    table.add_row({"Input:"});
    table[1][0]
      .format()
      .font_color(tabulate::Color::cyan)
      .font_style({tabulate::FontStyle::bold})
      .font_align(tabulate::FontAlign::left);

    // Process input data
    if constexpr (std::is_same_v<InputType, std::vector<int>> ||
                  std::is_same_v<InputType, std::vector<float>> ||
                  std::is_same_v<InputType, std::vector<double>>) {
      // Calculate how many items to show
      size_t items_to_show = std::min(max_items, input.size());
      size_t rows_needed = (items_to_show + cells_per_row - 1) / cells_per_row;  // Ceiling division

      // Format data rows with commas
      for (size_t row = 0; row < rows_needed; row++) {
        std::string row_data = "  ";
        for (size_t i = 0; i < cells_per_row && (row * cells_per_row + i) < items_to_show; i++) {
          try {
            row_data += std::to_string(input[row * cells_per_row + i]);
            // Add comma if not the last item in the row or vector
            if (i < cells_per_row - 1 && (row * cells_per_row + i + 1) < items_to_show) {
              row_data += ", ";
            }
          } catch (...) {
            row_data += "Error";
            if (i < cells_per_row - 1 && (row * cells_per_row + i + 1) < items_to_show) {
              row_data += ", ";
            }
          }
        }
        table.add_row({row_data});
      }

      // Add truncation message if needed
      if (input.size() > max_items) {
        table.add_row({fmt::format("  ...+{} more items", input.size() - max_items)});
        table[table.size() - 1][0]
          .format()
          .font_color(tabulate::Color::yellow)
          .font_style({tabulate::FontStyle::italic});
      }
    } else {
      table.add_row({"  (Input type not supported for debug view)"});
    }

    // Add output section if available
    if (output) {
      table.add_row({"Output:"});
      table[table.size() - 1][0]
        .format()
        .font_color(tabulate::Color::cyan)
        .font_style({tabulate::FontStyle::bold})
        .font_align(tabulate::FontAlign::left);

      if constexpr (std::is_same_v<OutputType, std::vector<int>> ||
                    std::is_same_v<OutputType, std::vector<float>> ||
                    std::is_same_v<OutputType, std::vector<double>>) {
        // Calculate how many items to show
        size_t items_to_show = std::min(max_items, output->size());
        size_t rows_needed =
          (items_to_show + cells_per_row - 1) / cells_per_row;  // Ceiling division

        // Format data rows with commas
        for (size_t row = 0; row < rows_needed; row++) {
          std::string row_data = "  ";
          for (size_t i = 0; i < cells_per_row && (row * cells_per_row + i) < items_to_show; i++) {
            try {
              row_data += std::to_string((*output)[row * cells_per_row + i]);
              // Add comma if not the last item in the row or vector
              if (i < cells_per_row - 1 && (row * cells_per_row + i + 1) < items_to_show) {
                row_data += ", ";
              }
            } catch (...) {
              row_data += "Error";
              if (i < cells_per_row - 1 && (row * cells_per_row + i + 1) < items_to_show) {
                row_data += ", ";
              }
            }
          }
          table.add_row({row_data});
        }

        // Add truncation message if needed
        if (output->size() > max_items) {
          table.add_row({fmt::format("  ...+{} more items", output->size() - max_items)});
          table[table.size() - 1][0]
            .format()
            .font_color(tabulate::Color::yellow)
            .font_style({tabulate::FontStyle::italic});
        }
      } else {
        table.add_row({"  (Output type not supported for debug view)"});
      }
    }

    // Apply styling to the table for a prettier appearance
    table.format().corner_color(tabulate::Color::blue).border_color(tabulate::Color::blue);

    // Print the table
    std::cout << table << std::endl;
  }
};

}  // namespace daa