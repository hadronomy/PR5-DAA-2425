#include "visualization/ui_components.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

namespace daa {
namespace visualization {

UIComponents::UIComponents(ObjectManager* object_manager)
    : object_manager_(object_manager),
      problem_manager_(nullptr),
      show_problem_selector_(true),
      show_problem_inspector_(true),
      show_algorithm_selector_(true),
      show_no_algorithm_warning_(false) {}
UIComponents::~UIComponents() {}

void UIComponents::Initialize() {
  // Default initialization
}

void UIComponents::SetProblemManager(ProblemManager* problem_manager) {
  problem_manager_ = problem_manager;
}

void UIComponents::ScanProblemsDirectory(const std::string& dir_path) {
  if (problem_manager_) {
    problem_manager_->scanDirectory(dir_path);
  }
}

void UIComponents::RenderDiagonalPattern(
  const ImVec2& min,
  const ImVec2& max,
  ImU32 color,
  float thickness,
  float spacing
) {
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  // First fill the entire rectangle with a background color
  ImU32 bg_color = ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.1f, 0.7f));
  draw_list->AddRectFilled(min, max, bg_color);

  float width = max.x - min.x;
  float height = max.y - min.y;

  // Calculate number of lines based on spacing
  int num_lines = static_cast<int>((width + height) / spacing) + 1;

  // Use a slightly lighter gray color for the stripes if not specified
  ImU32 line_color = color ? color : ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

  // Draw diagonal stripes
  for (int i = 0; i < num_lines; i++) {
    float offset = i * spacing;

    // Draw lines from bottom-left to top-right
    {
      float x_start = min.x + offset;
      float y_start = max.y;

      if (x_start > max.x) {
        y_start = max.y - (x_start - max.x);
        x_start = max.x;
      }

      float x_end = min.x;
      float y_end = max.y - offset;

      if (y_end < min.y) {
        x_end = min.x + (min.y - y_end);
        y_end = min.y;
      }

      draw_list->AddLine(ImVec2(x_start, y_start), ImVec2(x_end, y_end), line_color, thickness);
    }
  }
}

