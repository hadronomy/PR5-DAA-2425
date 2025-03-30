#include <cstdint>

#include "visualization/canvas.h"
#include "visualization/imgui_theme.h"
#include "visualization/object_manager.h"

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
  const std::string& label
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

  // Add the line object to our collection
  objects_.push_back(line);
}

void ObjectManager::AddDashedLine(
  const Vector2& start,
  const Vector2& end,
  Color color,
  float thickness,
  float dash_length,
  const std::string& label
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

  // Add the line object to our collection
  objects_.push_back(line);
}

void ObjectManager::DrawObjects(bool use_transformation, Vector2 offset, float scale) {
  // First, collect all transformed positions for overlap detection
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

      // Calculate a thickness that remains visible when zoomed out with reduced values
      float adjustedThickness = use_transformation
                                ? fmaxf(obj.size / (scale > 0.01f ? scale : 0.01f), 15.0f)
                                : fmaxf(obj.size, 15.0f);

      // Reduced cap on maximum thickness
      adjustedThickness = fminf(adjustedThickness, 250.0f);

      // Draw the line with adjusted thickness
      DrawLineEx({startX, startY}, {endX, endY}, adjustedThickness, obj.color);
    }
    // Draw dashed line objects
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

      // Also adjust dash length based on zoom, with reduced minimum values
      float adjustedDashLength =
        use_transformation ? fmaxf(dashLength * scale, 8.0f) : fmaxf(dashLength, 8.0f);

      // Draw dashed line
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

        // Draw with adjusted thickness
        DrawLineEx(dashStart, dashEnd, adjustedThickness, obj.color);
      }
    }
  }

  // SECOND PASS: Draw all shapes
  for (size_t i = 0; i < objects_.size(); i++) {
    auto& obj = objects_[i];
    Vector2 pos = transformed_positions[i];
    float sz = transformed_sizes[i];

    // Skip line objects in this pass
    if (obj.HasData("type") &&
        (obj.GetData("type") == "line" || obj.GetData("type") == "dashed_line")) {
      continue;
    }

    // Draw the object shape
    DrawShape(pos, sz, obj.shape, obj.color);

    // Draw outline
    DrawCircleLines(pos.x, pos.y, sz, ColorAlpha(WHITE, 0.3f));

    // For overlapping objects, draw a warning indicator
    if (has_overlap[i]) {
      // Create a pulsing effect
      float pulse = (sinf(GetTime() * 4.0f) + 1.0f) * 0.5f;
      Color overlap_color = ColorAlpha(RED, 0.2f + pulse * 0.3f);

      // Draw concentric warning circles
      DrawCircleLines(pos.x, pos.y, sz * 1.2f, overlap_color);
      DrawCircleLines(pos.x, pos.y, sz * 1.4f, overlap_color);

      // Draw warning text above the object
      const char* warning_text = "Overlapping!";
      float text_width = MeasureText(warning_text, 10);
      DrawText(warning_text, pos.x - text_width / 2, pos.y - sz - 15, 10, RED);
    }

    // Draw the object name if size is large enough
    if (sz > 15.0f) {
      DrawText(obj.name.c_str(), pos.x - sz / 2, pos.y - sz - 10, 10, WHITE);
    }
  }
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

