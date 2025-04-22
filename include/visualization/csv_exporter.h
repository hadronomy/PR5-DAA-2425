#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "visualization/problem_manager.h"

namespace daa {
namespace visualization {

/**
 * @brief Utility class for exporting benchmark results to CSV format
 */
class CSVExporter {
 public:
  /**
   * @brief Export benchmark results to a CSV file
   * 
   * @param results The benchmark results to export
   * @param filepath The path to save the CSV file
   * @return true if export was successful, false otherwise
   */
  static bool exportToCSV(
    const std::vector<ProblemManager::BenchmarkResult>& results,
    const std::string& filepath
  );

 private:
  /**
   * @brief Convert a single benchmark result to a CSV row
   * 
   * @param result The benchmark result to convert
   * @return std::string The CSV row
   */
  static std::string resultToCSVRow(const ProblemManager::BenchmarkResult& result);

  /**
   * @brief Get the CSV header row
   * 
   * @return std::string The CSV header row
   */
  static std::string getCSVHeader();
};

} // namespace visualization
} // namespace daa
