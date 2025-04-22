#include "visualization/problem_manager.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <stop_token>
#include <thread>

#include "algorithm_registry.h"
#include "imgui.h"
#include "tinyfiledialogs.h"
#include "visualization/csv_exporter.h"
#include "visualization/latex_exporter.h"

namespace fs = std::filesystem;

namespace daa {
namespace visualization {

ProblemManager::ProblemManager()
    : current_problem_filename_(""),
      problem_loaded_callback_(nullptr),
      solution_changed_callback_(nullptr) {}

void ProblemManager::addProblemFile(const std::string& filepath) {
  // Only add if it's a .txt file
  if (fs::path(filepath).extension() == ".txt") {
    additional_problem_files_.insert(filepath);
    // If no problems have been loaded yet, initiate a scan to include this file
    if (available_problems_.empty()) {
      scanDirectory("examples/");
    } else {
      // Just add the file to available problems if it's not already there
      if (std::find(available_problems_.begin(), available_problems_.end(), filepath) ==
          available_problems_.end()) {
        available_problems_.push_back(filepath);
        std::sort(available_problems_.begin(), available_problems_.end());
      }
    }
  }
}

void ProblemManager::removeProblemFile(const std::string& filepath) {
  additional_problem_files_.erase(filepath);
  // Remove from available problems if present
  auto it = std::find(available_problems_.begin(), available_problems_.end(), filepath);
  if (it != available_problems_.end()) {
    available_problems_.erase(it);
  }

  // If this was the currently loaded problem, clear it
  if (current_problem_filename_ == filepath) {
    problems_.erase(filepath);
    current_problem_filename_ = "";
    clearSolution();
  }
}

bool ProblemManager::scanDirectory(const std::string& dir_path) {
  // Remember the currently selected problem if any
  std::string previously_selected = current_problem_filename_;

  // Clear the cache of loaded problems to force reload from files
  problems_.clear();
  current_problem_filename_ = "";
  // Clear any existing solution
  clearSolution();

  available_problems_.clear();

  try {
    // Check if the directory exists
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
      std::cerr << "Directory does not exist: " << dir_path << std::endl;
      return false;
    }

    // Iterate through the directory
    for (const auto& entry : fs::directory_iterator(dir_path)) {
      if (entry.is_regular_file()) {
        std::string filename = entry.path().filename().string();
        // Filter for .txt files or other appropriate extensions
        if (entry.path().extension() == ".txt") {
          available_problems_.push_back(entry.path().string());
        }
      }
    }

    // Add additional problem files that have been manually added
    for (const auto& filepath : additional_problem_files_) {
      if (std::find(available_problems_.begin(), available_problems_.end(), filepath) ==
          available_problems_.end()) {
        available_problems_.push_back(filepath);
      }
    }

    // Sort alphabetically for better UI presentation
    std::sort(available_problems_.begin(), available_problems_.end());

    // If there was a previously selected problem and it still exists, reload it
    if (!previously_selected.empty()) {
      auto it =
        std::find(available_problems_.begin(), available_problems_.end(), previously_selected);
      if (it != available_problems_.end()) {
        loadProblem(previously_selected);
      }
    }

    return true;
  } catch (const fs::filesystem_error& e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
    return false;
  } catch (const std::exception& e) {
    std::cerr << "Error scanning directory: " << e.what() << std::endl;
    return false;
  }
}

bool ProblemManager::loadProblem(const std::string& filename) {
  // Clear any existing solution when loading a new problem
  clearSolution();

  // Check if we already have this problem loaded
  if (problems_.find(filename) != problems_.end()) {
    // Use existing problem instance
    current_problem_filename_ = filename;
  } else {
    // Create and load new problem instance
    auto problem = std::make_unique<VRPTProblem>();
    if (!problem->loadFromFile(filename)) {
      std::cerr << "Failed to load problem from: " << filename << std::endl;
      return false;
    }

    // Store the loaded problem
    problems_[filename] = std::move(problem);
    current_problem_filename_ = filename;
  }

  // Calculate bounds and notify via callback
  if (problem_loaded_callback_ && isProblemLoaded()) {
    Rectangle bounds = GetCurrentProblemBounds();
    problem_loaded_callback_(bounds);
  }

  return true;
}

Rectangle ProblemManager::GetCurrentProblemBounds() const {
  if (!isProblemLoaded()) {
    return Rectangle{0, 0, 0, 0};
  }

  auto problem = problems_.at(current_problem_filename_).get();

  // Initialize with extreme values
  float minX = std::numeric_limits<float>::max();
  float minY = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float maxY = std::numeric_limits<float>::lowest();

  bool hasLocations = false;

  // Process all types of locations to find the bounds

  // Include depot
  try {
    const auto& depot = problem->getDepot();
    minX = std::min(minX, static_cast<float>(depot.x()));
    minY = std::min(minY, static_cast<float>(depot.y()));
    maxX = std::max(maxX, static_cast<float>(depot.x()));
    maxY = std::max(maxY, static_cast<float>(depot.y()));
    hasLocations = true;
  } catch (const std::exception&) {
    // Ignore if depot not found
  }

  // Include landfill
  try {
    const auto& landfill = problem->getLandfill();
    minX = std::min(minX, static_cast<float>(landfill.x()));
    minY = std::min(minY, static_cast<float>(landfill.y()));
    maxX = std::max(maxX, static_cast<float>(landfill.x()));
    maxY = std::max(maxY, static_cast<float>(landfill.y()));
    hasLocations = true;
  } catch (const std::exception&) {
    // Ignore if landfill not found
  }

  // Include SWTS locations
  try {
    const auto& swtsLocations = problem->getSWTS();
    for (const auto& swts : swtsLocations) {
      minX = std::min(minX, static_cast<float>(swts.x()));
      minY = std::min(minY, static_cast<float>(swts.y()));
      maxX = std::max(maxX, static_cast<float>(swts.x()));
      maxY = std::max(maxY, static_cast<float>(swts.y()));
      hasLocations = true;
    }
  } catch (const std::exception&) {
    // Ignore if SWTS not found
  }

  // Include zones
  try {
    const auto& zones = problem->getZones();
    for (const auto& zone : zones) {
      minX = std::min(minX, static_cast<float>(zone.x()));
      minY = std::min(minY, static_cast<float>(zone.y()));
      maxX = std::max(maxX, static_cast<float>(zone.x()));
      maxY = std::max(maxY, static_cast<float>(zone.y()));
      hasLocations = true;
    }
  } catch (const std::exception&) {
    // Ignore if zones not found
  }

  // If no locations found, return empty rectangle
  if (!hasLocations) {
    return Rectangle{0, 0, 0, 0};
  }

  // Convert to Rectangle format
  return Rectangle{minX, minY, maxX - minX, maxY - minY};
}

VRPTProblem* ProblemManager::getCurrentProblem() {
  if (!isProblemLoaded()) {
    return nullptr;
  }

  return problems_[current_problem_filename_].get();
}

bool ProblemManager::isProblemLoaded() const {
  return !current_problem_filename_.empty() &&
         problems_.find(current_problem_filename_) != problems_.end();
}

const std::vector<std::string>& ProblemManager::getAvailableProblemFiles() const {
  return available_problems_;
}

bool ProblemManager::hasValidAlgorithmSelection() const {
  return !selected_algorithm_.empty() && AlgorithmRegistry::exists(selected_algorithm_);
}

bool ProblemManager::isAlgorithmSelected() const {
  return !selected_algorithm_.empty() && AlgorithmRegistry::exists(selected_algorithm_);
}

bool ProblemManager::runAlgorithm() {
  if (!isAlgorithmSelected() || !isProblemLoaded() || algorithm_running_) {
    return false;
  }

  try {
    // Get the current problem
    VRPTProblem* problem = getCurrentProblem();
    if (!problem) {
      return false;
    }

    // Reset progress and status
    algorithm_progress_ = 0.0f;
    {
      std::lock_guard<std::mutex> lock(solution_mutex_);
      algorithm_status_message_ = "Initializing...";
    }
    algorithm_running_ = true;
    algorithm_start_time_ = std::chrono::steady_clock::now();

    // If no algorithm instance exists or algorithm type changed, create a new one
    if (!current_algorithm_ || current_algorithm_type_ != selected_algorithm_) {
      current_algorithm_ =
        AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>(selected_algorithm_);
      current_algorithm_type_ = selected_algorithm_;
    }

    // Clear any previous solution
    {
      std::lock_guard<std::mutex> lock(solution_mutex_);
      thread_solution_.reset();
    }

    // Launch the algorithm using jthread with stop_token support
    algorithm_thread_ = std::jthread([this, problem](std::stop_token stop_token) {
      try {
        // Update status in a thread-safe way
        {
          std::lock_guard<std::mutex> lock(solution_mutex_);
          algorithm_status_message_ = "Solving problem...";
          algorithm_progress_ = 0.2f;
        }

        // Run the actual algorithm
        auto solution =
          std::make_unique<algorithm::VRPTSolution>(current_algorithm_->solve(*problem));

        // Check if we've been requested to stop
        if (stop_token.stop_requested()) {
          return;
        }

        // Store the solution and update status in a thread-safe way
        {
          std::lock_guard<std::mutex> lock(solution_mutex_);
          thread_solution_ = std::move(solution);
          algorithm_progress_ = 1.0f;
          algorithm_status_message_ = "Solution found!";
        }
      } catch (const std::exception& e) {
        if (!stop_token.stop_requested()) {
          std::lock_guard<std::mutex> lock(solution_mutex_);
          algorithm_status_message_ = std::string("Error: ") + e.what();
        }
      }
    });

    return true;
  } catch (const std::exception& e) {
    algorithm_running_ = false;
    std::cerr << "Error running algorithm: " << e.what() << std::endl;
    return false;
  }
}

bool ProblemManager::checkAlgorithmCompletion() {
  if (!algorithm_running_) {
    return false;
  }

  // Check if we have a solution available
  std::unique_ptr<algorithm::VRPTSolution> thread_result;
  {
    std::lock_guard<std::mutex> lock(solution_mutex_);
    if (thread_solution_) {
      thread_result = std::move(thread_solution_);
    }
  }

  // If we have a solution or the progress is at 100%, consider it complete
  if (thread_result || algorithm_progress_ >= 1.0f) {
    if (thread_result) {
      // We have a valid solution
      solution_ = std::move(thread_result);

      // Notify that a new solution has been generated
      if (solution_changed_callback_) {
        solution_changed_callback_();
      }
    }

    algorithm_running_ = false;
    return true;
  }

  // Thread is still running, update progress as time passes to provide visual feedback
  auto elapsed = std::chrono::steady_clock::now() - algorithm_start_time_;
  auto elapsed_sec =
    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0f;

  // Limit progress to 90% until we get the actual result
  if (algorithm_progress_ < 0.9f) {
    algorithm_progress_ = std::min(0.9f, 0.2f + elapsed_sec / 10.0f);
  }

  return false;
}

void ProblemManager::renderAlgorithmProgressDialog() {
  if (!algorithm_running_) {
    return;
  }

  // Setup dialog size and position
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(400, 150));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;

