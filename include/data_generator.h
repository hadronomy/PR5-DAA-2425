#pragma once

#include <random>
#include <vector>

namespace daa {

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

}  // namespace daa