#include "visualization/ui_components.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

#include "tinyfiledialogs.h"

namespace daa {
namespace visualization {

UIComponents::UIComponents(ObjectManager* object_manager)
    : canvas_(nullptr),
      object_manager_(object_manager),
      problem_manager_(nullptr),
      show_problem_selector_(true),
      show_problem_inspector_(true),
      show_algorithm_selector_(true),
      show_no_algorithm_warning_(false),
      show_solution_stats_(true) {}

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

std::string
  UIComponents::OpenFileDialog(const char* title, const char* const* filters, int num_filters) {
  const char* path = tinyfd_openFileDialog(
    title,        // title
    "",           // default path
    num_filters,  // number of filter patterns
    filters,      // filter patterns
    NULL,         // single filter description
    0             // allow multiple selections (0 = no)
  );

  return path ? std::string(path) : std::string();
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

  // Add button for refreshing
  ImGui::SameLine(ImGui::GetWindowWidth() - 140);  // Position for two buttons
  if (ImGui::Button("Add File")) {
    // Use native OS file dialog instead of custom popup
    const char* filters[] = {"*.txt"};
    std::string selected_file = OpenFileDialog("Select Problem File", filters, 1);

    // If a file was selected, add it to the problem manager
    if (!selected_file.empty()) {
      problem_manager_->addProblemFile(selected_file);
    }
  }

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
      for (size_t i = 0; i < available_problems.size(); ++i) {
        const std::string& problem_path = available_problems[i];
        std::string filename = std::filesystem::path(problem_path).filename().string();

        bool is_selected = problem_manager_->isProblemLoaded() &&
                           problem_path == problem_manager_->getCurrentProblemFilename();

        // Check if this is an additional file (not in the main directory)
        bool is_additional = problem_manager_->getAdditionalProblemFiles().count(problem_path) > 0;

        // Create a unique ID by combining the index and full path
        std::string unique_id = "##" + std::to_string(i) + "_" + problem_path;

        // Use colored text for additional files
        if (is_additional) {
          ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 220, 150, 255));  // Light orange
        }

        // Use the unique ID for the selectable but display only the filename
        if (ImGui::Selectable((filename + unique_id).c_str(), is_selected)) {
          problem_manager_->loadProblem(problem_path);
        }

