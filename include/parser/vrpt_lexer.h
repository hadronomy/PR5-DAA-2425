#pragma once

#ifndef yyFlexLexerOnce
#include <FlexLexer.h>
#endif

#include <istream>
#include <string>

#include "vrpt_parser.h"

namespace yy {

class vrpt_lexer : public yyFlexLexer {
 public:
  vrpt_lexer(std::istream* in = nullptr);
  virtual ~vrpt_lexer();

  // Get tokens and fill in semantic value
  yy::vrpt_parser::symbol_type get_next_token();
  int lex(yy::vrpt_parser::semantic_type* yylval, yy::vrpt_parser::location_type* yylloc);

  // Set the filename for location tracking
  void set_filename(const std::string& filename);

 private:
  // Keep track of semantic values and locations
  yy::vrpt_parser::semantic_type* yylval = nullptr;
  yy::vrpt_parser::location_type* yylloc = nullptr;
  yy::vrpt_parser::location_type loc;
  std::string current_filename;
};

}  // namespace yy
