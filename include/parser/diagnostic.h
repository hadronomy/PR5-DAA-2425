#pragma once

#include <fmt/color.h>
#include <fmt/core.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "location.hh"

struct CodeSnippet {
  std::vector<std::string> lines;
  int error_line_index;
  int error_column;
  int error_length;
};

class SourceManager {
 private:
  std::unordered_map<std::string, std::vector<std::string>> file_lines;

 public:
  void load_file(std::string_view filename) {
    std::ifstream file(filename.data());
    if (!file) {
      return;  // Silently fail - the diagnostic system will handle errors
    }

    std::string line;
    std::vector<std::string> lines;

    while (std::getline(file, line)) {
      lines.push_back(line);
    }

    file_lines[std::string(filename)] = std::move(lines);
  }

  CodeSnippet get_snippet(const yy::location& loc, int context_lines = 2) const {
    const std::string& filename = *loc.begin.filename;
    auto it = file_lines.find(filename);
    const auto& lines = (it != file_lines.end()) ? it->second : std::vector<std::string>{};

    int start_line = std::max(1, static_cast<int>(loc.begin.line) - context_lines);
    int end_line =
      std::min(static_cast<int>(lines.size()), static_cast<int>(loc.end.line) + context_lines);

    CodeSnippet snippet;
    snippet.error_line_index = loc.begin.line - start_line;
    snippet.error_column = loc.begin.column - 1;
    snippet.error_length =
      (loc.end.line == loc.begin.line) ? (loc.end.column - loc.begin.column) : 1;

    for (int i = start_line; i <= end_line; i++) {
      if (i - 1 >= 0 && static_cast<size_t>(i - 1) < lines.size()) {
        snippet.lines.push_back(lines[i - 1]);
      }
    }

    return snippet;
  }
};

enum class DiagnosticLevel { Error, Warning, Note };

struct Diagnostic {
  yy::location location;
  std::string message;
  std::vector<std::string> helps;
  DiagnosticLevel level;
};

class DiagnosticEngine {
 private:
  std::vector<Diagnostic> errors;
  SourceManager& sources;
  bool use_colors = true;

  fmt::text_style get_error_style() const {
    return use_colors ? fmt::fg(fmt::terminal_color::red) : fmt::text_style{};
  }

  fmt::text_style get_warning_style() const {
    return use_colors ? fmt::fg(fmt::terminal_color::yellow) : fmt::text_style{};
  }

  fmt::text_style get_note_style() const {
    return use_colors ? fmt::fg(fmt::terminal_color::blue) : fmt::text_style{};
  }

  fmt::text_style get_help_style() const {
    return use_colors ? fmt::fg(fmt::terminal_color::green) : fmt::text_style{};
  }

  fmt::text_style get_muted_style() const {
    return use_colors ? fmt::fg(fmt::terminal_color::bright_black) : fmt::text_style{};
  }

  int calculate_line_num_width(const CodeSnippet& snippet, const yy::location& loc) const {
    // Calculate the maximum width needed for line numbers
    int max_line_num = loc.begin.line - snippet.error_line_index + snippet.lines.size();
    return std::max(2, static_cast<int>(std::log10(max_line_num)) + 1);
  }

  void print_code_snippet(const CodeSnippet& snippet, const yy::location& loc) const {
    if (snippet.lines.empty())
      return;

    // Calculate width needed for line numbers
    int line_num_width = calculate_line_num_width(snippet, loc);
    auto line_num_style = get_note_style();

    // Print line numbers and code
    for (size_t i = 0; i < snippet.lines.size(); i++) {
      int line_num = loc.begin.line - snippet.error_line_index + i;
      bool is_error_line = static_cast<int>(i) == snippet.error_line_index;

      // Format code with muted style for non-error lines
      auto code_style = is_error_line ? fmt::text_style{} : get_muted_style();

      // Print line number with consistent padding and colored bar
      fmt::print(
        stderr,
        "{}{} {} {}\n",
        fmt::styled(fmt::format("{:>{}}", line_num, line_num_width), line_num_style),
        fmt::styled("", line_num_style),
        fmt::styled("│", line_num_style),
        fmt::styled(snippet.lines[i], code_style)
      );

      // Print the caret line under the error line
      if (is_error_line) {
        // Pointer line with margin
        fmt::print(
          stderr,
          "{}{} ",
          fmt::styled(fmt::format("{:{}}", "", line_num_width + 1), line_num_style),
          fmt::styled("│", line_num_style)
        );

        // Indent to the error position
        fmt::print(stderr, "{}", std::string(snippet.error_column, ' '));

        // Print the carets
        std::string carets(snippet.error_length + 1, '^');
        fmt::print(
          stderr,
          "{}{}\n",
          fmt::styled(carets, get_error_style()),
          fmt::styled("→ here", get_warning_style())
        );

        // Add an empty line after error indication
        fmt::print(
          stderr,
          "{}{}\n",
          fmt::styled(fmt::format("{:{}}", "", line_num_width + 1), line_num_style),
          fmt::styled("│", line_num_style)
        );
      }
    }
  }

 public:
  explicit DiagnosticEngine(SourceManager& sm) : sources(sm) {}

  void set_color_output(bool enable) { use_colors = enable; }

  void emit_error(
    const yy::location& loc,
    std::string_view message,
    std::vector<std::string> helps = {}
  ) {
    // Store error for later reporting
    errors.push_back({loc, std::string(message), std::move(helps), DiagnosticLevel::Error});

    // Print immediately for interactive feedback
    print_diagnostic(errors.back());
  }

  void emit_warning(
    const yy::location& loc,
    std::string_view message,
    std::vector<std::string> helps = {}
  ) {
    errors.push_back({loc, std::string(message), std::move(helps), DiagnosticLevel::Warning});
    print_diagnostic(errors.back());
  }

  void print_diagnostic(const Diagnostic& diag) const {
    // Print diagnostic label based on level
    if (diag.level == DiagnosticLevel::Error) {
      fmt::print(stderr, "{}: {}\n", fmt::styled("error", get_error_style()), diag.message);
    } else if (diag.level == DiagnosticLevel::Warning) {
      fmt::print(stderr, "{}: {}\n", fmt::styled("warning", get_warning_style()), diag.message);
    } else {
      fmt::print(stderr, "{}: {}\n", fmt::styled("note", get_note_style()), diag.message);
    }

    // Print file:line:col information
    if (diag.location.begin.filename) {
      fmt::print(
        stderr,
        "{} {}:{}:{}\n",
        fmt::styled(" --> ", get_note_style()),
        *diag.location.begin.filename,
        diag.location.begin.line,
        diag.location.begin.column
      );

      // Print code context with caret
      auto snippet = sources.get_snippet(diag.location);
      print_code_snippet(snippet, diag.location);
    }

    // Print help messages
    for (const auto& help : diag.helps) {
      fmt::print(stderr, "{}: {}\n", fmt::styled("help", get_help_style()), help);
    }

    fmt::print(stderr, "\n");
  }

  bool has_errors() const { return !errors.empty(); }

  // Get all the collected diagnostics
  const std::vector<Diagnostic>& get_diagnostics() const { return errors; }
};
