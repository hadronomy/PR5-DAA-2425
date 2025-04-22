#include "visualization/latex_exporter.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace daa {
namespace visualization {

// Initialize static member
std::string LatexExporter::algorithm_name_ = "GRASP";

bool LatexExporter::exportToLatex(
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

    // Group results by instance and algorithm
    std::unordered_map<std::string, std::vector<ProblemManager::BenchmarkResult>> grouped_results;
    std::string algorithm_name;

    // Group results by instance name
    for (const auto& result : results) {
      grouped_results[result.instance_name].push_back(result);
      algorithm_name =
        result
          .algorithm_name;  // Store algorithm name (assuming all results use the same algorithm)
    }

    // Store algorithm name for use in footer
    algorithm_name_ = algorithm_name;

    // Write LaTeX table header
    file << getLatexHeader(algorithm_name);

    // Sort instance names to ensure consistent order
    std::vector<std::string> instance_names;
    for (const auto& [name, _] : grouped_results) {
      instance_names.push_back(name);
    }
    std::sort(instance_names.begin(), instance_names.end());

    // Write each instance's results
    for (const auto& instance_name : instance_names) {
      const auto& instance_results = grouped_results[instance_name];

      // Sort results by num_zones and run_number
      std::vector<ProblemManager::BenchmarkResult> sorted_results = instance_results;
      std::sort(
        sorted_results.begin(),
        sorted_results.end(),
        [](const ProblemManager::BenchmarkResult& a, const ProblemManager::BenchmarkResult& b) {
          if (a.num_zones != b.num_zones)
            return a.num_zones < b.num_zones;
          return a.run_number < b.run_number;
        }
      );

      // Write each result as a row
      for (const auto& result : sorted_results) {
        file << resultToLatexRow(result) << "\n";
      }
    }

    // Add a row of dots to indicate more instances
    file
      << "      $\\cdots$ &$\\cdots$ &$\\cdots$ &$\\cdots$ &$\\cdots$ &$\\cdots$ &$\\cdots$ \\\\\n";

    // Calculate and write averages
    if (results.size() > 1) {
      double avg_cv = 0.0, avg_tv = 0.0, avg_time = 0.0;
      int count = 0;

      for (const auto& result : results) {
        avg_cv += result.cv_count;
        avg_tv += result.tv_count;
        avg_time += result.cpu_time_ms;
        count++;
      }

      if (count > 0) {
        avg_cv /= count;
        avg_tv /= count;
        avg_time /= count;
      }

      // Write averages row
      file << "      $average$   &     &    &       &  " << std::fixed << std::setprecision(2)
           << avg_cv << " & " << std::fixed << std::setprecision(2) << avg_tv << " & " << std::fixed
           << std::setprecision(2) << avg_time << " \\\\ \n";
    }

    // Write LaTeX table footer
    file << getLatexFooter();

    file.close();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error exporting benchmark results to LaTeX: " << e.what() << std::endl;
    return false;
  }
}

std::string LatexExporter::resultToLatexRow(const ProblemManager::BenchmarkResult& result) {
  std::stringstream ss;

  // Format the instance name without .txt extension
  std::string instance_name = result.instance_name;
  size_t dot_pos = instance_name.find(".txt");
  if (dot_pos != std::string::npos) {
    instance_name = instance_name.substr(0, dot_pos);
  }

  // Add all fields to the LaTeX row
  // Use run_number for both LRC and Ejecución columns
  ss << "      $" << instance_name << "$   &  " << result.num_zones << "   &  " << result.run_number
     << "  &   " << result.run_number << "    &  " << result.cv_count << " & " << result.tv_count
     << " & " << std::fixed << std::setprecision(2) << result.cpu_time_ms << " \\\\";

  return ss.str();
}

std::string LatexExporter::getLatexHeader(const std::string& algorithm_name) {
  std::stringstream ss;

  ss << "   \\begin{table}[h!]\n";
  ss << "   {\\small\n";
  ss << "   \\begin{center}\n";
  ss << "   \\begin{tabular}{ccccccc}\n";
  ss << "      \\multicolumn{7}{c}{" << algorithm_name << "} \\\\\n";
  ss << "      \\hline\n";
  ss << "      $Instance$ & $\\#Zonas$ & $|LRC|$ & $Ejecución$ &  $\\#CV$ & $\\#TV$ & $CPU\\_Time$ "
        "\\\\\n";
  ss << "      \\hline\n";

  return ss.str();
}

std::string LatexExporter::getLatexFooter() {
  std::stringstream ss;

  ss << "      \\hline\n";
  ss << "   \\end{tabular}\n";
  ss << "   \\end{center}\n";
  ss << "   }\n";
  // Use the algorithm name in the caption
  ss << "   \\caption{" << algorithm_name_ << ". Tabla de resultados}\n";
  ss << "   \\end{table}\n";

  return ss.str();
}

}  // namespace visualization
}  // namespace daa
