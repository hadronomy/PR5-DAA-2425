#include <cstdint>

#include "raylib.h"
#include "visualization/canvas.h"
#include "visualization/imgui_theme.h"
#include "visualization/object_manager.h"
#include "visualization/ui_icons.h"
#include "visualization/ui_theme_colors.h"

#include "imgui.h"
#include "raymath.h"

namespace daa {
namespace visualization {

ObjectManager::ObjectManager() {}

ObjectManager::~ObjectManager() {}

void ObjectManager::Initialize() {
  SetupDefaultObjects();
}

void ObjectManager::SetupDefaultObjects() {
  objects_.clear();
}

GraphicalObject& ObjectManager::AddObject(
  const Vector2& position,
  float size,
  Color color,
  const std::string& name,
  ObjectShape shape
) {
  objects_.emplace_back(position, size, color, name, shape);
  return objects_.back();
}

void ObjectManager::RemoveObject(size_t index) {
  if (index < objects_.size()) {
    objects_.erase(objects_.begin() + index);
  }
}

void ObjectManager::ClearObjects() {
  objects_.clear();
}

void ObjectManager::AddLine(
  const Vector2& start,
  const Vector2& end,
  Color color,
  float thickness,
  const std::string& label,
  const std::string& group_id
) {
  // Create a line object at the midpoint
  GraphicalObject line(
    {(start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f},  // position at midpoint
    thickness,                                             // use thickness as size
    color,
    label
  );

  // Store line properties as data
  line.AddData("type", "line");
  line.AddData("startX", std::to_string(start.x));
  line.AddData("startY", std::to_string(start.y));
  line.AddData("endX", std::to_string(end.x));
  line.AddData("endY", std::to_string(end.y));

  // Store group ID for route identification
  line.group_id = group_id;

  // Add the line object to our collection
  objects_.push_back(line);
}

void ObjectManager::AddDashedLine(
  const Vector2& start,
  const Vector2& end,
  Color color,
  float thickness,
  float dash_length,
  const std::string& label,
  const std::string& group_id
) {
  // Similar to AddLine, but we'll mark it as a dashed line
  GraphicalObject line(
    {(start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f},  // position at midpoint
    thickness,                                             // use thickness as size
    color,
    label
  );

  // Store line properties as data
  line.AddData("type", "dashed_line");
  line.AddData("startX", std::to_string(start.x));
  line.AddData("startY", std::to_string(start.y));
  line.AddData("endX", std::to_string(end.x));
  line.AddData("endY", std::to_string(end.y));
  line.AddData("dashLength", std::to_string(dash_length));

  // Store group ID for route identification
  line.group_id = group_id;

  // Add the line object to our collection
  objects_.push_back(line);
}

void ObjectManager::AssociateNodeWithGroup(
  const std::string& node_id,
  const std::string& group_id
) {
  node_to_group_map_[node_id].insert(group_id);
}

void ObjectManager::ClearNodeGroupAssociations() {
  node_to_group_map_.clear();
}

void ObjectManager::ClearNodeGroupAssociations(const std::string& group_id) {
  // Iterate through all node entries
  for (auto& [node_id, groups] : node_to_group_map_) {
    // Remove the specified group from this node's set of groups
    groups.erase(group_id);
  }

  // Clean up empty entries
  for (auto it = node_to_group_map_.begin(); it != node_to_group_map_.end();) {
    if (it->second.empty()) {
      it = node_to_group_map_.erase(it);
    } else {
      ++it;
    }
  }
}

void ObjectManager::SetSelectedRouteGroup(const std::string& group_id) {
  // If we're selecting a different group, clear any previous selection
  if (selected_group_id_ != group_id) {
    // Store the previous group ID before changing it
    std::string previous_group_id = selected_group_id_;

    // Update the selected group ID
    selected_group_id_ = group_id;

    // Reset hover effects for all objects to ensure proper visibility
    for (auto& obj : objects_) {
      obj.hover_alpha = 1.0f;
    }
  }
}

void ObjectManager::ClearSelectedRouteGroup() {
  // Clear the selected group ID
  selected_group_id_ = "";

  // Reset hover effects for all objects
  for (auto& obj : objects_) {
    obj.hover_alpha = 1.0f;
  }
}

bool ObjectManager::IsPointNearLine(
  const Vector2& point,
  const Vector2& line_start,
  const Vector2& line_end,
  float threshold
) {
  // Calculate distance from point to line
  // Algorithm from:
  // https://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment

  // Line vector
  float line_length_sq = Vector2DistanceSqr(line_start, line_end);
  if (line_length_sq == 0.0f)
    return Vector2Distance(point, line_start) <= threshold;

  // Project point onto line using dot product
  float t = ((point.x - line_start.x) * (line_end.x - line_start.x) +
             (point.y - line_start.y) * (line_end.y - line_start.y)) /
            line_length_sq;

  // Clamp t to [0,1] to get point on line segment
  t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);

  // Find the closest point on the line segment
  Vector2 closest = {
    line_start.x + t * (line_end.x - line_start.x), line_start.y + t * (line_end.y - line_start.y)
  };

  // Check if the distance is within threshold
  return Vector2Distance(point, closest) <= threshold;
}

void ObjectManager::HandleObjectInteraction(
  const Vector2& world_mouse_pos,
  bool mouse_down,
  bool mouse_released
) {
  // Reset previous hover state if needed
  std::string new_hovered_group_id = "";
  float closest_distance = 100.0f;  // Reasonable threshold for hover detection

  // First, check for lines/routes (they have priority for hover)
  for (auto& obj : objects_) {
    if (obj.HasData("type") &&
        (obj.GetData("type") == "line" || obj.GetData("type") == "dashed_line")) {

      // Extract line endpoints
      float startX = std::stof(obj.GetData("startX"));
      float startY = std::stof(obj.GetData("startY"));
      float endX = std::stof(obj.GetData("endX"));
      float endY = std::stof(obj.GetData("endY"));

      // Vector2 for calculations
      Vector2 start = {startX, startY};
      Vector2 end = {endX, endY};

      // Calculate hover effect range based on line thickness
      float hover_threshold = obj.size * 3.0f;  // 3x line thickness for comfortable hovering

      // Check if mouse is near the line segment (not just the midpoint)
      if (IsPointNearLine(world_mouse_pos, start, end, hover_threshold)) {
        // Found a hover - use the group_id if available
        if (!obj.group_id.empty()) {
          // Calculate actual distance to line for priority (closer line wins)
          // We need to find projection of point onto line
          Vector2 line_vec = {end.x - start.x, end.y - start.y};
          float line_length_sq = line_vec.x * line_vec.x + line_vec.y * line_vec.y;

          if (line_length_sq > 0) {
            float t = ((world_mouse_pos.x - start.x) * line_vec.x +
                       (world_mouse_pos.y - start.y) * line_vec.y) /
                      line_length_sq;
            t = Clamp(t, 0.0f, 1.0f);

            Vector2 projection = {start.x + t * line_vec.x, start.y + t * line_vec.y};

            float dist = Vector2Distance(world_mouse_pos, projection);

            if (dist < closest_distance) {
              closest_distance = dist;
              new_hovered_group_id = obj.group_id;
            }
          }
        }
      }
    }
  }

  // Now check for hover on nodes
  for (auto& obj : objects_) {
    // Skip lines - we already checked them
    if (obj.HasData("type") &&
        (obj.GetData("type") == "line" || obj.GetData("type") == "dashed_line")) {
      continue;
    }

    // Check if mouse is hovering over the object
    const float distance = Vector2Distance(world_mouse_pos, obj.position);
    const float hover_radius = obj.size * CANVAS_UNIT_TO_METERS;
    const bool is_hovered = distance <= hover_radius;

    if (is_hovered) {
      // If we have a node ID and it's associated with groups, use those groups
      if (obj.HasData("ID")) {
        std::string node_id = obj.GetData("ID");
        auto it = node_to_group_map_.find(node_id);
        if (it != node_to_group_map_.end() && !it->second.empty()) {
          // Just use the first group for now (node could be in multiple routes)
          new_hovered_group_id = *it->second.begin();
          break;
        }
      }

      // Only show tooltip when object is hovered
      // Enhanced Tooltip Configuration
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
      // Use theme colors for popup background and border
      ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyle().Colors[ImGuiCol_PopupBg]);
      ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyle().Colors[ImGuiCol_Border]);

      ImGui::SetNextWindowSizeConstraints(ImVec2(320, 0), ImVec2(FLT_MAX, FLT_MAX));
      ImGui::BeginTooltip();

      ImGui::PushFont(ImGuiThemeManager::GetInstance().GetFont("Geist Mono"));

      const float title_padding = 8.0f;
      ImVec2 title_min = ImGui::GetCursorScreenPos();
      ImVec2 title_max = {
        title_min.x + ImGui::GetContentRegionAvail().x,
        title_min.y + ImGui::GetFontSize() + title_padding * 2
      };

      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      // Use theme colors for title background
      ImVec4 headerBg = ImGui::GetStyle().Colors[ImGuiCol_TitleBg];
      draw_list->AddRectFilled(
        title_min, title_max, ImGui::ColorConvertFloat4ToU32(headerBg), 5.0f
      );

      // Use a subtle gradient based on the header color
      ImVec4 headerActive = ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive];
      ImVec4 gradientStart =
        ImVec4(headerActive.x + 0.05f, headerActive.y + 0.05f, headerActive.z + 0.05f, 0.3f);
      ImVec4 gradientEnd = ImVec4(headerBg.x + 0.02f, headerBg.y + 0.02f, headerBg.z + 0.02f, 0.1f);

      draw_list->AddRectFilledMultiColor(
        title_min,
        title_max,
        ImGui::ColorConvertFloat4ToU32(gradientStart),
        ImGui::ColorConvertFloat4ToU32(gradientEnd),
        ImGui::ColorConvertFloat4ToU32(gradientEnd),
        ImGui::ColorConvertFloat4ToU32(gradientStart)
      );

      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + title_padding);
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
      ImGui::TextColored(theme::GetAccentTextColor(), "* %s", obj.name.c_str());
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + title_padding - 2.0f);
      ImGui::PopFont();

