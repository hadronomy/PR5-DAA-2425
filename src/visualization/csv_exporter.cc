#include "visualization/csv_exporter.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace daa {
namespace visualization {

bool CSVExporter::exportToCSV(
  const std::vector<ProblemManager::BenchmarkResult>& results,
  const std::string& filepath
) {
  if (results.empty()) {
    std::cerr << "No benchmark results to export" << std::endl;
    return false;
  }

  try {
    // Create output file
    std::ofstream file(filepath);
    if (!file.is_open()) {
      std::cerr << "Failed to open file for writing: " << filepath << std::endl;
      return false;
    }

    // Write CSV header
    file << getCSVHeader() << std::endl;

    // Write each result as a row
    for (const auto& result : results) {
      file << resultToCSVRow(result) << std::endl;
    }

    // Calculate and write averages
    if (results.size() > 1) {
      double avg_cv = 0.0, avg_tv = 0.0, avg_zones_visited = 0.0;
      double avg_duration = 0.0, avg_waste = 0.0, avg_time = 0.0;
      int count = 0;

      for (const auto& result : results) {
        avg_cv += result.cv_count;
        avg_tv += result.tv_count;
        avg_zones_visited += result.zones_visited;
        avg_duration += result.total_duration;
        avg_waste += result.total_waste;
        avg_time += result.cpu_time_ms;
        count++;
      }

      if (count > 0) {
        avg_cv /= count;
        avg_tv /= count;
        avg_zones_visited /= count;
        avg_duration /= count;
        avg_waste /= count;
        avg_time /= count;
      }

      // Write averages row
      file << "AVERAGE,AVERAGE,,";

      file << std::fixed << std::setprecision(2) << avg_cv << ",";
      file << std::fixed << std::setprecision(2) << avg_tv << ",";
      file << std::fixed << std::setprecision(2) << avg_zones_visited << ",";
      file << std::fixed << std::setprecision(2) << avg_duration << ",";
      file << std::fixed << std::setprecision(2) << avg_waste << ",";
      file << std::fixed << std::setprecision(2) << avg_time << std::endl;
    }

    file.close();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error exporting benchmark results to CSV: " << e.what() << std::endl;
    return false;
  }
}

std::string CSVExporter::resultToCSVRow(const ProblemManager::BenchmarkResult& result) {
  std::stringstream ss;

  // Helper function to escape strings with commas
  auto escapeString = [](const std::string& str) -> std::string {
    if (str.find(',') != std::string::npos) {
      return '"' + str + '"';
    }
    return str;
  };

  // Add all fields to the CSV row
  ss << escapeString(result.instance_name) << ",";
  ss << escapeString(result.algorithm_name) << ",";
  ss << result.num_zones << ",";
  ss << result.run_number << ",";
  ss << result.cv_count << ",";
  ss << result.tv_count << ",";
  ss << result.zones_visited << ",";
  ss << std::fixed << std::setprecision(2) << result.total_duration << ",";
  ss << std::fixed << std::setprecision(2) << result.total_waste << ",";
  ss << std::fixed << std::setprecision(2) << result.cpu_time_ms;

  return ss.str();
}

std::string CSVExporter::getCSVHeader() {
  return "Instance,Algorithm,Total Zones,Run,CV Count,TV Count,Zones Visited,Total Duration,"
         "Total Waste,CPU Time (ms)";
}

}  // namespace visualization
}  // namespace daa
