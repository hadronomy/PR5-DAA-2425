#include "visualization/problem_manager.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>

#include "algorithm_registry.h"
#include "imgui.h"

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
    algorithm_status_message_ = "Initializing...";
    algorithm_running_ = true;
    algorithm_start_time_ = std::chrono::steady_clock::now();

    // If no algorithm instance exists or algorithm type changed, create a new one
    if (!current_algorithm_ || current_algorithm_type_ != selected_algorithm_) {
      current_algorithm_ =
        AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>(selected_algorithm_);
      current_algorithm_type_ = selected_algorithm_;
    }

    // Launch the algorithm asynchronously
    algorithm_future_ = std::async(std::launch::async, [this, problem]() {
      try {
        algorithm_status_message_ = "Solving problem...";
        algorithm_progress_ = 0.2f;

        // Run the actual algorithm
        auto solution =
          std::make_unique<algorithm::VRPTSolution>(current_algorithm_->solve(*problem));

        algorithm_progress_ = 1.0f;
        algorithm_status_message_ = "Solution found!";

        return solution;
      } catch (const std::exception& e) {
        algorithm_status_message_ = std::string("Error: ") + e.what();
        throw;  // Rethrow to be caught in the main thread
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

  // Check if the future is ready without blocking
  if (algorithm_future_.valid() &&
      algorithm_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    try {
      solution_ = algorithm_future_.get();
      algorithm_running_ = false;

      // Notify that a new solution has been generated
      if (solution_changed_callback_) {
        solution_changed_callback_();
      }

      return true;
    } catch (const std::exception& e) {
      std::cerr << "Error completing algorithm: " << e.what() << std::endl;
      algorithm_running_ = false;
      return false;
    }
  }

  // Update progress as time passes to provide visual feedback
  auto elapsed = std::chrono::steady_clock::now() - algorithm_start_time_;
  auto elapsed_sec =
    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0f;

  // Limit progress to 90% until we get the actual result
  algorithm_progress_ = std::min(0.9f, 0.2f + elapsed_sec / 10.0f);

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

    ImGui::TextWrapped("%s", algorithm_status_message_.c_str());
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

  // We can't truly cancel an async operation that doesn't support cancellation
  // But we can mark it as cancelled and ignore results
  algorithm_status_message_ = "Cancelled by user";
  algorithm_running_ = false;
  // Future will still run to completion but we won't use the result
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
    benchmark_results_.clear();
    benchmark_completed_count_ = 0;

    // Count total runs
    benchmark_total_count_ = available_problems_.size() * runs_per_instance;

    // Reset status
    algorithm_status_message_ = "Initializing benchmark...";
    benchmark_running_ = true;
    algorithm_running_ = true;  // Use same UI indicators
    algorithm_start_time_ = std::chrono::steady_clock::now();

    // If no algorithm instance exists or algorithm type changed, create a new one
    if (!current_algorithm_ || current_algorithm_type_ != selected_algorithm_) {
      current_algorithm_ =
        AlgorithmRegistry::createTyped<VRPTProblem, algorithm::VRPTSolution>(selected_algorithm_);
      current_algorithm_type_ = selected_algorithm_;
    }

    // Launch the benchmark asynchronously
    benchmark_future_ = std::async(std::launch::async, [this, runs_per_instance]() {
      // Save current problem to restore later
      std::string original_problem = current_problem_filename_;

      try {
        for (const auto& problem_file : available_problems_) {
          algorithm_status_message_ = "Benchmarking: " + fs::path(problem_file).filename().string();

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
            auto start_time = std::chrono::high_resolution_clock::now();

            // Run the algorithm
            auto solution = current_algorithm_->solve(*problem);

            auto end_time = std::chrono::high_resolution_clock::now();
            double elapsed_ms =
              std::chrono::duration<double, std::milli>(end_time - start_time).count();

            // Store the result
            BenchmarkResult result;
            result.instance_name = fs::path(problem_file).filename().string();
            result.num_zones = num_zones;
            result.run_number = run + 1;
            result.cv_count = solution.getCVRoutes().size();
            result.tv_count = solution.getTVRoutes().size();
            result.cpu_time_ms = elapsed_ms;

            benchmark_results_.push_back(result);
            benchmark_completed_count_++;
          }
        }

        // Restore original problem if valid
        if (!original_problem.empty()) {
          loadProblem(original_problem);
        }

        algorithm_status_message_ = "Benchmark complete!";

      } catch (const std::exception& e) {
        algorithm_status_message_ = std::string("Benchmark error: ") + e.what();
        // Restore original problem if valid
        if (!original_problem.empty()) {
          loadProblem(original_problem);
        }
        throw;
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

  // Update progress based on completed runs
  if (benchmark_total_count_ > 0) {
    algorithm_progress_ = static_cast<float>(benchmark_completed_count_) / benchmark_total_count_;
  }

  // Check if the future is ready without blocking
  if (benchmark_future_.valid() &&
      benchmark_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    try {
      benchmark_future_.get();  // Get any exceptions
      benchmark_running_ = false;
      algorithm_running_ = false;
      return true;
    } catch (const std::exception& e) {
      std::cerr << "Error completing benchmark: " << e.what() << std::endl;
      benchmark_running_ = false;
      algorithm_running_ = false;
      return false;
    }
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
    // Show status during running
    if (benchmark_running_) {
      ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "Running benchmark...");
      ImGui::Text(
        "%d/%d completed", benchmark_completed_count_.load(), benchmark_total_count_.load()
      );
      ImGui::ProgressBar(algorithm_progress_, ImVec2(-1, 16));

      auto elapsed = std::chrono::steady_clock::now() - algorithm_start_time_;
      auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
      ImGui::Text("Elapsed time: %02d:%02d", (int)(elapsed_sec / 60), (int)(elapsed_sec % 60));
    } else if (!benchmark_results_.empty()) {
      if (ImGui::BeginTable(
            "BenchmarkTable",
            6,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
              ImGuiTableFlags_Sortable
          )) {

        ImGui::TableSetupColumn("Instance", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("Zones", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Run #", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("#CV", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("#TV", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("CPU Time (ms)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableHeadersRow();

        // Display all benchmark results
        for (const auto& result : benchmark_results_) {
          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%s", result.instance_name.c_str());

          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%d", result.num_zones);

          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%d", result.run_number);

          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%d", result.cv_count);

          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%d", result.tv_count);

          ImGui::TableSetColumnIndex(5);
          ImGui::Text("%.2f", result.cpu_time_ms);
        }

        // Calculate and show averages
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("AVERAGES");

        // Calculate averages
        double avg_cv = 0.0, avg_tv = 0.0, avg_time = 0.0;
        int count = 0;

        for (const auto& result : benchmark_results_) {
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

        // Skip zones and run# columns
        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%.2f", avg_cv);

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.2f", avg_tv);

        ImGui::TableSetColumnIndex(5);
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
      if (ImGui::Button("Export Results", ImVec2(150, 0))) {
        // TODO: Implement exporting results to CSV
        // This would be a nice feature but isn't essential
      }

      ImGui::SameLine();

      if (ImGui::Button("Clear Results", ImVec2(150, 0))) {
        benchmark_results_.clear();
      }
    }
  }

  // Sync our internal state when window is closed via the X button
  if (p_open && !*p_open) {
    show_benchmark_results_ = false;
  }

  ImGui::End();
}

}  // namespace visualization
}  // namespace daa