      ImGui::Dummy(ImVec2(0, 4));
      const ImVec2 sep_pos = ImGui::GetCursorScreenPos();
      const float sep_width = ImGui::GetContentRegionAvail().x;
      // Use theme colors for separator
      ImVec4 sepColor = ImGui::GetStyle().Colors[ImGuiCol_Separator];
      ImVec4 sepColorDim = ImVec4(sepColor.x, sepColor.y, sepColor.z, sepColor.w * 0.4f);

      draw_list->AddRectFilled(
        ImVec2(sep_pos.x, sep_pos.y),
        ImVec2(sep_pos.x + sep_width, sep_pos.y + 1),
        ImGui::ColorConvertFloat4ToU32(sepColor)
      );
      draw_list->AddRectFilled(
        ImVec2(sep_pos.x, sep_pos.y + 1),
        ImVec2(sep_pos.x + sep_width, sep_pos.y + 2),
        ImGui::ColorConvertFloat4ToU32(sepColorDim)
      );
      ImGui::Dummy(ImVec2(0, 6));

      ImGui::Columns(2, "ObjectProperties", false);
      ImGui::SetColumnWidth(0, 100);

      ImGui::TextColored(theme::GetHeaderTextColor(), "[P] Position:");
      ImGui::NextColumn();
      ImGui::TextColored(
        theme::GetPrimaryTextColor(), "%.1f, %.1f", obj.position.x, obj.position.y
      );
      ImGui::NextColumn();