void UIComponents::RenderEmptyStateOverlay() {
  auto* canvas_window = ImGui::FindWindowByName("Canvas");

  if (!canvas_window || canvas_window->Hidden)
    return;

  if (problem_manager_ && problem_manager_->isProblemLoaded())
    return;

  ImGui::SetNextWindowPos(canvas_window->Pos);
  ImGui::SetNextWindowSize(canvas_window->Size);
  ImGui::SetNextWindowBgAlpha(0.0f);

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                  ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoScrollWithMouse;

  if (ImGui::Begin("##EmptyCanvasOverlay", nullptr, window_flags)) {
    ImVec2 min = ImGui::GetWindowPos();
    ImVec2 max = ImVec2(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());

    // Render diagonal pattern as background
    ImU32 pattern_color = ImGui::GetColorU32(ImVec4(0.6f, 0.2f, 0.2f, 0.3f));
    RenderDiagonalPattern(min, max, pattern_color, 8.0f, 50.0f);

    ImVec2 window_size = ImGui::GetWindowSize();

    // Main title and subtitle text
    const char* main_text = "No Problem Loaded";
    const char* subtext = "Select a problem file from the Problem Selector";

    ImVec2 main_text_size = ImGui::CalcTextSize(main_text);
    ImVec2 subtext_size = ImGui::CalcTextSize(subtext);

    // Calculate text box with padding to fit both texts
    float padding_x = 60.0f;
    float padding_y = 60.0f;
    float separator_height = 20.0f;  // Space for separator and spacing between texts

    // Use the wider of the two texts to determine box width
    float content_width = std::max(main_text_size.x, subtext_size.x);
    float content_height = main_text_size.y + subtext_size.y + separator_height;

    ImVec2 text_box_min(
      min.x + (window_size.x - content_width - padding_x) * 0.5f,
      min.y + (window_size.y - content_height - padding_y) * 0.5f
    );
    ImVec2 text_box_max(
      text_box_min.x + content_width + padding_x, text_box_min.y + content_height + padding_y
    );

    // Draw a more attractive background box with gradient effect
    ImGui::GetWindowDrawList()->AddRectFilled(
      text_box_min, text_box_max, ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.15f, 0.85f)), 12.0f
    );

    // Add a border with slightly rounded corners
    ImGui::GetWindowDrawList()->AddRect(
      text_box_min,
      text_box_max,
      ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.8f, 0.7f)),
      12.0f,
      ImDrawFlags_RoundCornersAll,
      2.5f
    );

    // Position main text - centered horizontally in the box
    ImVec2 text_pos(
      text_box_min.x + (text_box_max.x - text_box_min.x - main_text_size.x) * 0.5f,
      text_box_min.y + padding_y * 0.4f
    );

    // Add main text with a soft glow effect
    ImGui::GetWindowDrawList()->AddText(
      text_pos, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), main_text
    );

    // Add separator line below the main text
    ImGui::GetWindowDrawList()->AddLine(
      ImVec2(text_box_min.x + padding_x * 0.3f, text_pos.y + main_text_size.y + 10.0f),
      ImVec2(text_box_max.x - padding_x * 0.3f, text_pos.y + main_text_size.y + 10.0f),
      ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.8f, 0.6f)),
      1.0f
    );

    // Add subtitle with proper spacing below the separator
    ImVec2 subtext_pos(
      text_box_min.x + (text_box_max.x - text_box_min.x - subtext_size.x) * 0.5f,
      text_pos.y + main_text_size.y + 25.0f
    );

    ImGui::GetWindowDrawList()->AddText(
      subtext_pos, ImGui::GetColorU32(ImVec4(0.8f, 0.8f, 0.9f, 0.9f)), subtext
    );
  }
  ImGui::End();
}

void UIComponents::RenderWarningDialog(const char* title, const char* message, bool* p_open) {
  ImGui::SetNextWindowPos(
    ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f)
  );

  ImGuiWindowFlags window_flags =
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

  if (ImGui::Begin(title, p_open, window_flags)) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
    ImGui::Text("⚠");
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
    ImGui::TextUnformatted(message);
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();

    ImGui::Separator();

    float button_width = 120.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - button_width) * 0.5f);

    if (ImGui::Button("OK", ImVec2(button_width, 0))) {
      *p_open = false;
    }

    if (ImGui::IsWindowAppearing()) {
      ImGui::SetItemDefaultFocus();
    }
  }
  ImGui::End();
}

void UIComponents::RenderProblemSelector() {
  if (!show_problem_selector_ || !problem_manager_)
    return;

  ImGui::Begin("Problem Selector", &show_problem_selector_);

  ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Available Problems");
  ImGui::SameLine(ImGui::GetWindowWidth() - 70);
  if (ImGui::Button("Refresh")) {
    ScanProblemsDirectory();
  }

  ImGui::Separator();

  const auto& available_problems = problem_manager_->getAvailableProblemFiles();

  if (available_problems.empty()) {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "No problem files found");
    ImGui::TextWrapped("Check the examples directory or use the 'Refresh' button");
  } else {
    std::string current = problem_manager_->isProblemLoaded()
                          ? problem_manager_->getCurrentProblem()->toString().substr(0, 20)
                          : "";

    if (ImGui::BeginListBox(
          "##Problems", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing())
        )) {
      for (size_t i = 0; i < available_problems.size(); i++) {
        const std::string& problem_path = available_problems[i];
        std::string filename = std::filesystem::path(problem_path).filename().string();

        bool is_selected = problem_manager_->isProblemLoaded() &&
                           problem_path == problem_manager_->getCurrentProblemFilename();

        if (ImGui::Selectable(filename.c_str(), is_selected)) {
          problem_manager_->loadProblem(problem_path);
        }

        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::Text("%s", problem_path.c_str());
          ImGui::EndTooltip();
        }
      }
      ImGui::EndListBox();
    }

    if (ImGui::Button("Load Selected", ImVec2(-FLT_MIN, 0))) {}
  }

  ImGui::End();
}

