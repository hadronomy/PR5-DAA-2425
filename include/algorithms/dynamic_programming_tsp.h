#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "mitata/timer.h"
#include "tsp.h"

// Dynamic Programming TSP Implementation
class DynamicProgrammingTSP : public TSPAlgorithm {
 public:
  Path solve(const Graph<City, double>& graph) override {
    const auto ids = graph.getVertexIds();
    if (ids.size() <= 1)
      return ids;

    // Dynamic programming is feasible for up to about 20-25 cities
    static bool warned = false;
    if (ids.size() > 20 && !warned) {
      UI::warning("DP approach not suitable for graphs with >20 vertices");
      warned = true;
    }

    // Implementation of the Held-Karp algorithm
    // For simplicity, we'll remap the vertex IDs to 0...n-1
    std::vector<size_t> indexToId(ids.size());
    for (int i = 0; i < ids.size(); ++i) {
      indexToId[i] = ids[i];
    }

    int n = static_cast<int>(ids.size());

    // Bit manipulation helpers
    auto contains = [](int subset, int pos) {
      return (subset & (1 << pos)) != 0;
    };
    auto remove = [](int subset, int pos) {
      return subset & ~(1 << pos);
    };

    // Pre-compute all edge costs
    std::vector<std::vector<double>> distance(
      n, std::vector<double>(n, std::numeric_limits<double>::infinity())
    );
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        if (i != j) {
          auto edge = graph.getEdge(indexToId[i], indexToId[j]);
          if (edge) {
            distance[i][j] = edge->data();
          }
        }
      }
    }

    // Initialize DP table
    // dp[S][i] = cost of minimum path visiting all vertices in S and ending at i
    const int numSets = 1 << n;  // 2^n possible subsets
    std::vector<std::vector<double>> dp(
      numSets, std::vector<double>(n, std::numeric_limits<double>::infinity())
    );

    // Initialize the parent/next map to reconstruct the path
    std::vector<std::vector<int>> parent(numSets, std::vector<int>(n, -1));

    // Base case: Start at vertex 0
    dp[1][0] = 0;  // dp[{0}][0] = 0

    // Iterate through all subsets of vertices
    for (int s = 3; s < numSets; s += 2) {  // s starts from 3 because we want subsets containing 0
      if (mitata::timeout::requested()) {
        break;
      }
      if ((s & 1) == 0)
        continue;  // Skip subsets that don't contain vertex 0 (bit 0 is set)

      // For each ending vertex
      for (int i = 1; i < n; i++) {
        if (mitata::timeout::requested()) {
          break;
        }
        if (!contains(s, i))
          continue;  // Skip if i is not in the subset

        // Previous subset without vertex i
        int prev_s = remove(s, i);

        // Try all possible previous vertices
        for (int j = 0; j < n; j++) {
          if (mitata::timeout::requested()) {
            break;
          }
          if (j == i || !contains(prev_s, j))
            continue;

          double cost = dp[prev_s][j] + distance[j][i];
          if (cost < dp[s][i]) {
            dp[s][i] = cost;
            parent[s][i] = j;
          }
        }
      }
    }

    // Find the optimal tour cost and ending vertex
    double minCost = std::numeric_limits<double>::infinity();
    int lastVertex = -1;

    // Try all possible ending vertices
    for (int i = 1; i < n; i++) {
      if (mitata::timeout::requested()) {
        break;
      }
      double cost = dp[numSets - 1][i] + distance[i][0];
      if (cost < minCost) {
        minCost = cost;
        lastVertex = i;
      }
    }

    // Reconstruct the path
    if (lastVertex == -1) {
      // No solution found
      return Path();
    }

    // Reconstruct the path more efficiently
    int mask = numSets - 1;  // All vertices
    int curr = lastVertex;

    std::vector<size_t> path;
    path.reserve(n + 1);  // Pre-allocate to avoid reallocations

    while (curr != 0) {
      if (mitata::timeout::requested()) {
        break;
      }
      path.push_back(indexToId[curr]);
      int next = parent[mask][curr];
      mask = remove(mask, curr);
      curr = next;
    }
    path.push_back(indexToId[0]);

    // Reverse and return
    return Path(path.rbegin(), path.rend());
  }

  std::string name() const override { return "Dynamic Programming"; }

  std::string description() const override {
    return "Uses the Held-Karp algorithm to find the optimal tour";
  }

  std::string timeComplexity() const override { return "O(n²·2ⁿ)"; }
};
REGISTER_ALGORITHM(DynamicProgrammingTSP, "dynamic_programming");