      const char* property_icons[] = {"[S]", "[C]", "[T]", "[#]"};
      const auto& data = obj.GetAllData();
      int property_count = 0;

      for (const auto& [key, value] : data) {
        ImGui::TextColored(
          theme::GetHeaderTextColor(), "%s %s:", property_icons[property_count % 4], key.c_str()
        );
        ImGui::NextColumn();
        ImGui::TextColored(theme::GetPrimaryTextColor(), "%s", value.c_str());
        ImGui::NextColumn();
        property_count++;
      }

      ImGui::Columns(1);
      ImGui::Dummy(ImVec2(0, 3));
      const ImVec2 bottom_pos = ImGui::GetCursorScreenPos();
      // Use theme colors for bottom separator with gradient
      ImVec4 bottomSepColor = ImGui::GetStyle().Colors[ImGuiCol_Separator];
      ImVec4 bottomSepColorBright = ImVec4(
        bottomSepColor.x + 0.1f,
        bottomSepColor.y + 0.1f,
        bottomSepColor.z + 0.1f,
        bottomSepColor.w * 0.4f
      );
      ImVec4 bottomSepColorDim =
        ImVec4(bottomSepColor.x, bottomSepColor.y, bottomSepColor.z, bottomSepColor.w * 0.2f);

      draw_list->AddRectFilledMultiColor(
        bottom_pos,
        ImVec2(bottom_pos.x + ImGui::GetWindowWidth() - 24, bottom_pos.y + 1),
        ImGui::ColorConvertFloat4ToU32(bottomSepColorDim),
        ImGui::ColorConvertFloat4ToU32(bottomSepColorBright),
        ImGui::ColorConvertFloat4ToU32(bottomSepColorBright),
        ImGui::ColorConvertFloat4ToU32(bottomSepColorDim)
      );