  if (ImGui::Begin("##AlgorithmRunningDialog", nullptr, flags)) {
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 380);
    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "Running %s", selected_algorithm_.c_str());
    ImGui::PopTextWrapPos();
    ImGui::Spacing();
    ImGui::Spacing();

    // Get status message in a thread-safe way
    std::string status_message;
    {
      std::lock_guard<std::mutex> lock(solution_mutex_);
      status_message = algorithm_status_message_;
    }

    ImGui::TextWrapped("%s", status_message.c_str());
    ImGui::Spacing();

    ImGui::ProgressBar(algorithm_progress_, ImVec2(-1, 16), "");

    auto elapsed = std::chrono::steady_clock::now() - algorithm_start_time_;
    auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    ImGui::Text("Elapsed time: %02d:%02d", (int)(elapsed_sec / 60), (int)(elapsed_sec % 60));

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      cancelAlgorithm();
    }
  }
  ImGui::End();
}

void ProblemManager::cancelAlgorithm() {
  if (!algorithm_running_) {
    return;
  }

  // Request the algorithm thread to stop using the stop_token
  if (algorithm_thread_.joinable()) {
    algorithm_thread_.request_stop();
    // No need to join, jthread will automatically join when it goes out of scope
  }

  // Also cancel benchmark if it's running
  if (benchmark_running_ && benchmark_thread_.joinable()) {
    benchmark_thread_.request_stop();
  }

  // Update status message in a thread-safe way
  {
    std::lock_guard<std::mutex> lock(solution_mutex_);
    algorithm_status_message_ = "Cancelled by user";
  }

  algorithm_running_ = false;

  // If this was a benchmark cancellation, also update benchmark state
  if (benchmark_running_) {
    algorithm_progress_ = 1.0f;  // Set progress to 100% when cancelled
    benchmark_running_ = false;
  }
}

