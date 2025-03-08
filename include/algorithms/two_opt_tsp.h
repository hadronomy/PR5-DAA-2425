#pragma once

#include <algorithm>

#include "tsp.h"
#include "nearest_neighbor_tsp.h"

// Two-Opt Improvement TSP Implementation
class TwoOptTSP : public TSPAlgorithm {
 public:
  Path solve(const Graph<City, double>& graph) override {
    const auto ids = graph.getVertexIds();
    if (ids.size() <= 1)
      return ids;

    // Start with a greedy solution
    NearestNeighborTSP nearestNeighbor;
    Path path = nearestNeighbor.solve(graph);

    // Apply 2-opt improvement
    bool improvement = true;
    double bestCost = graph.getPathCost(path);

    while (improvement) {
      improvement = false;

      for (size_t i = 0; i < path.size() - 1; ++i) {
        for (size_t j = i + 1; j < path.size(); ++j) {
          Path newPath = path;
          std::reverse(newPath.begin() + i, newPath.begin() + j + 1);

          double newCost = graph.getPathCost(newPath);
          if (newCost < bestCost) {
            bestCost = newCost;
            path = newPath;
            improvement = true;
            break;
          }
        }
        if (improvement)
          break;
      }
    }

    return path;
  }

  std::string name() const override { return "Two-Opt Improvement"; }

  std::string description() const override {
    return "Improves a greedy solution by repeatedly reversing subtours";
  }

  std::string timeComplexity() const override {
    return "O(n²·i) where i is the number of improvements";
  }
};
REGISTER_ALGORITHM(TwoOptTSP, "two_opt");