      ImGui::EndTooltip();
      ImGui::PopStyleColor(2);
      ImGui::PopStyleVar(3);
    }
  }

  // Update the hover state if changed
  if (new_hovered_group_id != hovered_group_id_) {
    hovered_group_id_ = new_hovered_group_id;
  }

  // If we have hover on a route, show tooltip with route information
  if (!hovered_group_id_.empty()) {
    // Find a representative object from the group for info
    for (const auto& obj : objects_) {
      if (obj.group_id == hovered_group_id_ && obj.HasData("type") &&
          (obj.GetData("type") == "line" || obj.GetData("type") == "dashed_line")) {

        // Extract route name from the first matching object
        std::string route_name = obj.name;

        // Show route information tooltip
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
        // Use theme colors for popup background and border
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyle().Colors[ImGuiCol_PopupBg]);
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyle().Colors[ImGuiCol_Border]);

        ImGui::BeginTooltip();

        // With merge mode enabled, we can use the regular font for icons
        ImFont* regularFont = GetThemeManager().GetFont("Geist Mono");

        // Route name with icon
        ImGui::PushStyleColor(ImGuiCol_Text, theme::ColorToU32(theme::GetAccentTextColor()));
        if (regularFont) {
          ImGui::PushFont(regularFont);
          ImGui::Text("%s", icons::ROUTE);
          ImGui::PopFont();
          ImGui::SameLine(0, 5);  // Add 5 pixels of spacing after the icon
        }
        ImGui::Text("%s", route_name.c_str());
        ImGui::PopStyleColor();

        // Instruction with icon
        if (regularFont) {
          ImGui::PushFont(regularFont);
          ImGui::Text("%s", icons::INFO_ICON);
          ImGui::PopFont();
          ImGui::SameLine(0, 5);  // Add 5 pixels of spacing after the icon
        }
        ImGui::TextColored(
          theme::GetPrimaryTextColor(), "Hover over route segments to highlight the route"
        );
        ImGui::EndTooltip();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);

        break;
      }
    }
  }
}

void ObjectManager::UpdateHoverEffects(float delta_time) {
  // If we have a selected route, prioritize it over hover
  std::string priority_group = !selected_group_id_.empty() ? selected_group_id_ : hovered_group_id_;

  // Process each object
  for (auto& obj : objects_) {
    // Target alpha - prioritize selection over hover
    float target_alpha = 1.0f;

    // Check if this object is part of the priority group
    bool is_in_priority_group = false;

    // Direct group membership check
    if (obj.group_id == priority_group) {
      is_in_priority_group = true;
    }
    // For nodes, check if they're associated with the priority group
    else if (!priority_group.empty() && obj.HasData("ID")) {
      std::string node_id = obj.GetData("ID");
      auto it = node_to_group_map_.find(node_id);
      if (it != node_to_group_map_.end() && it->second.find(priority_group) != it->second.end()) {
        is_in_priority_group = true;
      }
    }

    // Set target alpha based on group membership
    if (!priority_group.empty() && !is_in_priority_group) {
      // Make non-selected/non-hovered elements practically invisible
      target_alpha = 0.001f;
    }

    // Smooth transition of alpha
    if (obj.hover_alpha != target_alpha) {
      if (obj.hover_alpha < target_alpha)
        obj.hover_alpha += hover_transition_speed_ * delta_time;
      else
        obj.hover_alpha -= hover_transition_speed_ * delta_time * 3.0f;  // Faster fade out

      // Clamp the alpha value
      if (obj.hover_alpha > 1.0f)
        obj.hover_alpha = 1.0f;
      if (obj.hover_alpha < 0.001f)
        obj.hover_alpha = 0.001f;
    }
  }
}

