#pragma once

#include <random>
#include <string>
#include <vector>

// Forward declarations to avoid circular dependency
class City;
enum class GraphType;

/**
 * @brief Interface for generating test data for algorithms
 *
 * @tparam DataType The type of data to generate
 */
template <typename DataType>
class DataGenerator {
 public:
  virtual ~DataGenerator() = default;
  virtual DataType generate(int size) const = 0;
};

/**
 * @brief Default random integer vector generator
 */
class DefaultVectorIntGenerator : public DataGenerator<std::vector<int>> {
 public:
  std::vector<int> generate(int size) const override {
    std::vector<int> data;
    data.reserve(size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 10000);

    for (int i = 0; i < size; i++) {
      data.push_back(distrib(gen));
    }

    return data;
  }
};

// Remove the binary search and hanoi tower types as they're not needed for TSP

// Include the full definitions after DataGenerator is defined
#include "graph.h"
#include "tsp.h"

/**
 * @brief Generates random TSP problem instances
 */
class RandomTSPGenerator : public DataGenerator<Graph<City, double>> {
 private:
  GraphType graph_type_;

 public:
  explicit RandomTSPGenerator(GraphType type = GraphType::Undirected) : graph_type_(type) {}

  Graph<City, double> generate(int size) const override {
    std::vector<City> cities;
    cities.reserve(size);

    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);

    // Generate random cities
    for (int i = 0; i < size; ++i) {
      cities.emplace_back("City" + std::to_string(i), dis(gen), dis(gen));
    }

    // Create a complete graph with the cities
    return TSPGraphFactory::createComplete(cities, graph_type_);
  }
};
