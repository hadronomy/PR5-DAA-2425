%require "3.7"
%language "c++"
%defines "tsp_parser.h"
%output "tsp_parser.cc"

%locations
%define api.namespace {yy}
%define api.parser.class {tsp_parser}
%define api.value.type variant
%define api.token.constructor
%define parse.error detailed
%define parse.trace

%code requires {
    #include <string>
    #include <vector>
    #include <iostream>
    #include <unordered_map>
    
    // Forward declaration
    class TSPDriver;
    
    // Structure to hold an edge definition
    struct EdgeDef {
        std::string from;
        std::string to;
        int distance;
    };
}


%parse-param { TSPDriver& driver }

%code {
    #include "parser/tsp_driver.h"

    #define yylex driver.lexer->get_next_token
}

%token <int> NUMBER
%token <std::string> NODE_ID
%token NEWLINE
%token END 0 "end of file"

%type <EdgeDef> edge
%type <std::vector<EdgeDef>> edge_list

%%

start:
  NUMBER NEWLINE edge_list {
    if ($1 <= 0) {
        driver.diagnostic.emit_error(@1, "Number of nodes must be positive");
    } else if (driver.expected_node_count > 0 && $1 != driver.expected_node_count) {
        driver.diagnostic.emit_error(@1, 
            "Number of nodes (" + std::to_string($1) + ") doesn't match the actual nodes in the graph (" + 
            std::to_string(driver.expected_node_count) + ")",
            {"Check that all nodes are referenced in the edges"});
    }
    
    // Verify graph is complete
    driver.validate_graph();
  }
;

edge_list:
  edge { 
    driver.add_edge($1);
    $$ = std::vector<EdgeDef>{$1}; 
  }
| edge_list edge { 
    driver.add_edge($2);
    $1.push_back($2); 
    $$ = $1; 
  }
| edge_list error NEWLINE {
    driver.diagnostic.emit_error(@2, "Invalid edge definition");
    $$ = $1; // Continue with what we have
  }
;

edge: 
  NODE_ID NODE_ID NUMBER NEWLINE {
    if ($3 < 0) {
        driver.diagnostic.emit_error(@3, "Distance cannot be negative", 
            {"TSP requires non-negative distances between nodes"});
        $$ = EdgeDef{$1, $2, 0}; // Use 0 to recover
    } else {
        $$ = EdgeDef{$1, $2, $3};
    }
  }
| NODE_ID NODE_ID NUMBER error {
    driver.diagnostic.emit_error(@4, "Expected newline after edge definition");
    $$ = EdgeDef{$1, $2, $3}; // Try to recover
  }
| NODE_ID NODE_ID error NEWLINE {
    driver.diagnostic.emit_error(@3, "Expected a distance value");
    $$ = EdgeDef{$1, $2, 0}; // Use placeholder to recover
  }
;

%%

void yy::tsp_parser::error(const location_type& loc, const std::string& msg) {
    driver.diagnostic.emit_error(loc, msg);
}