void ObjectManager::DrawObjects(bool use_transformation, Vector2 offset, float scale) {
  // First, update hover effects
  UpdateHoverEffects(GetFrameTime());

  // Collect transformed positions for overlap detection
  std::vector<Vector2> transformed_positions(objects_.size());
  std::vector<float> transformed_sizes(objects_.size());

  for (size_t i = 0; i < objects_.size(); i++) {
    Vector2 pos = objects_[i].position;
    float sz = objects_[i].size * CANVAS_UNIT_TO_METERS;

    if (use_transformation) {
      pos.x = pos.x * scale + offset.x;
      pos.y = pos.y * scale + offset.y;
      sz *= scale;
    }

    transformed_positions[i] = pos;
    transformed_sizes[i] = sz;
  }

  // Find overlapping objects
  std::vector<bool> has_overlap(objects_.size(), false);
  for (size_t i = 0; i < objects_.size(); i++) {
    for (size_t j = i + 1; j < objects_.size(); j++) {
      float distance = Vector2Distance(transformed_positions[i], transformed_positions[j]);
      float overlap_threshold = std::min(transformed_sizes[i], transformed_sizes[j]);

      if (distance < overlap_threshold) {
        has_overlap[i] = has_overlap[j] = true;
      }
    }
  }

  // FIRST PASS: Draw all lines
  for (size_t i = 0; i < objects_.size(); i++) {
    auto& obj = objects_[i];

    // Skip non-line objects in this pass
    if (!obj.HasData("type") ||
        (obj.GetData("type") != "line" && obj.GetData("type") != "dashed_line")) {
      continue;
    }

    // Use priority group ID (selection takes precedence over hover)
    std::string priority_group =
      !selected_group_id_.empty() ? selected_group_id_ : hovered_group_id_;

    // Only draw if this is the priority group or if no priority group is set
    bool is_priority = priority_group.empty() || obj.group_id == priority_group;

    // For non-priority objects with extremely low alpha, skip drawing completely
    if (!is_priority && obj.hover_alpha < 0.01f) {
      continue;  // Skip drawing this object entirely
    }

    // Apply emphasis effect to selected/hovered routes
    Color line_color = obj.color;

    // For non-priority elements, ensure alpha is very low
    if (!is_priority) {
      line_color.a = static_cast<unsigned char>(fmin(5.0f, line_color.a * obj.hover_alpha));
    } else {
      // For priority elements (selected or hovered), enhance contrast
      line_color.a = 255;  // Full opacity

      // If this route is explicitly selected (not just hovered), make it even more pronounced
      if (!selected_group_id_.empty() && obj.group_id == selected_group_id_) {
        // Brighten the color slightly for selected routes
        line_color.r = static_cast<unsigned char>(fmin(255.0f, line_color.r * 1.2f));
        line_color.g = static_cast<unsigned char>(fmin(255.0f, line_color.g * 1.2f));
        line_color.b = static_cast<unsigned char>(fmin(255.0f, line_color.b * 1.2f));
      }
    }

    // Draw line objects
    if (obj.GetData("type") == "line") {
      // Extract line endpoints
      float startX = std::stof(obj.GetData("startX"));
      float startY = std::stof(obj.GetData("startY"));
      float endX = std::stof(obj.GetData("endX"));
      float endY = std::stof(obj.GetData("endY"));

      // Apply transformations to endpoints if needed
      if (use_transformation) {
        startX = startX * scale + offset.x;
        startY = startY * scale + offset.y;
        endX = endX * scale + offset.x;
        endY = endY * scale + offset.y;
      }

      // Calculate a thickness that remains visible when zoomed out
      float adjustedThickness = use_transformation
                                ? fmaxf(obj.size / (scale > 0.01f ? scale : 0.01f), 15.0f)
                                : fmaxf(obj.size, 15.0f);

      // Reduced cap on maximum thickness
      adjustedThickness = fminf(adjustedThickness, 250.0f);

      // Add brighter highlight for selected routes
      if (!selected_group_id_.empty() && obj.group_id == selected_group_id_) {
        adjustedThickness *= 2.0f;  // Make selected routes much thicker
      } else if (!hovered_group_id_.empty() && obj.group_id == hovered_group_id_) {
        adjustedThickness *= 1.5f;  // Make hovered routes somewhat thicker
      }

      // Draw the line with adjusted thickness and alpha
      DrawLineEx({startX, startY}, {endX, endY}, adjustedThickness, line_color);

      // Calculate direction vector and length
      Vector2 dir = {endX - startX, endY - startY};
      float length = sqrtf(dir.x * dir.x + dir.y * dir.y);

      // Arrow size (proportional to line thickness)
      float arrowSize = adjustedThickness * 3.0f;

      // Draw arrows at intervals along the line
      if (length > 0) {
        // Determine number of arrows based on line length
        // Longer lines get more arrows, with a minimum spacing
        float minSpacing = arrowSize * 6.0f;  // Reduced spacing between arrows
        int numArrows = static_cast<int>(fmaxf(2.0f, length / minSpacing));

        // Maximum number of arrows to avoid overcrowding
        numArrows = fminf(numArrows, 12);

        // Calculate spacing between arrows
        float spacing = length / (numArrows + 1);

        // Draw arrows at calculated intervals
        for (int i = 1; i <= numArrows; i++) {
          float distance = spacing * i;
          DrawArrowAlongLine({startX, startY}, {endX, endY}, distance, arrowSize, line_color);
        }
      }

      // Draw arrow at the end of the line
      // Fills the gap at the end
      DrawCircle(endX, endY, adjustedThickness * 0.5f, line_color);

      // Draw the final arrow at the end
      Vector2 arrowTip = {endX, endY};
      Vector2 perp = {-dir.y / (length > 0 ? length : 1.0f), dir.x / (length > 0 ? length : 1.0f)};
      Vector2 arrowBase1 = {
        endX - dir.x / (length > 0 ? length : 1.0f) * arrowSize + perp.x * arrowSize * 0.5f,
        endY - dir.y / (length > 0 ? length : 1.0f) * arrowSize + perp.y * arrowSize * 0.5f
      };
      Vector2 arrowBase2 = {
        endX - dir.x / (length > 0 ? length : 1.0f) * arrowSize - perp.x * arrowSize * 0.5f,
        endY - dir.y / (length > 0 ? length : 1.0f) * arrowSize - perp.y * arrowSize * 0.5f
      };

      DrawTriangle(arrowBase2, arrowBase1, arrowTip, line_color);
    }
    // Draw dashed line objects with similar hover effects
    else if (obj.GetData("type") == "dashed_line") {
      // Extract line endpoints
      float startX = std::stof(obj.GetData("startX"));
      float startY = std::stof(obj.GetData("startY"));
      float endX = std::stof(obj.GetData("endX"));
      float endY = std::stof(obj.GetData("endY"));
      float dashLength = std::stof(obj.GetData("dashLength"));

      // Apply transformations to endpoints if needed
      if (use_transformation) {
        startX = startX * scale + offset.x;
        startY = startY * scale + offset.y;
        endX = endX * scale + offset.x;
        endY = endY * scale + offset.y;
      }

      // Calculate thickness with reduced values
      float adjustedThickness = use_transformation
                                ? fmaxf(obj.size / (scale > 0.01f ? scale : 0.01f), 20.0f)
                                : fmaxf(obj.size, 20.0f);

      // Reduced cap on maximum thickness
      adjustedThickness = fminf(adjustedThickness, 300.0f);

      // Add brighter highlight for selected routes
      if (!selected_group_id_.empty() && obj.group_id == selected_group_id_) {
        adjustedThickness *= 2.0f;  // Make selected routes much thicker
      } else if (!hovered_group_id_.empty() && obj.group_id == hovered_group_id_) {
        adjustedThickness *= 1.5f;  // Make hovered routes somewhat thicker
      }

      // Also adjust dash length based on zoom, with reduced minimum values
      float adjustedDashLength =
        use_transformation ? fmaxf(dashLength * scale, 8.0f) : fmaxf(dashLength, 8.0f);

      // Draw dashed line with hover alpha
      Vector2 start = {startX, startY};
      Vector2 end = {endX, endY};

      // Calculate the direction and total length
      Vector2 dir = {end.x - start.x, end.y - start.y};
      float length = sqrtf(dir.x * dir.x + dir.y * dir.y);

      // Normalize direction
      if (length > 0) {
        dir.x /= length;
        dir.y /= length;
      }

      // Draw dashed segments
      float dashCount = length / (adjustedDashLength * 2);
      for (float i = 0; i < dashCount; i++) {
        Vector2 dashStart = {
          start.x + dir.x * i * adjustedDashLength * 2, start.y + dir.y * i * adjustedDashLength * 2
        };

        Vector2 dashEnd = {
          dashStart.x + dir.x * adjustedDashLength, dashStart.y + dir.y * adjustedDashLength
        };

        // Make sure we don't exceed the end point
        if (Vector2Distance(start, dashEnd) > length) {
          dashEnd = end;
        }

        // Draw with adjusted thickness and hover alpha
        DrawLineEx(dashStart, dashEnd, adjustedThickness, line_color);
      }

      // Draw arrows at intervals along the dashed line
      // Arrow size (proportional to line thickness)
      float arrowSize = adjustedThickness * 2.5f;

      // Determine number of arrows based on line length
      // For dashed lines, we want fewer arrows than solid lines
      float minSpacing = arrowSize * 8.0f;  // Reduced spacing for better visibility
      int numArrows = static_cast<int>(fmaxf(2.0f, length / minSpacing));

      // Maximum number of arrows to avoid overcrowding
      numArrows = fminf(numArrows, 10);

      // Calculate spacing between arrows
      float spacing = length / (numArrows + 1);

      // Draw arrows at calculated intervals
      for (int i = 1; i <= numArrows; i++) {
        float distance = spacing * i;
        DrawArrowAlongLine(start, end, distance, arrowSize, line_color);
      }

      // Draw final arrow at the end
      if (length > arrowSize * 3) {
        // Draw circle at the end to fill gap
        DrawCircle(end.x, end.y, adjustedThickness * 0.4f, line_color);

        // Draw the final arrow
        Vector2 arrowTip = end;
        Vector2 perp = {-dir.y, dir.x};
        Vector2 arrowBase1 = {
          end.x - dir.x * arrowSize + perp.x * arrowSize * 0.5f,
          end.y - dir.y * arrowSize + perp.y * arrowSize * 0.5f
        };
        Vector2 arrowBase2 = {
          end.x - dir.x * arrowSize - perp.x * arrowSize * 0.5f,
          end.y - dir.y * arrowSize - perp.y * arrowSize * 0.5f
        };

        DrawTriangle(arrowBase2, arrowBase1, arrowTip, line_color);
      }
    }
  }

  // SECOND PASS: Draw all shapes with hover effects
  for (size_t i = 0; i < objects_.size(); i++) {
    auto& obj = objects_[i];
    Vector2 pos = transformed_positions[i];
    float sz = transformed_sizes[i];

    // Skip line objects in this pass
    if (obj.HasData("type") &&
        (obj.GetData("type") == "line" || obj.GetData("type") == "dashed_line")) {
      continue;
    }

    // Use priority group ID (selection takes precedence over hover)
    std::string priority_group =
      !selected_group_id_.empty() ? selected_group_id_ : hovered_group_id_;

    // Check if this node is part of the priority group
    bool is_priority = priority_group.empty() || obj.group_id == priority_group;

    // For nodes, also check if they're associated with the priority group via ID
    if (!is_priority && !priority_group.empty() && obj.HasData("ID")) {
      std::string node_id = obj.GetData("ID");
      auto it = node_to_group_map_.find(node_id);
      if (it != node_to_group_map_.end() && it->second.find(priority_group) != it->second.end()) {
        is_priority = true;
      }
    }

    // For non-priority objects with extremely low alpha, skip drawing completely
    if (!is_priority && obj.hover_alpha < 0.01f) {
      continue;  // Skip drawing this node entirely
    }

    // Apply emphasis effect to selected/hovered nodes
    Color node_color = obj.color;

    // For non-priority elements, ensure alpha is very low
    if (!is_priority) {
      node_color.a = static_cast<unsigned char>(fmin(5.0f, node_color.a * obj.hover_alpha));
    } else {
      // For priority elements (selected or hovered), enhance contrast
      node_color.a = 255;  // Full opacity

      // If this node is explicitly selected (not just hovered), make it even more pronounced
      if (!selected_group_id_.empty() && obj.group_id == selected_group_id_) {
        // Brighten the color slightly for selected nodes
        node_color.r = static_cast<unsigned char>(fmin(255.0f, node_color.r * 1.2f));
        node_color.g = static_cast<unsigned char>(fmin(255.0f, node_color.g * 1.2f));
        node_color.b = static_cast<unsigned char>(fmin(255.0f, node_color.b * 1.2f));
      }
    }

    // Draw the object shape with modified alpha
    DrawShape(pos, sz, obj.shape, node_color);

    // Draw outline with adjusted alpha
    Color outline_color = ColorAlpha(WHITE, 0.3f * obj.hover_alpha);
    DrawCircleLines(pos.x, pos.y, sz, outline_color);

    // For overlapping objects, draw a warning indicator
    if (has_overlap[i]) {
      // Create a pulsing effect
      float pulse = (sinf(GetTime() * 4.0f) + 1.0f) * 0.5f;
      Color overlap_color = ColorAlpha(RED, (0.2f + pulse * 0.3f) * obj.hover_alpha);

      // Draw concentric warning circles
      DrawCircleLines(pos.x, pos.y, sz * 1.2f, overlap_color);
      DrawCircleLines(pos.x, pos.y, sz * 1.4f, overlap_color);

      // Draw warning text above the object with adjusted alpha
      const char* warning_text = "Overlapping!";
      float text_width = MeasureText(warning_text, 10);
      Color warning_color = RED;
      warning_color.a = static_cast<unsigned char>(warning_color.a * obj.hover_alpha);
      DrawText(warning_text, pos.x - text_width / 2, pos.y - sz - 15, 10, warning_color);
    }

    // Draw the object name if size is large enough, with adjusted alpha
    if (sz > 15.0f) {
      Color text_color = WHITE;
      text_color.a = static_cast<unsigned char>(text_color.a * obj.hover_alpha);
      DrawText(obj.name.c_str(), pos.x - sz / 2, pos.y - sz - 10, 10, text_color);
    }
  }
}