void ObjectManager::HandleObjectInteraction(
  const Vector2& world_mouse_pos,
  bool mouse_down,
  bool mouse_released
) {
  // Calculate hover state for each object
  for (auto& obj : objects_) {
    // Check if mouse is hovering over the object
    const float distance = Vector2Distance(world_mouse_pos, obj.position);
    const float hover_radius = obj.size * CANVAS_UNIT_TO_METERS;
    const bool is_hovered = distance <= hover_radius;

    // Only show tooltip when object is hovered
    if (!is_hovered)
      continue;

    // ╭──────────────────────────────────────────╮
    // │ Enhanced Tooltip Configuration           │
    // ╰──────────────────────────────────────────╯

    // Create a more attractive tooltip style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.11f, 0.12f, 0.14f, 0.94f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.8f, 0.8f, 0.25f));

    // Set tooltip width constraints
    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 0), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::BeginTooltip();

    // ╭──────────────────────────────────────────╮
    // │ Enhanced Title with Decoration           │
    // ╰──────────────────────────────────────────╯
    ImGui::PushFont(ImGuiThemeManager::GetInstance().GetFont("Geist Mono"));

    // Title background with gradient effect
    const float title_padding = 8.0f;
    ImVec2 title_min = ImGui::GetCursorScreenPos();
    ImVec2 title_max = {
      title_min.x + ImGui::GetContentRegionAvail().x,
      title_min.y + ImGui::GetFontSize() + title_padding * 2
    };

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(title_min, title_max, ImColor(0.18f, 0.2f, 0.25f, 0.9f), 5.0f);

    // Add gradient overlay to title
    draw_list->AddRectFilledMultiColor(
      title_min,
      title_max,
      ImColor(0.4f, 0.5f, 0.7f, 0.3f),
      ImColor(0.2f, 0.3f, 0.5f, 0.1f),
      ImColor(0.2f, 0.3f, 0.5f, 0.1f),
      ImColor(0.4f, 0.5f, 0.7f, 0.3f)
    );

    // Title text with sparkle
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + title_padding);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
    ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.7f, 1.0f), "* %s", obj.name.c_str());
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + title_padding - 2.0f);
    ImGui::PopFont();

    // Fancy separator
    ImGui::Dummy(ImVec2(0, 4));
    const ImVec2 sep_pos = ImGui::GetCursorScreenPos();
    const float sep_width = ImGui::GetContentRegionAvail().x;
    draw_list->AddRectFilled(
      ImVec2(sep_pos.x, sep_pos.y),
      ImVec2(sep_pos.x + sep_width, sep_pos.y + 1),
      ImColor(0.6f, 0.7f, 1.0f, 0.4f)
    );
    draw_list->AddRectFilled(
      ImVec2(sep_pos.x, sep_pos.y + 1),
      ImVec2(sep_pos.x + sep_width, sep_pos.y + 2),
      ImColor(0.3f, 0.4f, 0.7f, 0.2f)
    );
    ImGui::Dummy(ImVec2(0, 6));

    // ╭──────────────────────────────────────────╮
    // │ Enhanced Properties Display              │
    // ╰──────────────────────────────────────────╯
    ImGui::Columns(2, "ObjectProperties", false);
    ImGui::SetColumnWidth(0, 100);

    // Property display with icons
    ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 0.85f), "[P] Position:");
    ImGui::NextColumn();
    ImGui::TextColored(
      ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%.1f, %.1f", obj.position.x, obj.position.y
    );
    ImGui::NextColumn();

    // Display important properties with icons
    const char* property_icons[] = {"[S]", "[C]", "[T]", "[#]"};
    const auto& data = obj.GetAllData();
    int property_count = 0;

    for (const auto& [key, value] : data) {
      ImGui::TextColored(
        ImVec4(0.7f, 0.85f, 1.0f, 0.85f), "%s %s:", property_icons[property_count % 4], key.c_str()
      );
      ImGui::NextColumn();
      ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", value.c_str());
      ImGui::NextColumn();
      property_count++;
    }

    // Bottom decoration
    ImGui::Columns(1);
    ImGui::Dummy(ImVec2(0, 3));
    const ImVec2 bottom_pos = ImGui::GetCursorScreenPos();
    draw_list->AddRectFilledMultiColor(
      bottom_pos,
      ImVec2(bottom_pos.x + ImGui::GetWindowWidth() - 24, bottom_pos.y + 1),
      ImColor(0.4f, 0.5f, 0.7f, 0.2f),
      ImColor(0.6f, 0.7f, 0.9f, 0.4f),
      ImColor(0.6f, 0.7f, 0.9f, 0.4f),
      ImColor(0.4f, 0.5f, 0.7f, 0.2f)
    );

    // Cleanup
    ImGui::EndTooltip();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
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