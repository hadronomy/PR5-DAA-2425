#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "location.hh"

// ANSI color codes
enum AnsiColor {
  RESET = 0,
  RED = 31,
  GREEN = 32,
  YELLOW = 33,
  BLUE = 34,
  MAGENTA = 35,
  CYAN = 36,
  WHITE = 37
};

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
  void load_file(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    std::vector<std::string> lines;

    while (std::getline(file, line)) {
      lines.push_back(line);
    }

    file_lines[filename] = std::move(lines);
  }

  CodeSnippet get_snippet(const yy::location& loc, int context_lines = 2) {
    const std::string& filename = *loc.begin.filename;
    auto& lines = file_lines[filename];

    int start_line = std::max(1, (int)loc.begin.line - context_lines);
    int end_line = std::min((int)lines.size(), (int)loc.end.line + context_lines);

    CodeSnippet snippet;
    snippet.error_line_index = loc.begin.line - start_line;
    snippet.error_column = loc.begin.column - 1;
    snippet.error_length =
      (loc.end.line == loc.begin.line) ? (loc.end.column - loc.begin.column) : 1;

    for (int i = start_line; i <= end_line; i++) {
      if (i - 1 < lines.size()) {
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

  std::string color(AnsiColor c) {
    if (!use_colors)
      return "";
    return "\033[" + std::to_string(c) + "m";
  }

  void print_code_snippet(const CodeSnippet& snippet, const yy::location& loc) {
    // Print line numbers and code
    for (size_t i = 0; i < snippet.lines.size(); i++) {
      int line_num = loc.begin.line - snippet.error_line_index + i;

      // Line number margin right-aligned with consistent width
      std::string line_num_str = std::to_string(line_num);
      std::cerr << color(BLUE) << " " << line_num_str << " | " << color(RESET) << snippet.lines[i]
                << "\n";

      // Print the caret line under the error line
      if (i == snippet.error_line_index) {
        // Print margin with same width as line numbers
        std::string margin(line_num_str.length(), ' ');
        std::cerr << color(BLUE) << " " << margin << " | " << color(RESET);

        // Indent to the error position
        for (int j = 0; j < snippet.error_column; j++) {
          std::cerr << " ";
        }

        // Print the carets
        std::cerr << color(RED);
        for (int j = 0; j < snippet.error_length; j++) {
          std::cerr << "^";
        }
        std::cerr << color(RESET) << "\n";

        // Add an empty line with just the margin after error line
        std::cerr << color(BLUE) << " " << margin << " |" << color(RESET) << "\n";
      }
    }
  }

 public:
  DiagnosticEngine(SourceManager& sm) : sources(sm) {}

  void emit_error(
    const yy::location& loc,
    const std::string& message,
    std::vector<std::string> helps = {}
  ) {
    // Store error for later reporting
    errors.push_back({loc, message, helps, DiagnosticLevel::Error});

    // Print immediately for interactive feedback
    print_diagnostic(errors.back());
  }

  void print_diagnostic(const Diagnostic& diag) {
    // Format like rustc
    std::cerr << color(RED) << "error" << color(RESET) << ": " << diag.message << "\n";

    // Print file:line:col information
    if (diag.location.begin.filename) {
      std::cerr << color(BLUE) << " --> " << color(RESET) << *diag.location.begin.filename << ":"
                << diag.location.begin.line << ":" << diag.location.begin.column << "\n";

      // Print code context with caret
      auto snippet = sources.get_snippet(diag.location);
      print_code_snippet(snippet, diag.location);
    }

    // Print help messages
    for (const auto& help : diag.helps) {
      std::cerr << color(GREEN) << "help: " << color(RESET) << help << "\n";
    }

    std::cerr << "\n";
  }

  bool has_errors() const { return !errors.empty(); }
};
