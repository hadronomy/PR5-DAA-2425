#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "visualization/problem_manager.h"

namespace daa {
namespace visualization {

/**
 * @brief Utility class for exporting benchmark results to LaTeX format
 */
class LatexExporter {
 public:
  /**
   * @brief Export benchmark results to a LaTeX file
   *
   * @param results The benchmark results to export
   * @param filepath The path to save the LaTeX file
   * @return true if export was successful, false otherwise
   */
  static bool exportToLatex(
    const std::vector<ProblemManager::BenchmarkResult>& results,
    const std::string& filepath
  );

 private:
  /**
   * @brief Convert a single benchmark result to a LaTeX table row
   *
   * @param result The benchmark result to convert
   * @return std::string The LaTeX table row
   */
  static std::string resultToLatexRow(const ProblemManager::BenchmarkResult& result);

  /**
   * @brief Get the LaTeX table header
   *
   * @param algorithm_name The name of the algorithm to display in the table header
   * @return std::string The LaTeX table header
   */
  static std::string getLatexHeader(const std::string& algorithm_name);

  /**
   * @brief Get the LaTeX table footer
   *
   * @return std::string The LaTeX table footer
   */
  static std::string getLatexFooter();

  /**
   * @brief Store the algorithm name for use in the footer
   */
  static std::string algorithm_name_;
};

}  // namespace visualization
}  // namespace daa