void UIComponents::RenderProblemInspector() {
  if (!show_problem_inspector_ || !problem_manager_)
    return;

  ImGui::Begin("Problem Inspector", &show_problem_inspector_);

  if (!problem_manager_->isProblemLoaded()) {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "No problem loaded");
    ImGui::TextWrapped("Select a problem from the Problem Selector window");
  } else {
    VRPTProblem* problem = problem_manager_->getCurrentProblem();

    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Problem Summary");
    ImGui::Separator();

    ImGui::Columns(2, "ProblemStats", false);
    ImGui::SetColumnWidth(0, 180);

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Collection Vehicles:");
    ImGui::NextColumn();
    ImGui::Text("%d vehicles", problem->getNumCVVehicles());
    ImGui::NextColumn();

    ImGui::Text("Max Duration (CV):");
    ImGui::NextColumn();
    ImGui::Text("%.2f minutes", problem->getCVMaxDuration().value(units::TimeUnit::Minutes));
    ImGui::NextColumn();

    ImGui::Text("Capacity (CV):");
    ImGui::NextColumn();
    ImGui::Text("%.2f units", problem->getCVCapacity().value());
    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Transportation Vehicles:");
    ImGui::NextColumn();
    ImGui::Text("Dynamic");
    ImGui::NextColumn();

    ImGui::Text("Max Duration (TV):");
    ImGui::NextColumn();
    ImGui::Text("%.2f minutes", problem->getTVMaxDuration().value(units::TimeUnit::Minutes));
    ImGui::NextColumn();

    ImGui::Text("Capacity (TV):");
    ImGui::NextColumn();
    ImGui::Text("%.2f units", problem->getTVCapacity().value());
    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Locations:");
    ImGui::NextColumn();
    ImGui::Text("");
    ImGui::NextColumn();

    ImGui::Text("Collection Zones:");
    ImGui::NextColumn();
    ImGui::Text("%d zones", problem->getNumZones());
    ImGui::NextColumn();

    ImGui::Text("Transfer Stations:");
    ImGui::NextColumn();
    ImGui::Text("%zu stations", problem->getSWTS().size());
    ImGui::NextColumn();

    ImGui::Columns(1);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Vehicle Parameters");
    ImGui::Text(
      "Speed: %.2f km/h",
      problem->getVehicleSpeed().getValue(units::DistanceUnit::Kilometers, units::TimeUnit::Hours)
    );

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Depot & Landfill Locations")) {
      try {
        const auto& depot = problem->getDepot();
        const auto& landfill = problem->getLandfill();

        ImGui::Text("Depot: (%.2f, %.2f)", depot.x(), depot.y());
        ImGui::Text("Landfill: (%.2f, %.2f)", landfill.x(), landfill.y());
      } catch (const std::exception& e) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", e.what());
      }
    }

    if (ImGui::CollapsingHeader("Transfer Stations")) {
      const auto stations = problem->getSWTS();
      for (size_t i = 0; i < stations.size(); ++i) {
        ImGui::Text(
          "%s: (%.2f, %.2f)", stations[i].name().c_str(), stations[i].x(), stations[i].y()
        );
      }
    }

    if (ImGui::CollapsingHeader("Collection Zones")) {
      const auto zones = problem->getZones();

      if (ImGui::BeginTable(
            "ZonesTable",
            4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
            ImVec2(0, 200)
          )) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Coordinates");
        ImGui::TableSetupColumn("Waste");
        ImGui::TableSetupColumn("Service Time");
        ImGui::TableHeadersRow();

        for (const auto& zone : zones) {
          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%s", zone.id().c_str());

          ImGui::TableSetColumnIndex(1);
          ImGui::Text("(%.2f, %.2f)", zone.x(), zone.y());

          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%.2f", zone.wasteAmount().value());

          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%.2fs", zone.serviceTime().value(units::TimeUnit::Seconds));
        }

        ImGui::EndTable();
      }
    }
  }

  ImGui::End();
}

