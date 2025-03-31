%require "3.7"
%language "c++"
%defines "vrpt_parser.h"
%output "vrpt_parser.cc"

%locations
%define api.namespace {yy}
%define api.parser.class {vrpt_parser}
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
    class VRPTDriver;
    
    // Structure to hold problem parameters
    struct VRPTParameters {
        double l1 = 0.0;            // L1: Max duration for collection vehicles (minutes)
        double l2 = 0.0;            // L2: Max duration for transportation vehicles (minutes)
        int num_vehicles = 0;       // Number of collection vehicles
        int num_zones = 0;          // Number of collection zones
        double map_width = 0.0;     // Map width (Lx)
        double map_height = 0.0;    // Map height (Ly)
        double q1 = 0.0;            // Q1: Collection vehicle capacity
        double q2 = 0.0;            // Q2: Transportation vehicle capacity
        double vehicle_speed = 0.0; // V: Vehicle speed (km/h)
        double epsilon = 0.0;       // Epsilon parameter
        double offset = 0.0;        // Offset parameter
        int k_param = 0;            // k parameter
    };

    // Structure to hold a location definition
    struct LocationDef {
        std::string type;  // "Depot", "Dumpsite", "IF", "IF1"
        double x;
        double y;
    };

    // Structure to hold a zone definition
    struct ZoneDef {
        int id;
        double x;
        double y;
        double service_time;
        double waste_amount;
    };
}

%parse-param { VRPTDriver& driver }

%code {
    #include "parser/vrpt_driver.h"

    #define yylex driver.lexer->get_next_token
}

%token <std::string> KEYWORD ID
%token <int> INTEGER
%token <double> FLOAT
%token NEWLINE
%token END 0 "end of file"

%type <LocationDef> location
%type <ZoneDef> zone
%type <std::vector<ZoneDef>> zone_list

%%

start:
  entry_list zone_list {
    // Validate that we have the expected number of zones
    if (driver.parameters.num_zones != $2.size()) {
        driver.diagnostic.emit_error(@2, 
            "Number of zones (" + std::to_string($2.size()) + 
            ") doesn't match the specified parameter num_zones (" + 
            std::to_string(driver.parameters.num_zones) + ")");
    }
  }
;

entry_list:
  entry
| entry_list entry
;

entry:
  parameter
| location
;

parameter:
  KEYWORD INTEGER NEWLINE {
    std::string key = $1;
    int value = $2;
    
    if (key == "num_vehicles") {
        driver.parameters.num_vehicles = value;
    } else if (key == "num_zones") {
        driver.parameters.num_zones = value;
    } else if (key == "k") {
        driver.parameters.k_param = value;
    } else if (key == "L1") {
        driver.parameters.l1 = static_cast<double>(value);
    } else if (key == "L2") {
        driver.parameters.l2 = static_cast<double>(value);
    } else if (key == "Lx") {
        driver.parameters.map_width = static_cast<double>(value);
    } else if (key == "Ly") {
        driver.parameters.map_height = static_cast<double>(value);
    } else if (key == "Q1") {
        driver.parameters.q1 = static_cast<double>(value);
    } else if (key == "Q2") {
        driver.parameters.q2 = static_cast<double>(value);
    } else if (key == "V") {
        driver.parameters.vehicle_speed = static_cast<double>(value);
    } else if (key == "epsilon") {
        driver.parameters.epsilon = static_cast<double>(value);
    } else if (key == "offset") {
        driver.parameters.offset = static_cast<double>(value);
    } else {
        driver.diagnostic.emit_error(@1, "Integer parameter " + key + " not recognized");
    }
  }
| KEYWORD FLOAT NEWLINE {
    std::string key = $1;
    double value = $2;
    
    if (key == "L1") {
        driver.parameters.l1 = value;
    } else if (key == "L2") {
        driver.parameters.l2 = value;
    } else if (key == "Lx") {
        driver.parameters.map_width = value;
    } else if (key == "Ly") {
        driver.parameters.map_height = value;
    } else if (key == "Q1") {
        driver.parameters.q1 = value;
    } else if (key == "Q2") {
        driver.parameters.q2 = value;
    } else if (key == "V") {
        driver.parameters.vehicle_speed = value;
    } else if (key == "epsilon") {
        driver.parameters.epsilon = value;
    } else if (key == "offset") {
        driver.parameters.offset = value;
    } else {
        driver.diagnostic.emit_error(@1, "Float parameter " + key + " not recognized");
    }
  }
;

location:
  KEYWORD INTEGER INTEGER NEWLINE {
    std::string type = $1;
    if (type == "Depot" || type == "Dumpsite" || type == "IF" || type == "IF1") {
        $$ = LocationDef{type, static_cast<double>($2), static_cast<double>($3)};
        driver.locations.push_back($$);
    } else {
        driver.diagnostic.emit_error(@1, "Location type " + type + " not recognized");
        $$ = LocationDef{"Unknown", static_cast<double>($2), static_cast<double>($3)}; // Return something to avoid undefined behavior
    }
  }
