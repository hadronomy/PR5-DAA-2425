#pragma once

#include <algorithm>
#include <random>
#include "../tsp.h"

// Simulated Annealing TSP Implementation
class SimulatedAnnealingTSP : public TSPAlgorithm {
 public:
  Path solve(const Graph<City, double>& graph) override {
    const auto ids = graph.getVertexIds();
    if (ids.size() <= 1)
      return ids;

    // Parameters for simulated annealing
    double initialTemp = 100.0;
    double finalTemp = 0.01;
    double coolingRate = 0.99;
    int iterationsPerTemp = 100;

    // Start with a random solution
    Path currentPath = ids;
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(currentPath.begin(), currentPath.end(), g);

    Path bestPath = currentPath;
    double currentCost = graph.getPathCost(currentPath);
    double bestCost = currentCost;

    double temp = initialTemp;
    std::uniform_real_distribution<> dis(0.0, 1.0);

    // Main simulated annealing loop
    while (temp > finalTemp) {
      for (int i = 0; i < iterationsPerTemp; ++i) {
        // Generate a neighbor by swapping two random cities
        Path newPath = currentPath;
        int pos1 = dis(g) * newPath.size();
        int pos2 = dis(g) * newPath.size();
        std::swap(newPath[pos1], newPath[pos2]);

        // Calculate the new cost
        double newCost = graph.getPathCost(newPath);

        // Decide whether to accept the new solution
        double deltaE = newCost - currentCost;
        if (deltaE < 0 || dis(g) < std::exp(-deltaE / temp)) {
          currentPath = newPath;
          currentCost = newCost;

          // Update best solution if applicable
          if (currentCost < bestCost) {
            bestPath = currentPath;
            bestCost = currentCost;
          }
        }
      }

      // Cool down
      temp *= coolingRate;
    }

    return bestPath;
  }

  std::string name() const override { return "Simulated Annealing"; }

  std::string description() const override {
    return "A metaheuristic approach that mimics the process of annealing in metallurgy";
  }

  std::string timeComplexity() const override {
    return "O(i·n) where i is the number of iterations";
  }
};
REGISTER_ALGORITHM(SimulatedAnnealingTSP, "simulated_annealing");
