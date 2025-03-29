#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "diagnostic.h"
#include "vrpt_lexer.h"
#include "vrpt_parser.h"

// Forward declaration
namespace daa {
class VRPTProblem;
}

class VRPTDriver {
 public:
  VRPTParameters parameters;
  std::vector<LocationDef> locations;
  std::vector<ZoneDef> zones;

  SourceManager source_mgr;
  DiagnosticEngine diagnostic;
  std::unique_ptr<yy::vrpt_lexer> lexer;

  VRPTDriver() : diagnostic(source_mgr) {}

  bool parse_file(const std::string& filename) {
    source_mgr.load_file(filename);

    std::ifstream in_file(filename);
    if (!in_file.good()) {
      std::cerr << "Error: Could not open file " << filename << std::endl;
      return false;
    }

    lexer = std::make_unique<yy::vrpt_lexer>(&in_file);
    lexer->set_filename(filename);

    yy::vrpt_parser parser(*this);
    int result = parser.parse();

    return result == 0 && !diagnostic.has_errors();
  }
};