void ObjectManager::DrawArrowAlongLine(
  const Vector2& start,
  const Vector2& end,
  float distance_from_start,
  float arrow_size,
  Color color
) {
  // Calculate direction vector from start to end
  Vector2 dir = {end.x - start.x, end.y - start.y};
  float length = sqrtf(dir.x * dir.x + dir.y * dir.y);

  // Normalize direction
  if (length > 0) {
    dir.x /= length;
    dir.y /= length;
  } else {
    return;  // Can't draw arrow on zero-length line
  }

  // Calculate perpendicular vector
  Vector2 perp = {-dir.y, dir.x};

  // Calculate arrow position along the line
  Vector2 arrowPos = {start.x + dir.x * distance_from_start, start.y + dir.y * distance_from_start};

  // Make the arrow more visible by increasing its size
  float actualArrowSize = arrow_size * 0.8f;

  // Calculate arrow points - make it more pronounced
  Vector2 arrowTip = {arrowPos.x + dir.x * actualArrowSize, arrowPos.y + dir.y * actualArrowSize};
  Vector2 arrowBase1 = {
    arrowPos.x - dir.x * actualArrowSize * 0.2f + perp.x * actualArrowSize * 0.7f,
    arrowPos.y - dir.y * actualArrowSize * 0.2f + perp.y * actualArrowSize * 0.7f
  };
  Vector2 arrowBase2 = {
    arrowPos.x - dir.x * actualArrowSize * 0.2f - perp.x * actualArrowSize * 0.7f,
    arrowPos.y - dir.y * actualArrowSize * 0.2f - perp.y * actualArrowSize * 0.7f
  };

  // Draw the arrow triangle
  DrawTriangle(arrowBase2, arrowBase1, arrowTip, color);
}

