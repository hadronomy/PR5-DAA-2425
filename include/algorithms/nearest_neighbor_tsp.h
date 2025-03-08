#pragma once

#include "../tsp.h"

// Nearest Neighbor (Greedy) TSP Implementation
class NearestNeighborTSP : public TSPAlgorithm {
 public:
  Path solve(const Graph<City, double>& graph) override {
    if (graph.vertexCount() == 0)
      return {};

    // Start at the first vertex (ID 0)
    const auto ids = graph.getVertexIds();
    if (ids.empty())
      return {};

    // Use the graph's built-in nearest neighbor algorithm
    return graph.getNearestNeighborPath(ids.front());
  }

  std::string name() const override { return "Nearest Neighbor"; }

  std::string description() const override {
    return "A greedy algorithm that always chooses the nearest unvisited city";
  }

  std::string timeComplexity() const override { return "O(n²)"; }
};
REGISTER_ALGORITHM(NearestNeighborTSP, "nearest_neighbor");
