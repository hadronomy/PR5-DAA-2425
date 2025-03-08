#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "algorithm_registry.h"
#include "graph.h"

// Useful City class for TSP that works well with our Graph
class City {
 private:
  std::string name_;
  double x_;
  double y_;

 public:
  City() : name_(""), x_(0), y_(0) {}

  City(std::string name, double x, double y) : name_(std::move(name)), x_(x), y_(y) {}

  const std::string& name() const { return name_; }
  double x() const { return x_; }
  double y() const { return y_; }

  // Calculate Euclidean distance between cities
  static double distance(const City& a, const City& b) {
    double dx = a.x_ - b.x_;
    double dy = a.y_ - b.y_;
    return std::sqrt(dx * dx + dy * dy);
  }

  // For serialization
  friend std::ostream& operator<<(std::ostream& os, const City& city) {
    os << city.name_ << " " << city.x_ << " " << city.y_;
    return os;
  }

  friend std::istream& operator>>(std::istream& is, City& city) {
    is >> city.name_ >> city.x_ >> city.y_;
    return is;
  }

  // Make City hashable to satisfy the Hashable concept
  friend struct std::hash<City>;

  // Equality operator for hash tables
  bool operator==(const City& other) const {
    return name_ == other.name_ && std::abs(x_ - other.x_) < 1e-9 && std::abs(y_ - other.y_) < 1e-9;
  }
};

// Implement the hash function for City to satisfy the Hashable concept
namespace std {
template <>
struct hash<City> {
  std::size_t operator()(const City& city) const {
    std::size_t h1 = std::hash<std::string>{}(city.name());
    std::size_t h2 = std::hash<double>{}(city.x());
    std::size_t h3 = std::hash<double>{}(city.y());
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};
}  // namespace std

// TSP graph factory
class TSPGraphFactory {
 public:
  static Graph<City, double>
    createComplete(const std::vector<City>& cities, GraphType type = GraphType::Undirected) {
    Graph<City, double> graph(type);

    // Add all cities as vertices
    std::vector<std::size_t> ids;
    for (const auto& city : cities) {
      ids.push_back(graph.addVertex(city));
    }

    // Create a complete graph
    graph.makeComplete([](const City& a, const City& b) { return City::distance(a, b); });

    return graph;
  }

  static Graph<City, double> fromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      throw std::runtime_error("Could not open file: " + filename);
    }

    std::string header;
    std::getline(file, header);

    GraphType type = (header == "DIRECTED") ? GraphType::Directed : GraphType::Undirected;

    Graph<City, double> graph(type);

    int cityCount;
    file >> cityCount;

    for (int i = 0; i < cityCount; ++i) {
      std::string name;
      double x, y;
      file >> name >> x >> y;
      graph.addVertex(City(name, x, y));
    }

    // Create a complete graph
    graph.makeComplete([](const City& a, const City& b) { return City::distance(a, b); });

    return graph;
  }
};

// Path type for TSP solutions
using Path = std::vector<std::size_t>;

// TSP algorithm interface extends the general Algorithm interface
class TSPAlgorithm : public TypedAlgorithm<Graph<City, double>, Path> {
 public:
  // The solve method fulfills the TypedAlgorithm interface
  virtual Path solve(const Graph<City, double>& graph) override = 0;
};

// Algorithm runner with timing and comparison functionality
class TSPSolver {
 public:
  struct Result {
    std::string algorithmName;
    Path path;
    double pathLength;
    std::chrono::milliseconds duration;
    bool timedOut = false;

    // Optional statistics
    std::unordered_map<std::string, std::string> stats;
  };

  // Visualize a path
  static void printPath(const Path& path, const Graph<City, double>& graph) {
    if (path.empty()) {
      std::cout << "Empty path\n";
      return;
    }

    std::cout << "Path: ";
    for (size_t i = 0; i < path.size(); ++i) {
      const auto* vertex = graph.getVertex(path[i]);
      if (vertex) {
        std::cout << vertex->data().name();
        if (i < path.size() - 1) {
          std::cout << " -> ";
        }
      } else {
        std::cout << "Unknown(" << path[i] << ")";
        if (i < path.size() - 1) {
          std::cout << " -> ";
        }
      }
    }

    // Show returning to start for TSP
    if (path.size() > 1) {
      const auto* first = graph.getVertex(path.front());
      if (first) {
        std::cout << " -> " << first->data().name();
      }
    }

    std::cout << std::endl;
  }

  // Save the best solution to a file
  static void saveBestSolution(
    const std::vector<Result>& results,
    const Graph<City, double>& graph,
    const std::string& filename
  ) {
    if (results.empty()) {
      throw std::runtime_error("No results to save");
    }

    // Find the best solution
    auto best =
      std::min_element(results.begin(), results.end(), [](const Result& a, const Result& b) {
        return a.pathLength < b.pathLength;
      });

    std::ofstream file(filename);
    if (!file) {
      throw std::runtime_error("Could not open file for writing: " + filename);
    }

    file << "# TSP Solution\n";
    file << "Algorithm: " << best->algorithmName << "\n";
    file << "Path Length: " << best->pathLength << "\n";
    file << "Computation Time: " << best->duration.count() << " ms\n";
    file << "Path:\n";

    for (std::size_t id : best->path) {
      const auto* vertex = graph.getVertex(id);
      if (vertex) {
        const auto& city = vertex->data();
        file << id << " " << city.name() << " " << city.x() << " " << city.y() << "\n";
      }
    }

    file.close();
  }

 private:
  // Helper to calculate path length
  static double calculatePathLength(const Path& path, const Graph<City, double>& graph) {
    return graph.getPathCost(path);
  }
};
