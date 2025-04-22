#pragma once

#include <atomic>
#include <chrono>
#include <expected>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <stop_token>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "algorithms/vrpt_solution.h"
#include "problem/vrpt_problem.h"
#include "raylib.h"

#include "algorithm_registry.h"

namespace daa {
namespace visualization {

class ProblemManager {
 public:
  ProblemManager();

  // Define a callback type for problem loading notification
  using ProblemLoadedCallback = std::function<void(const Rectangle&)>;

  // Define a callback type for solution changes
  using SolutionChangedCallback = std::function<void()>;

  // Set callback to be called when a problem is loaded
  void SetProblemLoadedCallback(ProblemLoadedCallback callback) {
    problem_loaded_callback_ = callback;
  }

  // Set callback to be called when a solution is cleared or changed
  void SetSolutionChangedCallback(SolutionChangedCallback callback) {
    solution_changed_callback_ = callback;
  }

  // Scan a directory for problem files
  bool scanDirectory(const std::string& dir_path);

  // Load a problem by filename
  bool loadProblem(const std::string& filename);

  // Get the currently active problem
  VRPTProblem* getCurrentProblem();

  // Get the current problem filename
  const std::string& getCurrentProblemFilename() const { return current_problem_filename_; }

  // Check if a problem is loaded
  bool isProblemLoaded() const;

  // Get list of available problems
  const std::vector<std::string>& getAvailableProblemFiles() const;

  // Add a problem file outside the main directory
  void addProblemFile(const std::string& filepath);

  // Remove a problem file from additional files
  void removeProblemFile(const std::string& filepath);

  // Get the list of additional problem files
  const std::set<std::string>& getAdditionalProblemFiles() const {
    return additional_problem_files_;
  }

  // Set selected algorithm
  void setSelectedAlgorithm(const std::string& name) {
    // Only reset if the algorithm type actually changed
    if (selected_algorithm_ != name) {
      selected_algorithm_ = name;
      // Reset the current algorithm to force recreation with new type
      current_algorithm_.reset();
    }
  }

  // Get selected algorithm
  const std::string& getSelectedAlgorithm() const { return selected_algorithm_; }

  // Check if algorithm selection is valid
  bool hasValidAlgorithmSelection() const;

  // Run selected algorithm with current problem
  bool runAlgorithm();

  // Check if algorithm is currently running
  bool isAlgorithmRunning() const { return algorithm_running_; }

  // Check if algorithm has completed and process results
  bool checkAlgorithmCompletion();

  // Display the algorithm progress dialog
  void renderAlgorithmProgressDialog();

  // Cancel a running algorithm
  void cancelAlgorithm();

  // Get the bounds of the current problem
  Rectangle GetCurrentProblemBounds() const;

  // Solution-related methods
  bool hasSolution() const { return solution_ != nullptr; }
  const algorithm::VRPTSolution* getSolution() const { return solution_.get(); }
  void clearSolution() {
    solution_.reset();
    // Notify that solution has been cleared
    if (solution_changed_callback_) {
      solution_changed_callback_();
    }
  }

  // New algorithm configuration getters/setters
  void setSelectedGenerator(const std::string& generator) { selected_generator_ = generator; }
  const std::string& getSelectedGenerator() const { return selected_generator_; }

  void setSelectedSearches(const std::vector<std::string>& searches) {
    selected_searches_ = searches;
  }
  const std::vector<std::string>& getSelectedSearches() const { return selected_searches_; }

  void setIterationsOrStarts(int value) { iterations_or_starts_ = value; }
  int getIterationsOrStarts() const { return iterations_or_starts_; }

  // Check if an algorithm is selected
  bool isAlgorithmSelected() const;

  // Render algorithm configuration UI
  void renderAlgorithmConfigurationUI();

  // Benchmark-related structures and methods
  struct BenchmarkResult {
    std::string instance_name;   // Name of the problem instance
    std::string algorithm_name;  // Name of the algorithm used
    int num_zones;               // Total number of zones in the problem
    int run_number;              // Run number (for multiple runs)
    int cv_count;                // Number of collection vehicles used
    int tv_count;                // Number of transport vehicles used
    int zones_visited;           // Number of zones visited in the solution
    double total_duration;       // Total duration of all routes
    double total_waste;          // Total waste collected
    double cpu_time_ms;          // CPU time in milliseconds
  };

  // Run benchmark on all available problems
  bool runBenchmark(int runs_per_instance = 3);

  // Check if benchmark is running
  bool isBenchmarkRunning() const { return benchmark_running_; }

  // Check for benchmark completion
  bool checkBenchmarkCompletion();

  // Display benchmark results
  void renderBenchmarkResultsWindow(bool* p_open = nullptr);

  // Show/hide benchmark results window
  bool shouldShowBenchmarkResults() const { return show_benchmark_results_; }
  void setShowBenchmarkResults(bool show) { show_benchmark_results_ = show; }

 private:
  // Map of filename to problem instance
  std::unordered_map<std::string, std::unique_ptr<VRPTProblem>> problems_;

  // Currently loaded problem
  std::string current_problem_filename_;

  // List of available problem files
  std::vector<std::string> available_problems_;

  // Additional problem files to include beyond the scanned directory
  std::set<std::string> additional_problem_files_;

  // Selected algorithm
  std::string selected_algorithm_;

  // Current solution
  std::unique_ptr<algorithm::VRPTSolution> solution_;

  // Callback to notify when a problem is loaded
  ProblemLoadedCallback problem_loaded_callback_;

  // Callback to notify when a solution is cleared or changed
  SolutionChangedCallback solution_changed_callback_;

  // Algorithm configuration
  std::string selected_generator_;
  std::vector<std::string> selected_searches_;
  int iterations_or_starts_ = 10;

  // Algorithm management
  std::string current_algorithm_type_;
  std::unique_ptr<daa::TypedAlgorithm<VRPTProblem, daa::algorithm::VRPTSolution>>
    current_algorithm_;
  bool has_solution_ = false;

  // Async algorithm management
  std::jthread algorithm_thread_;
  std::mutex solution_mutex_;  // Protects thread_solution_, algorithm_status_message_, and other
                               // shared state
  std::unique_ptr<algorithm::VRPTSolution> thread_solution_;
  bool algorithm_running_ = false;
  std::atomic<float> algorithm_progress_{0.0f};
  std::string algorithm_status_message_ = "Initializing...";
  std::chrono::steady_clock::time_point algorithm_start_time_;

  // Benchmark-related member variables
  std::vector<BenchmarkResult> benchmark_results_;
  std::mutex benchmark_results_mutex_;
  bool benchmark_running_ = false;
  bool show_benchmark_results_ = false;
  std::atomic<int> benchmark_completed_count_{0};
  std::atomic<int> benchmark_total_count_{0};
  std::jthread benchmark_thread_;
};

}  // namespace visualization
}  // namespace daa
