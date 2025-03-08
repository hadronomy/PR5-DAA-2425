#pragma once

#include <algorithm>

#include "mitata/timer.h"
#include "tsp.h"

// Brute Force (Exact) TSP Implementation
class BruteForceTSP : public TSPAlgorithm {
 public:
  Path solve(const Graph<City, double>& graph) override {
    const auto ids = graph.getVertexIds();
    if (ids.empty())
      return {};

    // For larger graphs, abort with a warning
    static bool warned = false;
    if (ids.size() > 11 && !warned) {
      UI::warning("Brute force approach not suitable for graphs with >11 vertices");
      warned = true;
    }

    // Start with vertex IDs in order
    Path bestPath = ids;
    Path currentPath = ids;
    double bestCost = graph.getPathCost(bestPath);

    // Try all permutations
    while (std::next_permutation(currentPath.begin(), currentPath.end())) {
      if (mitata::timeout::requested())
        break;
      double currentCost = graph.getPathCost(currentPath);
      if (currentCost < bestCost) {
        bestCost = currentCost;
        bestPath = currentPath;
      }
    }

    return bestPath;
  }

  std::string name() const override { return "Brute Force"; }

  std::string description() const override {
    return "Examines all possible permutations to find the optimal tour";
  }

  std::string timeComplexity() const override { return "O(n!)"; }
};
REGISTER_ALGORITHM(BruteForceTSP, "brute_force");