void UIComponents::RenderAlgorithmSelector() {
  if (!show_algorithm_selector_ || !problem_manager_)
    return;

  ImGui::Begin("Algorithm Selector", &show_algorithm_selector_);

  if (!problem_manager_->isProblemLoaded()) {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "No problem loaded");
    ImGui::TextWrapped("Load a problem first to select algorithms");
    ImGui::End();
    return;
  }

  std::vector<std::string> generators = problem_manager_->getAvailableGenerators();
  std::vector<std::string> searches = problem_manager_->getAvailableSearches();

  bool has_generators = !generators.empty();
  bool has_searches = !searches.empty();

  // Show warning only once when we detect there are no algorithms
  static bool already_shown_warning = false;
  if (!has_generators && !has_searches && !already_shown_warning) {
    show_no_algorithm_warning_ = true;
    already_shown_warning = true;
  }

  ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Algorithm Selection");
  ImGui::Separator();

  if (ImGui::BeginCombo(
        "Solution Generator",
        problem_manager_->getSelectedGenerator().empty()
          ? "Select Generator"
          : problem_manager_->getSelectedGenerator().c_str(),
        has_generators ? 0 : ImGuiComboFlags_NoArrowButton
      )) {
    if (has_generators) {
      for (const auto& gen : generators) {
        bool is_selected = gen == problem_manager_->getSelectedGenerator();
        if (ImGui::Selectable(gen.c_str(), is_selected)) {
          problem_manager_->setSelectedGenerator(gen);
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
    } else {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No generators available");
    }
    ImGui::EndCombo();
  }

  if (ImGui::BeginCombo(
        "Local Search",
        problem_manager_->getSelectedSearch().empty()
          ? "Select Search"
          : problem_manager_->getSelectedSearch().c_str(),
        has_searches ? 0 : ImGuiComboFlags_NoArrowButton
      )) {
    if (has_searches) {
      for (const auto& search : searches) {
        bool is_selected = search == problem_manager_->getSelectedSearch();
        if (ImGui::Selectable(search.c_str(), is_selected)) {
          problem_manager_->setSelectedSearch(search);
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
    } else {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No searches available");
    }
    ImGui::EndCombo();
  }

  if (!has_generators || !has_searches) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
    ImGui::TextWrapped(
      "Warning: Some algorithm components are missing. Make sure algorithms are registered "
      "correctly."
    );
    ImGui::PopStyleColor();
  }

  ImGui::Separator();

  ImGui::BeginDisabled(!problem_manager_->hasValidAlgorithmSelection());
  if (ImGui::Button("Run Algorithm", ImVec2(-FLT_MIN, 0))) {
    problem_manager_->runAlgorithm();
  }
  ImGui::EndDisabled();

  if (!problem_manager_->hasValidAlgorithmSelection()) {
    ImGui::TextColored(
      ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select both a generator and a search algorithm to run"
    );
  }

  ImGui::End();
}

void UIComponents::RenderProblemVisualization() {
  if (!problem_manager_ || !problem_manager_->isProblemLoaded() || !object_manager_)
    return;

  VRPTProblem* problem = problem_manager_->getCurrentProblem();

  object_manager_->ClearObjects();

  float min_x = std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::min();
  float max_y = std::numeric_limits<float>::min();

  const auto& depot = problem->getDepot();
  const auto& landfill = problem->getLandfill();
  const auto& stations = problem->getSWTS();
  const auto& zones = problem->getZones();

  min_x = std::min(min_x, static_cast<float>(depot.x()));
  min_y = std::min(min_y, static_cast<float>(depot.y()));
  max_x = std::max(max_x, static_cast<float>(depot.x()));
  max_y = std::max(max_y, static_cast<float>(depot.y()));

  min_x = std::min(min_x, static_cast<float>(landfill.x()));
  min_y = std::min(min_y, static_cast<float>(landfill.y()));
  max_x = std::max(max_x, static_cast<float>(landfill.x()));
  max_y = std::max(max_y, static_cast<float>(landfill.y()));

  for (const auto& station : stations) {
    min_x = std::min(min_x, static_cast<float>(station.x()));
    min_y = std::min(min_y, static_cast<float>(station.y()));
    max_x = std::max(max_x, static_cast<float>(station.x()));
    max_y = std::max(max_y, static_cast<float>(station.y()));
  }

  for (const auto& zone : zones) {
    min_x = std::min(min_x, static_cast<float>(zone.x()));
    min_y = std::min(min_y, static_cast<float>(zone.y()));
    max_x = std::max(max_x, static_cast<float>(zone.x()));
    max_y = std::max(max_y, static_cast<float>(zone.y()));
  }

  const float padding = 50.0f;
  min_x -= padding;
  min_y -= padding;
  max_x += padding;
  max_y += padding;

  // Add depot with additional data
  auto& depot_obj = object_manager_->AddObject(
    {static_cast<float>(depot.x()), static_cast<float>(depot.y())},
    20.0f,
    BLUE,
    "Depot",
    ObjectShape::PENTAGON
  );
  depot_obj.AddData("Type", "Depot");
  depot_obj.AddData("Description", "Starting point for all vehicles");

  // Add landfill with additional data
  auto& landfill_obj = object_manager_->AddObject(
    {static_cast<float>(landfill.x()), static_cast<float>(landfill.y())},
    20.0f,
    DARKBROWN,
    "Landfill",
    ObjectShape::SQUARE
  );
  landfill_obj.AddData("Type", "Landfill");
  landfill_obj.AddData("Description", "Final disposal site for waste");

  // Add transfer stations with additional data
  for (const auto& station : stations) {
    auto& station_obj = object_manager_->AddObject(
      {static_cast<float>(station.x()), static_cast<float>(station.y())},
      20.0f,
      GREEN,
      station.name(),
      ObjectShape::TRIANGLE
    );
    station_obj.AddData("Type", "Transfer Station");
    station_obj.AddData("ID", station.id());
    station_obj.AddData("Name", station.name());
  }

  // Add collection zones with additional data
  for (const auto& zone : zones) {
    float waste = static_cast<float>(zone.wasteAmount().value());
    float size = 10.0f + std::min(15.0f, waste * 0.5f);

    auto& zone_obj = object_manager_->AddObject(
      {static_cast<float>(zone.x()), static_cast<float>(zone.y())},
      size,
      RED,
      zone.name(),
      ObjectShape::CIRCLE
    );
    zone_obj.AddData("Type", "Collection Zone");
    zone_obj.AddData("ID", zone.id());

    // Format waste amount to 2 decimal places
    std::ostringstream waste_ss;
    waste_ss << std::fixed << std::setprecision(2) << zone.wasteAmount().value() << " units";
    zone_obj.AddData("Waste", waste_ss.str());

    // Format service time to 2 decimal places
    std::ostringstream service_time_ss;
    service_time_ss << std::fixed << std::setprecision(2)
                    << zone.serviceTime().value(units::TimeUnit::Seconds) << " seconds";
    zone_obj.AddData("Service Time", service_time_ss.str());
  }
}

void UIComponents::RenderUI() {
  RenderProblemSelector();
  RenderProblemInspector();
  RenderAlgorithmSelector();

  RenderProblemVisualization();

  if (!problem_manager_ || !problem_manager_->isProblemLoaded()) {
    RenderEmptyStateOverlay();
  }

  if (show_no_algorithm_warning_) {
    RenderWarningDialog(
      "No Algorithms Available",
      "No algorithms are registered in the system.\n\nPlease ensure that algorithm plugins are "
      "properly loaded.",
      &show_no_algorithm_warning_
    );
  }
}

}  // namespace visualization
}  // namespace daa