void ProblemManager::renderAlgorithmConfigurationUI() {
  if (!current_algorithm_) {
    if (!isAlgorithmSelected()) {
      return;
    }
    // Create the algorithm instance if it doesn't exist
    try {
      current_algorithm_ =
        AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>(selected_algorithm_);
      current_algorithm_type_ = selected_algorithm_;
    } catch (const std::exception& e) {
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", e.what());
      return;
    }
  }

  // Render the algorithm's configuration UI
  current_algorithm_->renderConfigurationUI();
}

bool ProblemManager::runBenchmark(int runs_per_instance) {
  if (!isAlgorithmSelected() || benchmark_running_) {
    return false;
  }

  try {
    // Clear previous results
    {
      std::lock_guard<std::mutex> lock(benchmark_results_mutex_);
      benchmark_results_.clear();
    }
    benchmark_completed_count_ = 0;

    // Count total runs
    benchmark_total_count_ = available_problems_.size() * runs_per_instance;

    // Reset status and progress
    {
      std::lock_guard<std::mutex> lock(solution_mutex_);
      algorithm_status_message_ = "Initializing benchmark...";
      algorithm_progress_ = 0.0f;  // Start with 0% progress
    }
    benchmark_running_ = true;
    algorithm_running_ = true;  // Use same UI indicators
    algorithm_start_time_ = std::chrono::steady_clock::now();

    // If no algorithm instance exists or algorithm type changed, create a new one
    if (!current_algorithm_ || current_algorithm_type_ != selected_algorithm_) {
      current_algorithm_ =
        AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>(selected_algorithm_);
      current_algorithm_type_ = selected_algorithm_;
    }

    // Launch the benchmark using jthread with stop_token support
    benchmark_thread_ = std::jthread([this, runs_per_instance](std::stop_token stop_token) {
      // Save current problem to restore later
      std::string original_problem = current_problem_filename_;

      try {
        for (const auto& problem_file : available_problems_) {
          // Check if stop was requested
          if (stop_token.stop_requested()) {
            std::lock_guard<std::mutex> lock(solution_mutex_);
            algorithm_status_message_ = "Benchmark cancelled";
            algorithm_progress_ = 1.0f;  // Set progress to 100% when cancelled
            break;
          }

          // Update status message in a thread-safe way
          {
            std::lock_guard<std::mutex> lock(solution_mutex_);
            algorithm_status_message_ =
              "Benchmarking: " + fs::path(problem_file).filename().string();
          }

          // Load the problem
          if (!loadProblem(problem_file)) {
            continue;  // Skip if can't load
          }

          VRPTProblem* problem = getCurrentProblem();
          if (!problem) {
            continue;
          }

          // Get number of zones
          int num_zones = problem->getNumZones();

          // Run the algorithm multiple times on this instance
          for (int run = 0; run < runs_per_instance; run++) {
            // Check if stop was requested
            if (stop_token.stop_requested()) {
              std::lock_guard<std::mutex> lock(solution_mutex_);
              algorithm_status_message_ = "Benchmark cancelled";
              algorithm_progress_ = 1.0f;  // Set progress to 100% when cancelled
              break;
            }

            auto start_time = std::chrono::high_resolution_clock::now();

            // Run the algorithm
            auto solution = current_algorithm_->solve(*problem);

            auto end_time = std::chrono::high_resolution_clock::now();
            double elapsed_ms =
              std::chrono::duration<double, std::milli>(end_time - start_time).count();

            // Store the result in a thread-safe way
            BenchmarkResult result;
            result.instance_name = fs::path(problem_file).filename().string();
            result.algorithm_name = current_algorithm_->name();

            result.num_zones = num_zones;
            result.run_number = run + 1;
            result.cv_count = solution.getCVRoutes().size();
            result.tv_count = solution.getTVRoutes().size();
            result.zones_visited = solution.visitedZones(*problem);
            result.total_duration = solution.totalDuration().value();
            result.total_waste = solution.totalWasteCollected().value();
            result.cpu_time_ms = elapsed_ms;

            {
              std::lock_guard<std::mutex> lock(benchmark_results_mutex_);
              benchmark_results_.push_back(result);
            }

            // Update completion count and progress
            benchmark_completed_count_++;

            // Update progress directly to ensure UI reflects current state
            if (benchmark_total_count_ > 0) {
              algorithm_progress_ =
                static_cast<float>(benchmark_completed_count_) / benchmark_total_count_;
            }
          }

          // Check if we need to break the outer loop too
          if (stop_token.stop_requested()) {
            break;
          }
        }

        // Restore original problem if valid
        if (!original_problem.empty()) {
          loadProblem(original_problem);
        }

        if (!stop_token.stop_requested()) {
          // Signal benchmark completion
          std::lock_guard<std::mutex> lock(solution_mutex_);
          algorithm_status_message_ = "Benchmark complete!";

          // Ensure progress is at 100% when complete
          algorithm_progress_ = 1.0f;

          // Force benchmark state to complete
          benchmark_running_ = false;
          algorithm_running_ = false;

          // Make sure the results window stays open
          show_benchmark_results_ = true;
        }

      } catch (const std::exception& e) {
        if (!stop_token.stop_requested()) {
          std::lock_guard<std::mutex> lock(solution_mutex_);
          algorithm_status_message_ = std::string("Benchmark error: ") + e.what();
          algorithm_progress_ = 1.0f;  // Set progress to 100% on error

          // Force benchmark state to complete
          benchmark_running_ = false;
          algorithm_running_ = false;

          // Make sure the results window stays open
          show_benchmark_results_ = true;
        }
        // Restore original problem if valid
        if (!original_problem.empty()) {
          loadProblem(original_problem);
        }
      }
    });

    // Show the results window when starting a benchmark
    show_benchmark_results_ = true;
    return true;
  } catch (const std::exception& e) {
    benchmark_running_ = false;
    algorithm_running_ = false;
    std::cerr << "Error starting benchmark: " << e.what() << std::endl;
    return false;
  }
}

