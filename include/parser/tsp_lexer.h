#pragma once

#ifndef yyFlexLexerOnce
#include <FlexLexer.h>
#endif

#include <istream>
#include <string>
#include "tsp_parser.h"

namespace yy {

class tsp_lexer : public yyFlexLexer {
 public:
  tsp_lexer(std::istream* in = nullptr);
  virtual ~tsp_lexer();

  // Get tokens and fill in semantic value
  yy::tsp_parser::symbol_type get_next_token();
  int lex(yy::tsp_parser::semantic_type* yylval, yy::tsp_parser::location_type* yylloc);

  // Set the filename for location tracking
  void set_filename(const std::string& filename);

 private:
  // Keep track of semantic values and locations
  yy::tsp_parser::semantic_type* yylval = nullptr;
  yy::tsp_parser::location_type* yylloc = nullptr;
  yy::tsp_parser::location_type loc;
  std::string current_filename;
};

}  // namespace yy
