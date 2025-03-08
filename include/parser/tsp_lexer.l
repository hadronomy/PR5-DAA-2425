%{
#include <string>
#include <memory>
#include "tsp_parser.h"  /* This must be the first include */
#include "parser/tsp_lexer.h"

#undef YY_DECL
#define YY_DECL yy::tsp_parser::symbol_type yy::tsp_lexer::get_next_token()

/* Forward declaration of error function */
void tsp_error(const yy::location& loc, const std::string& msg);
%}

%option noyywrap nounput noinput batch
%option c++
%option prefix="tsp_"
%option outfile="tsp_lexer.cc"
%option yyclass="yy::tsp_lexer"

/* Update location information */
%{
#define YY_USER_ACTION \
    if (yylloc) { \
        yylloc->columns(yyleng); \
    }
%}

DIGIT    [0-9]
INTEGER  {DIGIT}+
NODEID   [A-Za-z][A-Za-z0-9_]*
WS       [ \t]+

%%

{INTEGER}  {
    int val = std::stoi(yytext);
    return yy::tsp_parser::make_NUMBER(val, loc);
}

{NODEID}   {
    std::string val = yytext;
    return yy::tsp_parser::make_NODE_ID(val, loc);
}

\n         { 
    if (yylloc) {
        yylloc->lines(1);
        yylloc->step();
    }
    loc.lines(1);
    loc.step();
    return yy::tsp_parser::make_NEWLINE(loc); 
}

{WS}       { 
    if (yylloc) yylloc->step(); 
    loc.step();
    /* Skip whitespace */ 
}

<<EOF>>    { 
    return yy::tsp_parser::make_END(loc); 
}

.          {
    std::string msg = "Unexpected character: '";
    msg += yytext[0];
    msg += "'";
    tsp_error(loc, msg);
    return yy::tsp_parser::make_END(loc); // Return something to avoid potential issues
}

%%

/* Implementation of tsp_lexer methods */
yy::tsp_lexer::tsp_lexer(std::istream* in) : yyFlexLexer(in) {}

yy::tsp_lexer::~tsp_lexer() {}

void yy::tsp_lexer::set_filename(const std::string& filename) {
    current_filename = filename;
    loc.begin.filename = &current_filename;
    loc.end.filename = &current_filename;
}

int yy::tsp_lexer::lex(yy::tsp_parser::semantic_type* lval, yy::tsp_parser::location_type* lloc) {
    yylval = lval;
    yylloc = lloc;
    if (yylloc) {
        yylloc->begin.filename = yylloc->end.filename = &current_filename;
    }
    return yylex();
}

/* Implement tsp_error function if it wasn't defined elsewhere */
void tsp_error(const yy::location& loc, const std::string& msg) {
    std::cerr << "Error at " << loc << ": " << msg << std::endl;
}