bool ProblemManager::checkBenchmarkCompletion() {
  if (!benchmark_running_) {
    return false;
  }

  // Get status message in a thread-safe way
  std::string status_message;
  {
    std::lock_guard<std::mutex> lock(solution_mutex_);
    status_message = algorithm_status_message_;
  }

  // Get current completion count
  int completed = benchmark_completed_count_.load();
  int total = benchmark_total_count_.load();

  // Check for completion conditions
  bool is_complete = false;

  // Check if all runs are completed
  if (total > 0 && completed >= total) {
    is_complete = true;
  }

  // Check status messages that indicate completion
  if (status_message == "Benchmark complete!" || status_message == "Benchmark cancelled" ||
      status_message.find("Benchmark error") == 0) {
    is_complete = true;
  }

  // Check if thread is no longer running
  if (!benchmark_thread_.joinable()) {
    is_complete = true;
  }

  // If benchmark is complete, update state
  if (is_complete) {
    // Set flags to indicate completion
    benchmark_running_ = false;
    algorithm_running_ = false;

    // Ensure progress bar shows 100% when complete
    algorithm_progress_ = 1.0f;

    // Make sure the status message indicates completion if it doesn't already
    if (status_message != "Benchmark complete!" && status_message != "Benchmark cancelled" &&
        status_message.find("Benchmark error") != 0) {
      std::lock_guard<std::mutex> lock(solution_mutex_);
      algorithm_status_message_ = "Benchmark complete!";
    }

    // Make sure the results window stays open
    show_benchmark_results_ = true;

    return true;
  }

  return false;
}

