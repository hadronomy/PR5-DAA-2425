#pragma once

#include <memory>
#include <string>
#include <vector>

namespace daa {

// Forward declaration
class VRPTProblem;

class Solution {
 public:
  struct Route {
    std::vector<int> nodes;
    double cost;
  };

  Solution(const VRPTProblem& problem);

  // Add a route to the solution
  void addRoute(const Route& route);

  // Get total cost of the solution
  double getCost() const;

  // Get number of routes
  int getRouteCount() const;

  // Get all routes
  const std::vector<Route>& getRoutes() const;

  // Check if the solution is valid for the problem
  bool isValid() const;

  // Get solution statistics as string
  std::string getStatistics() const;

 private:
  const VRPTProblem& problem_;
  std::vector<Route> routes_;
  double total_cost_;
  bool is_valid_;
};

}  // namespace daa
