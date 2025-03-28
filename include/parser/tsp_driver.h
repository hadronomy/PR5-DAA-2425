#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "diagnostic.h"
#include "graph.h"
#include "tsp_lexer.h"
#include "tsp_parser.h"

class TSPDriver {
 public:
  daa::Graph<std::string, int> graph;
  SourceManager source_mgr;
  DiagnosticEngine diagnostic;
  std::unique_ptr<yy::tsp_lexer> lexer;
  std::unordered_map<std::string, std::size_t> node_ids;
  int expected_node_count = 0;

  TSPDriver() : graph(daa::GraphType::Undirected), diagnostic(source_mgr) {}

  bool parse_file(const std::string& filename) {
    source_mgr.load_file(filename);

    std::ifstream in_file(filename);
    if (!in_file.good()) {
      std::cerr << "Error: Could not open file " << filename << std::endl;
      return false;
    }

    lexer = std::make_unique<yy::tsp_lexer>(&in_file);
    lexer->set_filename(filename);

    yy::tsp_parser parser(*this);
    int result = parser.parse();

    return result == 0 && !diagnostic.has_errors();
  }

  void add_edge(const EdgeDef& edge) {
    // Get or create node IDs
    std::size_t source_id = get_or_create_node_id(edge.from);
    std::size_t target_id = get_or_create_node_id(edge.to);

    // Add the edge to the graph
    graph.addEdge(source_id, target_id, edge.distance);
  }

  std::size_t get_or_create_node_id(const std::string& node_name) {
    auto it = node_ids.find(node_name);
    if (it != node_ids.end()) {
      return it->second;
    }

    // Create a new node
    std::size_t id = graph.addVertex(node_name);
    node_ids[node_name] = id;

    // Update expected node count if this is a new node
    expected_node_count = node_ids.size();

    return id;
  }

  void validate_graph() {
    // Get all vertex IDs
    std::vector<std::size_t> vertices = graph.getVertexIds();

    // Check if the graph is complete
    for (std::size_t i = 0; i < vertices.size(); i++) {
      for (std::size_t j = 0; j < vertices.size(); j++) {
        if (i != j && !graph.hasEdge(vertices[i], vertices[j])) {
          // Create an error location (simplified, as we don't have a specific location)
          yy::location loc;
          diagnostic.emit_error(
            loc,
            "The graph is not complete - no path found from " + get_node_name(vertices[i]) +
              " to " + get_node_name(vertices[j]),
            {"TSP requires a path between every pair of nodes"}
          );
          return;
        }
      }
    }
  }

  std::string get_node_name(std::size_t id) {
    // Find the node name from the ID
    for (const auto& [name, node_id] : node_ids) {
      if (node_id == id) {
        return name;
      }
    }
    return "Unknown";
  }
};