void ProblemManager::renderBenchmarkResultsWindow(bool* p_open) {
  // If p_open is provided and false, don't show the window
  if (p_open && !*p_open) {
    show_benchmark_results_ = false;
    return;
  }

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

  if (ImGui::Begin("Benchmark Results", p_open, window_flags)) {
    // Get status message in a thread-safe way
    std::string status_message;
    {
      std::lock_guard<std::mutex> lock(solution_mutex_);
      status_message = algorithm_status_message_;
    }

    // Get a thread-safe copy of the benchmark results
    std::vector<BenchmarkResult> results_copy;
    {
      std::lock_guard<std::mutex> lock(benchmark_results_mutex_);
      results_copy = benchmark_results_;
    }

    // Check if we have a completion status message
    bool has_completion_status =
      (status_message == "Benchmark complete!" || status_message == "Benchmark cancelled" ||
       status_message.find("Benchmark error") == 0);

    // Show status during running
    if (benchmark_running_) {
      ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "Running benchmark...");
      ImGui::Text(
        "%d/%d completed", benchmark_completed_count_.load(), benchmark_total_count_.load()
      );
      ImGui::TextWrapped("%s", status_message.c_str());
      ImGui::ProgressBar(algorithm_progress_, ImVec2(-1, 16));

      auto elapsed = std::chrono::steady_clock::now() - algorithm_start_time_;
      auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
      ImGui::Text("Elapsed time: %02d:%02d", (int)(elapsed_sec / 60), (int)(elapsed_sec % 60));
    }
    // Show completion message and results
    else {
      // Show completion status if available
      if (has_completion_status) {
        ImGui::TextColored(
          status_message == "Benchmark complete!" ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
                                                  : ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
          "%s",
          status_message.c_str()
        );
        ImGui::Separator();
        ImGui::Spacing();
      }

      // Show results if available
      if (!results_copy.empty()) {
        if (ImGui::BeginTable(
              "BenchmarkTable",
              10,
              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_Sortable
            )) {

          ImGui::TableSetupColumn("Instance", ImGuiTableColumnFlags_DefaultSort);
          ImGui::TableSetupColumn("Algorithm", ImGuiTableColumnFlags_WidthFixed, 150.0f);
          ImGui::TableSetupColumn("Total Zones", ImGuiTableColumnFlags_WidthFixed, 60.0f);
          ImGui::TableSetupColumn("Run #", ImGuiTableColumnFlags_WidthFixed, 60.0f);
          ImGui::TableSetupColumn("#CV", ImGuiTableColumnFlags_WidthFixed, 60.0f);
          ImGui::TableSetupColumn("#TV", ImGuiTableColumnFlags_WidthFixed, 60.0f);
          ImGui::TableSetupColumn("Zones Visited", ImGuiTableColumnFlags_WidthFixed, 80.0f);
          ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f);
          ImGui::TableSetupColumn("Waste", ImGuiTableColumnFlags_WidthFixed, 80.0f);
          ImGui::TableSetupColumn("CPU Time (ms)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
          ImGui::TableHeadersRow();

          // Display all benchmark results (we already have a thread-safe copy from above)

          for (const auto& result : results_copy) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", result.instance_name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", result.algorithm_name.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", result.num_zones);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", result.run_number);

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%d", result.cv_count);

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%d", result.tv_count);

            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%d", result.zones_visited);

            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%.2f", result.total_duration);

            ImGui::TableSetColumnIndex(8);
            ImGui::Text("%.2f", result.total_waste);

            ImGui::TableSetColumnIndex(9);
            ImGui::Text("%.2f", result.cpu_time_ms);
          }

          // Calculate and show averages
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
          ImGui::Text("AVERAGES");

          // Calculate averages (using the thread-safe copy)
          double avg_cv = 0.0, avg_tv = 0.0, avg_zones_visited = 0.0;
          double avg_duration = 0.0, avg_waste = 0.0, avg_time = 0.0;
          int count = 0;

          for (const auto& result : results_copy) {
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

          // Skip instance, algorithm, total zones, and run# columns

          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%.2f", avg_cv);

          ImGui::TableSetColumnIndex(5);
          ImGui::Text("%.2f", avg_tv);

          ImGui::TableSetColumnIndex(6);
          ImGui::Text("%.2f", avg_zones_visited);

          ImGui::TableSetColumnIndex(7);
          ImGui::Text("%.2f", avg_duration);

          ImGui::TableSetColumnIndex(8);
          ImGui::Text("%.2f", avg_waste);

          ImGui::TableSetColumnIndex(9);
          ImGui::Text("%.2f", avg_time);
          ImGui::PopStyleColor();

          ImGui::EndTable();
        }
      } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No benchmark data available");
        ImGui::Spacing();
        ImGui::Text("Click 'Benchmark All' in the Algorithm Selector to start a benchmark.");
      }

      ImGui::Spacing();

      if (benchmark_running_) {
        if (ImGui::Button("Cancel Benchmark", ImVec2(150, 0))) {
          cancelAlgorithm();  // Reuse cancel functionality
          benchmark_running_ = false;
        }
      } else {
        // Export button and popup
        if (ImGui::Button("Export Results", ImVec2(150, 0))) {
          // Get a thread-safe copy of the benchmark results
          std::vector<BenchmarkResult> results_copy;
          {
            std::lock_guard<std::mutex> lock(benchmark_results_mutex_);
            results_copy = benchmark_results_;
          }

          if (results_copy.empty()) {
            // Show a message if there are no results to export
            ImGui::OpenPopup("No Results##ExportError");
          } else {
            // Open the export format popup
            ImGui::OpenPopup("Export Format##Popup");
          }
        }

        // Export format popup
        if (ImGui::BeginPopup("Export Format##Popup")) {
          ImGui::Text("Select Export Format:");
          ImGui::Separator();

          // Get a thread-safe copy of the benchmark results for the popup
          std::vector<BenchmarkResult> results_copy;
          {
            std::lock_guard<std::mutex> lock(benchmark_results_mutex_);
            results_copy = benchmark_results_;
          }

          if (ImGui::MenuItem("CSV")) {
            // Use tinyfiledialogs to get save location for CSV
            const char* csv_filters[] = {"*.csv"};
            const char* csv_filepath = tinyfd_saveFileDialog(
              "Save Benchmark Results as CSV",  // title
              "benchmark_results.csv",          // default filename
              1,                                // number of filter patterns
              csv_filters,                      // filter patterns
              "CSV Files"                       // filter description
            );

            if (csv_filepath) {
              // Export the results to CSV
              bool success = CSVExporter::exportToCSV(results_copy, csv_filepath);

              // Show success/failure message
              if (success) {
                ImGui::CloseCurrentPopup();  // Close the format popup
                ImGui::OpenPopup("Export Success");
              } else {
                ImGui::CloseCurrentPopup();  // Close the format popup
                ImGui::OpenPopup("Export Failed");
              }
            }
          }

          if (ImGui::MenuItem("LaTeX")) {
            // Use tinyfiledialogs to get save location for LaTeX
            const char* tex_filters[] = {"*.tex"};
            const char* tex_filepath = tinyfd_saveFileDialog(
              "Save Benchmark Results as LaTeX",  // title
              "benchmark_results.tex",            // default filename
              1,                                  // number of filter patterns
              tex_filters,                        // filter patterns
              "LaTeX Files"                       // filter description
            );

            if (tex_filepath) {
              // Export the results to LaTeX
              bool success = LatexExporter::exportToLatex(results_copy, tex_filepath);

              // Show success/failure message
              if (success) {
                ImGui::CloseCurrentPopup();  // Close the format popup
                ImGui::OpenPopup("Export Success");
              } else {
                ImGui::CloseCurrentPopup();  // Close the format popup
                ImGui::OpenPopup("Export Failed");
              }
            }
          }

          ImGui::EndPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear Results", ImVec2(150, 0))) {
          std::lock_guard<std::mutex> lock(benchmark_results_mutex_);
          benchmark_results_.clear();
        }

        // Export error popup
        if (ImGui::BeginPopupModal(
              "No Results##ExportError", NULL, ImGuiWindowFlags_AlwaysAutoResize
            )) {
          ImGui::Text("There are no benchmark results to export.");
          ImGui::Separator();
          if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
          }
          ImGui::EndPopup();
        }

        // Export success popup
        if (ImGui::BeginPopupModal("Export Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
          ImGui::Text("Benchmark results exported successfully.");
          ImGui::Separator();
          if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
          }
          ImGui::EndPopup();
        }

        // Export failure popup
        if (ImGui::BeginPopupModal("Export Failed", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
          ImGui::Text("Failed to export benchmark results.");
          ImGui::Text("Please check file permissions and try again.");
          ImGui::Separator();
          if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
          }
          ImGui::EndPopup();
        }
      }
    }

    // Sync our internal state when window is closed via the X button
    if (p_open && !*p_open) {
      show_benchmark_results_ = false;
    }

    ImGui::End();
  }
}

}  // namespace visualization
}  // namespace daa