void ObjectManager::DrawShape(const Vector2& position, float size, ObjectShape shape, Color color) {
  switch (shape) {
    case ObjectShape::CIRCLE:
      DrawCircleV(position, size, color);
      break;

    case ObjectShape::SQUARE: {
      Rectangle rect = {position.x - size, position.y - size, size * 2.0f, size * 2.0f};
      DrawRectangleRec(rect, color);
      break;
    }

    case ObjectShape::TRIANGLE: {
      // Equilateral triangle pointing upward
      Vector2 p1 = {position.x, position.y - size};
      Vector2 p2 = {position.x - size * 0.866f, position.y + size * 0.5f};  // cos(60°), sin(60°)
      Vector2 p3 = {position.x + size * 0.866f, position.y + size * 0.5f};

      DrawTriangle(p1, p2, p3, color);
      break;
    }

    case ObjectShape::PENTAGON: {
      const int sides = 5;
      DrawPoly(position, sides, size, 0.0f, color);
      break;
    }

    case ObjectShape::HEXAGON: {
      const int sides = 6;
      DrawPoly(position, sides, size, 0.0f, color);
      break;
    }
  }
}

void ObjectManager::RenderControlWindow(bool* p_open) {
  ImGui::Begin("Object Controls", p_open);
  ImGui::Text("2D Objects");
  ImGui::Separator();

  // Add new object button
  if (ImGui::Button("Add New Object")) {
    Vector2 pos = {(float)(GetRandomValue(100, 500)), (float)(GetRandomValue(100, 400))};
    float size = (float)(GetRandomValue(20, 60));
    Color color = {
      (unsigned char)GetRandomValue(50, 255),
      (unsigned char)GetRandomValue(50, 255),
      (unsigned char)GetRandomValue(50, 255),
      255
    };
    std::string name = "Object " + std::to_string(objects_.size() + 1);
    ObjectShape shape = static_cast<ObjectShape>(GetRandomValue(0, 4));  // Random shape
    AddObject(pos, size, color, name, shape);
  }

  ImGui::Separator();

  // Object editors
  for (size_t i = 0; i < objects_.size(); i++) {
    auto& obj = objects_[i];
    if (ImGui::TreeNode((void*)(intptr_t)i, "%s", obj.name.c_str())) {
      // Name editor
      char buf[128];
      strncpy(buf, obj.name.c_str(), sizeof(buf) - 1);
      if (ImGui::InputText("Name", buf, IM_ARRAYSIZE(buf))) {
        obj.name = buf;
      }

      // Position editor
      ImGui::DragFloat2("Position", (float*)&obj.position, 1.0f);

      // Size editor
      ImGui::DragFloat("Size", &obj.size, 0.5f, 10.0f, 100.0f);

      // Color editor
      float color[4] = {
        obj.color.r / 255.0f, obj.color.g / 255.0f, obj.color.b / 255.0f, obj.color.a / 255.0f
      };
      if (ImGui::ColorEdit4("Color", color)) {
        obj.color = Color{
          (unsigned char)(color[0] * 255),
          (unsigned char)(color[1] * 255),
          (unsigned char)(color[2] * 255),
          (unsigned char)(color[3] * 255)
        };
      }

      // Shape editor
      const char* shapes[] = {"Circle", "Square", "Triangle", "Pentagon", "Hexagon"};
      int current_shape = static_cast<int>(obj.shape);
      if (ImGui::Combo("Shape", &current_shape, shapes, IM_ARRAYSIZE(shapes))) {
        obj.shape = static_cast<ObjectShape>(current_shape);
      }

      // Delete button
      if (ImGui::Button("Delete")) {
        RemoveObject(i);
        ImGui::TreePop();
        break;
      }

      ImGui::TreePop();
    }
  }

  ImGui::End();
}

}  // namespace visualization
}  // namespace daa