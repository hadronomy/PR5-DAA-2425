%{
#include <string>
#include <memory>
#include "vrpt_parser.h"  /* This must be the first include */
#include "parser/vrpt_lexer.h"

#undef YY_DECL
#define YY_DECL yy::vrpt_parser::symbol_type yy::vrpt_lexer::get_next_token()

/* Forward declaration of error function */
void vrpt_error(const yy::location& loc, const std::string& msg);
%}

%option noyywrap nounput noinput batch
%option c++
%option prefix="vrpt_"
%option outfile="vrpt_lexer.cc"
%option yyclass="yy::vrpt_lexer"

/* Update location information */
%{
#define YY_USER_ACTION \
    if (yylloc) { \
        yylloc->columns(yyleng); \
    }
%}

DIGIT      [0-9]
INTEGER    {DIGIT}+
FLOAT      {INTEGER}\.{INTEGER}?|\.{INTEGER}
ID         [A-Za-z][A-Za-z0-9_]*
KEYWORD    L1|L2|num_vehicles|num_zones|Lx|Ly|Q1|Q2|V|Depot|Dumpsite|IF|IF1|epsilon|offset|k
WS         [ \t]+

%%

{KEYWORD}    {
    return yy::vrpt_parser::make_KEYWORD(yytext, loc);
}

{INTEGER}    {
    int val = std::stoi(yytext);
    return yy::vrpt_parser::make_INTEGER(val, loc);
}

{FLOAT}      {
    double val = std::stod(yytext);
    return yy::vrpt_parser::make_FLOAT(val, loc);
}

{ID}         {
    std::string val = yytext;
    return yy::vrpt_parser::make_ID(val, loc);
}

\n           { 
    if (yylloc) {
        yylloc->lines(1);
        yylloc->step();
    }
    loc.lines(1);
    loc.step();
    return yy::vrpt_parser::make_NEWLINE(loc); 
}

{WS}         { 
    if (yylloc) yylloc->step(); 
    loc.step();
    /* Skip whitespace */ 
}

<<EOF>>      { 
    return yy::vrpt_parser::make_END(loc); 
}

.            {
    std::string msg = "Unexpected character: '";
    msg += yytext[0];
    msg += "'";
    vrpt_error(loc, msg);
    return yy::vrpt_parser::make_END(loc); // Return something to avoid potential issues
}

%%

/* Implementation of vrpt_lexer methods */
yy::vrpt_lexer::vrpt_lexer(std::istream* in) : yyFlexLexer(in) {}

yy::vrpt_lexer::~vrpt_lexer() {}

void yy::vrpt_lexer::set_filename(const std::string& filename) {
    current_filename = filename;
    loc.begin.filename = &current_filename;
    loc.end.filename = &current_filename;
}

int yy::vrpt_lexer::lex(yy::vrpt_parser::semantic_type* lval, yy::vrpt_parser::location_type* lloc) {
    yylval = lval;
    yylloc = lloc;
    if (yylloc) {
        yylloc->begin.filename = yylloc->end.filename = &current_filename;
    }
    return yylex();
}

/* Implement vrpt_error function if it wasn't defined elsewhere */
void vrpt_error(const yy::location& loc, const std::string& msg) {
    std::cerr << "Error at " << loc << ": " << msg << std::endl;
}