| KEYWORD FLOAT INTEGER NEWLINE {
    std::string type = $1;
    if (type == "Depot" || type == "Dumpsite" || type == "IF" || type == "IF1") {
        $$ = LocationDef{type, $2, static_cast<double>($3)};
        driver.locations.push_back($$);
    } else {
        driver.diagnostic.emit_error(@1, "Location type " + type + " not recognized");
        $$ = LocationDef{"Unknown", $2, static_cast<double>($3)}; // Return something to avoid undefined behavior
    }
  }
| KEYWORD INTEGER FLOAT NEWLINE {
    std::string type = $1;
    if (type == "Depot" || type == "Dumpsite" || type == "IF" || type == "IF1") {
        $$ = LocationDef{type, static_cast<double>($2), $3};
        driver.locations.push_back($$);
    } else {
        driver.diagnostic.emit_error(@1, "Location type " + type + " not recognized");
        $$ = LocationDef{"Unknown", static_cast<double>($2), $3}; // Return something to avoid undefined behavior
    }
  }
| KEYWORD FLOAT FLOAT NEWLINE {
    std::string type = $1;
    if (type == "Depot" || type == "Dumpsite" || type == "IF" || type == "IF1") {
        $$ = LocationDef{type, $2, $3};
        driver.locations.push_back($$);
    } else {
        driver.diagnostic.emit_error(@1, "Location type " + type + " not recognized");
        $$ = LocationDef{"Unknown", $2, $3}; // Return something to avoid undefined behavior
    }
  }
;

zone_list:
  zone {
    $$ = std::vector<ZoneDef>{$1};
  }
| zone_list zone {
    $1.push_back($2);
    $$ = $1;
  }
;

zone:
  INTEGER FLOAT FLOAT FLOAT FLOAT NEWLINE {
    $$ = ZoneDef{$1, $2, $3, $4, $5};
    driver.zones.push_back($$);
  }
| INTEGER INTEGER FLOAT FLOAT FLOAT NEWLINE {
    $$ = ZoneDef{$1, static_cast<double>($2), $3, $4, $5};
    driver.zones.push_back($$);
  }
| INTEGER FLOAT INTEGER FLOAT FLOAT NEWLINE {
    $$ = ZoneDef{$1, $2, static_cast<double>($3), $4, $5};
    driver.zones.push_back($$);
  }
| INTEGER INTEGER INTEGER FLOAT FLOAT NEWLINE {
    $$ = ZoneDef{$1, static_cast<double>($2), static_cast<double>($3), $4, $5};
    driver.zones.push_back($$);
  }
| INTEGER FLOAT FLOAT INTEGER FLOAT NEWLINE {
    $$ = ZoneDef{$1, $2, $3, static_cast<double>($4), $5};
    driver.zones.push_back($$);
  }
| INTEGER FLOAT FLOAT FLOAT INTEGER NEWLINE {
    $$ = ZoneDef{$1, $2, $3, $4, static_cast<double>($5)};
    driver.zones.push_back($$);
  }
| INTEGER INTEGER INTEGER INTEGER FLOAT NEWLINE {
    $$ = ZoneDef{$1, static_cast<double>($2), static_cast<double>($3), static_cast<double>($4), $5};
    driver.zones.push_back($$);
  }
| INTEGER INTEGER INTEGER INTEGER INTEGER NEWLINE {
    $$ = ZoneDef{$1, static_cast<double>($2), static_cast<double>($3), static_cast<double>($4), static_cast<double>($5)};
    driver.zones.push_back($$);
  }
| INTEGER FLOAT FLOAT FLOAT FLOAT %prec END {
    $$ = ZoneDef{$1, $2, $3, $4, $5};
    driver.zones.push_back($$);
  }
| INTEGER INTEGER FLOAT FLOAT FLOAT %prec END {
    $$ = ZoneDef{$1, static_cast<double>($2), $3, $4, $5};
    driver.zones.push_back($$);
  }
| INTEGER FLOAT INTEGER FLOAT FLOAT %prec END {
    $$ = ZoneDef{$1, $2, static_cast<double>($3), $4, $5};
    driver.zones.push_back($$);
  }
| INTEGER INTEGER INTEGER FLOAT FLOAT %prec END {
    $$ = ZoneDef{$1, static_cast<double>($2), static_cast<double>($3), $4, $5};
    driver.zones.push_back($$);
  }
| INTEGER FLOAT FLOAT INTEGER FLOAT %prec END {
    $$ = ZoneDef{$1, $2, $3, static_cast<double>($4), $5};
    driver.zones.push_back($$);
  }
| INTEGER FLOAT FLOAT FLOAT INTEGER %prec END {
    $$ = ZoneDef{$1, $2, $3, $4, static_cast<double>($5)};
    driver.zones.push_back($$);
  }
| INTEGER INTEGER INTEGER INTEGER FLOAT %prec END {
    $$ = ZoneDef{$1, static_cast<double>($2), static_cast<double>($3), static_cast<double>($4), $5};
    driver.zones.push_back($$);
  }
| INTEGER INTEGER INTEGER INTEGER INTEGER %prec END {
    $$ = ZoneDef{$1, static_cast<double>($2), static_cast<double>($3), static_cast<double>($4), static_cast<double>($5)};
    driver.zones.push_back($$);
  }
;

%%

void yy::vrpt_parser::error(const location_type& loc, const std::string& msg) {
    driver.diagnostic.emit_error(loc, msg);
}