        if (is_additional) {
          ImGui::PopStyleColor();

          // Use a unique ID for the context menu
          std::string context_menu_id = "context_menu_" + std::to_string(i) + "_" + problem_path;

          // Add context menu for additional files to allow removal
          if (ImGui::BeginPopupContextItem(context_menu_id.c_str())) {
            if (ImGui::MenuItem(("Remove from list##" + std::to_string(i)).c_str())) {
              problem_manager_->removeProblemFile(problem_path);
            }
            ImGui::EndPopup();
          }
        }

        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          ImGui::Text("%s", problem_path.c_str());
          if (is_additional) {
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "(Added manually)");
            ImGui::Text("Right-click to remove");
          }
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

void UIComponents::SetSelectedRoute(const std::string& route_type, int route_index) {
  selected_route_ = RouteSelection{route_type, route_index};

  // Update visualization to highlight the selected route
  std::string group_id = route_type + "_route_" + std::to_string(route_index);
  object_manager_->SetSelectedRouteGroup(group_id);

  // Focus the camera on the selected route
  FocusOnSelectedRoute();
}

void UIComponents::ClearSelectedRoute() {
  selected_route_.reset();
  object_manager_->ClearSelectedRouteGroup();
}

void UIComponents::FocusOnSelectedRoute() {
  if (!selected_route_ || !canvas_ || !problem_manager_ || !problem_manager_->hasSolution())
    return;

  Rectangle bounds{0, 0, 0, 0};
  bool has_bounds = false;

  // Get all points in the route to calculate bounds
  std::vector<Vector2> route_points;

  // Find all objects belonging to this route group and collect their positions
  std::string group_id = selected_route_->type + "_route_" + std::to_string(selected_route_->index);

  for (const auto& obj : object_manager_->GetObjects()) {
    if (obj.group_id == group_id) {
      // For lines, add both endpoints
      if (obj.HasData("type") &&
          (obj.GetData("type") == "line" || obj.GetData("type") == "dashed_line")) {
        float startX = std::stof(obj.GetData("startX"));
        float startY = std::stof(obj.GetData("startY"));
        float endX = std::stof(obj.GetData("endX"));
        float endY = std::stof(obj.GetData("endY"));

        route_points.push_back({startX, startY});
        route_points.push_back({endX, endY});
      }
      // For other objects, add their position
      else {
        route_points.push_back(obj.position);
      }
    }
  }

  // Calculate bounds from collected points
  if (!route_points.empty()) {
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();

    for (const auto& point : route_points) {
      min_x = std::min(min_x, point.x);
      min_y = std::min(min_y, point.y);
      max_x = std::max(max_x, point.x);
      max_y = std::max(max_y, point.y);
    }

    bounds = {min_x, min_y, max_x - min_x, max_y - min_y};
    has_bounds = true;
  }

  // Tell canvas to fit the view to these bounds
  if (has_bounds) {
    canvas_->FitViewToBounds(bounds, 0.2f);  // 20% padding
  }
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

  problem_manager_->setSelectedAlgorithm("VRPTSolver");
  problem_manager_->renderAlgorithmConfigurationUI();

  if (ImGui::Button("Run Algorithm", ImVec2(-FLT_MIN, 0))) {
    if (!problem_manager_->runAlgorithm()) {
      show_no_algorithm_warning_ = true;
    }
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

  // Create location ID to coordinate mapping for drawing routes
  std::unordered_map<std::string, Vector2> location_coords;

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
  depot_obj.AddData("ID", depot.id());  // Add ID for route group association

  // Store depot coordinates
  location_coords[depot.id()] = {static_cast<float>(depot.x()), static_cast<float>(depot.y())};

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
  landfill_obj.AddData("ID", landfill.id());  // Add ID for route group association

  // Store landfill coordinates
  location_coords[landfill.id()] = {
    static_cast<float>(landfill.x()), static_cast<float>(landfill.y())
  };

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

    // Store station coordinates
    location_coords[station.id()] = {
      static_cast<float>(station.x()), static_cast<float>(station.y())
    };
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

    // Store zone coordinates
    location_coords[zone.id()] = {static_cast<float>(zone.x()), static_cast<float>(zone.y())};
  }

  // Draw solution routes if available
  if (problem_manager_->hasSolution()) {
    const auto* solution = problem_manager_->getSolution();

    // Draw CV routes
    const auto& cv_routes = solution->getCVRoutes();

    // Generate distinct colors for each CV route based on total count
    for (size_t i = 0; i < cv_routes.size(); ++i) {
      const auto& route = cv_routes[i];
      const auto& locations = route.locationIds();

      if (locations.empty())
        continue;

      // Generate a distinct color for each route
      // Use golden ratio to create well-distributed colors across the hue spectrum (0-240)
      float golden_ratio = 0.618033988749895f;
      float hue = fmodf(i * golden_ratio, 1.0f) * 240.0f;

      // Vary saturation and brightness slightly to create even more distinction
      float saturation = 0.7f + fmodf(i * 0.1f, 0.3f);
      float value = 0.85f + fmodf(i * 0.07f, 0.15f);

      Color route_color = ColorFromHSV(hue, saturation, value);
      route_color.a = 180;  // Add transparency

      // Add route information
      std::ostringstream route_name;
      route_name << "CV Route " << (i + 1);

      // Create unique group ID for this CV route
      std::string group_id = "cv_route_" + std::to_string(i);

      // Add the depot as starting point
      std::string prev_id = problem->getDepot().id();
      Vector2 prev_pos = location_coords[prev_id];

      // Associate depot with this route group
      object_manager_->AssociateNodeWithGroup(prev_id, group_id);

      for (size_t j = 0; j < locations.size(); ++j) {
        const auto& loc_id = locations[j];

        // Skip if location not found in mapping
        if (location_coords.find(loc_id) == location_coords.end())
          continue;

        // Associate this location with the route group
        object_manager_->AssociateNodeWithGroup(loc_id, group_id);

        Vector2 curr_pos = location_coords[loc_id];

        // Draw route segment with group ID for hover detection
        object_manager_->AddLine(
          prev_pos, curr_pos, route_color, 75.0f, route_name.str(), group_id
        );

        prev_id = loc_id;
        prev_pos = curr_pos;
      }

      // Close route back to depot if needed
      if (prev_id != problem->getDepot().id() && !locations.empty()) {
        object_manager_->AddLine(
          prev_pos,
          location_coords[problem->getDepot().id()],
          route_color,
          75.0f,
          route_name.str(),
          group_id
        );
      }
    }

    // Draw TV routes
    const auto& tv_routes = solution->getTVRoutes();
    for (size_t i = 0; i < tv_routes.size(); ++i) {
      const auto& route = tv_routes[i];
      const auto& locations = route.locationIds();

      if (locations.empty())
        continue;

      // Generate distinct colors for TV routes - use a different hue range (240-360)
      // This ensures TV routes are visually different from CV routes
      float golden_ratio = 0.618033988749895f;
      float hue = 240.0f + fmodf(i * golden_ratio, 1.0f) * 120.0f;

      // Vary saturation and brightness slightly
      float saturation = 0.8f + fmodf(i * 0.12f, 0.2f);
      float value = 0.9f + fmodf(i * 0.05f, 0.1f);

      Color route_color = ColorFromHSV(hue, saturation, value);
      route_color.a = 180;  // Add transparency

      // Add route information
      std::ostringstream route_name;
      route_name << "TV Route " << (i + 1);

      // Create unique group ID for this TV route
      std::string group_id = "tv_route_" + std::to_string(i);

      // Add the landfill as starting point
      std::string prev_id = problem->getLandfill().id();
      Vector2 prev_pos = location_coords[prev_id];

      // Associate landfill with this route
      object_manager_->AssociateNodeWithGroup(prev_id, group_id);

      for (size_t j = 0; j < locations.size(); ++j) {
        const auto& loc_id = locations[j];

        // Skip if location not found in mapping
        if (location_coords.find(loc_id) == location_coords.end())
          continue;

        // Associate this location with the route group
        object_manager_->AssociateNodeWithGroup(loc_id, group_id);

        Vector2 curr_pos = location_coords[loc_id];

        // Draw route segment with group ID for hover detection
        object_manager_->AddDashedLine(
          prev_pos, curr_pos, route_color, 100.0f, 15.0f, route_name.str(), group_id
        );

        prev_id = loc_id;
        prev_pos = curr_pos;
      }

      // Close route back to landfill if needed
      if (prev_id != problem->getLandfill().id() && !locations.empty()) {
        object_manager_->AddDashedLine(
          prev_pos,
          location_coords[problem->getLandfill().id()],
          route_color,
          100.0f,
          15.0f,
          route_name.str(),
          group_id
        );
      }
    }
  }
}

void UIComponents::RenderSolutionStatsWindow() {
  if (!problem_manager_ || !problem_manager_->isProblemLoaded() || !problem_manager_->hasSolution())
    return;

  const auto* solution = problem_manager_->getSolution();
  const auto& cv_routes = solution->getCVRoutes();
  const auto& tv_routes = solution->getTVRoutes();
  VRPTProblem* problem = problem_manager_->getCurrentProblem();

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

  ImGui::Begin("Solution Statistics", &show_solution_stats_, window_flags);

  // Main header with algorithm info
  ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 180, 255, 255));
  ImGui::TextWrapped("Algorithm: %s", problem_manager_->getSelectedAlgorithm().c_str());
  ImGui::PopStyleColor();

  ImGui::SameLine(ImGui::GetWindowWidth() - 80);
  if (ImGui::Button("Export")) {
    // TODO: Implement export functionality if needed
  }

  ImGui::Separator();

  // Solution summary section
  if (ImGui::CollapsingHeader("Solution Summary", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::BeginTable("SummaryStats", 2, ImGuiTableFlags_BordersInnerH);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.5f, 1.0f), "Total Routes:");
    ImGui::TableNextColumn();
    ImGui::Text(
      "%zu (CV: %zu, TV: %zu)",
      cv_routes.size() + tv_routes.size(),
      cv_routes.size(),
      tv_routes.size()
    );

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.5f, 1.0f), "Total Waste Collected:");
    ImGui::TableNextColumn();
    ImGui::Text("%.2f units", solution->totalWasteCollected().value());

    // Calculate some overall statistics
    Duration total_cv_time(0.0);
    int zones_visited = 0;
    float total_zones = problem->getNumZones();

    // Count actual collection zones visited and compute other stats
    for (const auto& route : cv_routes) {
      if (!route.isEmpty()) {
        total_cv_time = total_cv_time + route.totalDuration();

        // Count only collection zones (not SWTS or other locations)
        for (const auto& loc_id : route.locationIds()) {
          try {
            const auto& location = problem->getLocation(loc_id);
            if (location.type() == LocationType::COLLECTION_ZONE) {
              zones_visited++;
            }
          } catch (...) {
            // Skip locations that might not be found
          }
        }
      }
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.5f, 1.0f), "Zone Coverage:");
    ImGui::TableNextColumn();
    ImGui::Text(
      "%d of %.0f (%.1f%%)", zones_visited, total_zones, (zones_visited / total_zones) * 100.0f
    );

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.5f, 1.0f), "Total Collection Time:");
    ImGui::TableNextColumn();
    ImGui::Text("%.2f minutes", total_cv_time.value(units::TimeUnit::Minutes));

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.5f, 1.0f), "Solution Complete:");
    ImGui::TableNextColumn();
    if (solution->isComplete()) {
      ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Yes");
    } else {
      ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "No");
    }

    ImGui::EndTable();
  }

  // Collection Vehicle Routes details
  if (ImGui::CollapsingHeader("Collection Vehicle Routes")) {
    if (cv_routes.empty()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No CV routes in solution");
    } else {
      // Table for routes overview - changed from 5 columns to 3 columns
      if (ImGui::BeginTable(
            "CVRoutesTable",
            3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY
          )) {
        ImGui::TableSetupColumn("Route #");
        ImGui::TableSetupColumn("Locations");
        ImGui::TableSetupColumn("Duration");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < cv_routes.size(); i++) {
          const auto& route = cv_routes[i];

          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          // Check if this is the selected route
          bool is_selected = selected_route_ && selected_route_->type == "cv" &&
                             selected_route_->index == static_cast<int>(i);

          // Modified selectable with focus highlighting and selection state
          if (ImGui::Selectable(
                std::to_string(i + 1).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns
              )) {
            // Toggle selection
            if (is_selected) {
              ClearSelectedRoute();
            } else {
              SetSelectedRoute("cv", i);
            }
          }

          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%zu", route.locationIds().size());

          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%.1f min", route.totalDuration().value(units::TimeUnit::Minutes));

          // Detailed route information on hover
          if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            ImGui::BeginTooltip();
            ImGui::Text("Vehicle ID: %s", route.vehicleId().c_str());
            ImGui::Text(
              "Duration: %.2f min / %.2f min (%.1f%%)",
              route.totalDuration().value(units::TimeUnit::Minutes),
              problem->getCVMaxDuration().value(units::TimeUnit::Minutes),
              (route.totalDuration().value() / problem->getCVMaxDuration().value()) * 100.0f
            );

            ImGui::Text(
              "Load: %.2f / %.2f units (%.1f%%)",
              route.currentLoad().value(),
              problem->getCVCapacity().value(),
              (route.currentLoad().value() / problem->getCVCapacity().value()) * 100.0f
            );

            // List of all locations
            if (!route.locationIds().empty()) {
              ImGui::Separator();
              ImGui::Text("Route Path:");
              ImGui::Indent();
              ImGui::Text("Depot → ");
              for (size_t j = 0; j < route.locationIds().size(); j++) {
                const auto& loc_id = route.locationIds()[j];
                const auto& location = problem->getLocation(loc_id);
                ImGui::SameLine(0, 0);

                // Color based on location type
                if (location.type() == LocationType::COLLECTION_ZONE) {
                  ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%s", loc_id.c_str());
                } else if (location.type() == LocationType::SWTS) {
                  ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", loc_id.c_str());
                } else {
                  ImGui::Text("%s", loc_id.c_str());
                }

                if (j < route.locationIds().size() - 1) {
                  ImGui::SameLine(0, 0);
                  ImGui::Text(" → ");
                }
              }
              ImGui::SameLine(0, 0);
              ImGui::Text(" → Depot");
              ImGui::Unindent();
            }
            ImGui::EndTooltip();
          }
        }
        ImGui::EndTable();
      }

      // SWTS Deliveries section
      const auto& deliveries = solution->getAllDeliveryTasks();
      if (!deliveries.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Waste Deliveries at Transfer Stations");

        if (ImGui::BeginTable(
              "DeliveriesTable",
              4,
              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
              ImVec2(0, 150)
            )) {
          ImGui::TableSetupColumn("SWTS");
          ImGui::TableSetupColumn("Arrival Time");
          ImGui::TableSetupColumn("Amount");
          ImGui::TableSetupColumn("% of Capacity");
          ImGui::TableHeadersRow();

          for (const auto& delivery : deliveries) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", delivery.swtsId().c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.1f min", delivery.arrivalTime().value(units::TimeUnit::Minutes));

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f units", delivery.amount().value());

            ImGui::TableSetColumnIndex(3);
            float pct = (delivery.amount().value() / problem->getCVCapacity().value()) * 100.0f;
            ImGui::Text("%.1f%%", pct);
          }
          ImGui::EndTable();
        }
      }
    }
  }

  // Transportation Vehicle Routes details - also update to remove Pickups column
  if (ImGui::CollapsingHeader("Transportation Vehicle Routes")) {
    if (tv_routes.empty()) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No TV routes in solution");
    } else {
      if (ImGui::BeginTable(
            "TVRoutesTable",
            3,  // Changed from 4 to 3 columns (removing Pickups column)
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY
          )) {
        ImGui::TableSetupColumn("Route #");
        ImGui::TableSetupColumn("Locations");
        ImGui::TableSetupColumn("Duration");
        // Removed "Pickups" column
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < tv_routes.size(); i++) {
          const auto& route = tv_routes[i];

          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          // Check if this is the selected route
          bool is_selected = selected_route_ && selected_route_->type == "tv" &&
                             selected_route_->index == static_cast<int>(i);

          // Modified selectable with focusing
          if (ImGui::Selectable(
                std::to_string(i + 1).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns
              )) {
            // Toggle selection
            if (is_selected) {
              ClearSelectedRoute();
            } else {
              SetSelectedRoute("tv", i);
            }
          }

          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%zu", route.locationIds().size());

          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%.1f min", route.currentTime().value(units::TimeUnit::Minutes));

          // Detailed route information on hover - keep tooltip with pickups info
          if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            ImGui::BeginTooltip();
            ImGui::Text("Vehicle ID: %s", route.vehicleId().c_str());
            ImGui::Text(
              "Duration: %.2f min / %.2f min (%.1f%%)",
              route.currentTime().value(units::TimeUnit::Minutes),
              problem->getTVMaxDuration().value(units::TimeUnit::Minutes),
              (route.currentTime().value() / problem->getTVMaxDuration().value()) * 100.0f
            );

            // Keep pickups information in tooltip
            ImGui::Text("Pickups: %zu", route.pickups().size());

            // List of all locations
            if (!route.locationIds().empty()) {
              ImGui::Separator();
              ImGui::Text("Route Path:");
              ImGui::Indent();
              ImGui::Text("Landfill → ");
              for (size_t j = 0; j < route.locationIds().size(); j++) {
                const auto& loc_id = route.locationIds()[j];
                ImGui::SameLine(0, 0);

                // Color landfill differently
                if (loc_id == problem->getLandfill().id()) {
                  ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.4f, 1.0f), "%s", loc_id.c_str());
                } else {
                  ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", loc_id.c_str());
                }

                if (j < route.locationIds().size() - 1) {
                  ImGui::SameLine(0, 0);
                  ImGui::Text(" → ");
                }
              }
              ImGui::Unindent();
            }
            ImGui::EndTooltip();
          }
        }
        ImGui::EndTable();
      }
    }
  }

  // Performance Analysis section
  if (ImGui::CollapsingHeader("Performance Analysis")) {
    // Calculate some statistics for visualization
    float max_duration_pct = 0.0f;
    float avg_duration_pct = 0.0f;
    int routes_count = 0;

    for (const auto& route : cv_routes) {
      if (!route.isEmpty()) {
        float route_duration_pct =
          (route.totalDuration().value() / problem->getCVMaxDuration().value()) * 100.0f;
        max_duration_pct = std::max(max_duration_pct, route_duration_pct);
        avg_duration_pct += route_duration_pct;
        routes_count++;
      }
    }

    if (routes_count > 0) {
      avg_duration_pct /= routes_count;
    }

    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.15f, 1.0f), "Duration Utilization");

    // Progress bar for average duration
    char avg_text[32];
    sprintf(avg_text, "Average: %.1f%%", avg_duration_pct);
    ImGui::ProgressBar(avg_duration_pct / 100.0f, ImVec2(-1, 0), avg_text);

    // Progress bar for max duration
    char max_text[32];
    sprintf(max_text, "Maximum: %.1f%%", max_duration_pct);
    ImGui::ProgressBar(max_duration_pct / 100.0f, ImVec2(-1, 0), max_text);

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.15f, 1.0f), "Solution Efficiency");

    // If we have info on the original problem, calculate how efficient the solution is
    float zones_per_cv =
      static_cast<float>(problem->getNumZones()) / std::max(1, problem->getNumCVVehicles());

    // Fix: Use proper integer division for actual_zones_per_cv calculation
    float efficiency = (zones_per_cv / zones_per_cv) * 100.0f;

    char eff_text[32];
    sprintf(eff_text, "Zones/Vehicle: %.1f%%", efficiency);
    ImGui::ProgressBar(std::min(1.0f, efficiency / 100.0f), ImVec2(-1, 0), eff_text);
  }

  ImGui::End();
}

void UIComponents::RenderUI() {
  // Add a menu bar to allow reopening closed windows
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Windows")) {
      ImGui::MenuItem("Problem Selector", NULL, &show_problem_selector_);
      ImGui::MenuItem("Problem Inspector", NULL, &show_problem_inspector_);
      ImGui::MenuItem("Algorithm Selector", NULL, &show_algorithm_selector_);

      // Only enable this menu item when there's a solution available
      bool has_solution =
        problem_manager_ && problem_manager_->isProblemLoaded() && problem_manager_->hasSolution();
      if (ImGui::MenuItem("Solution Statistics", NULL, &show_solution_stats_, has_solution)) {
        // If clicking to enable and it was disabled, make sure it's visible
        if (show_solution_stats_ && has_solution) {
          // Optionally center or position the window when reopening
          ImGui::SetNextWindowPos(
            ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f)
          );
        }
      }

      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  RenderProblemSelector();
  RenderProblemInspector();
  RenderAlgorithmSelector();

  if (problem_manager_ && problem_manager_->isProblemLoaded() && problem_manager_->hasSolution() &&
      show_solution_stats_) {
    RenderSolutionStatsWindow();
  }

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