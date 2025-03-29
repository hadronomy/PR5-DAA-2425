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

  // Draw all objects
  for (size_t i = 0; i < objects_.size(); i++) {
    auto& obj = objects_[i];
    Vector2 pos = transformed_positions[i];
    float sz = transformed_sizes[i];

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
  for (auto& obj : objects_) {
    // Check if mouse is hovering over object
    float dist = Vector2Distance(world_mouse_pos, obj.position);
    // Apply the CANVAS_UNIT_TO_METERS scaling to the object size for hover detection
    bool is_hovered = dist <= obj.size * CANVAS_UNIT_TO_METERS;

    // Display tooltip when hovering
    if (is_hovered) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.8f, 0.8f, 0.3f));
      
      // Set minimum tooltip width to ensure columns display properly
      ImGui::SetNextWindowSizeConstraints(ImVec2(280, 0), ImVec2(FLT_MAX, FLT_MAX));
      ImGui::BeginTooltip();

      // Tooltip title with background
      ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
      ImGui::PushFont(ImGuiThemeManager::GetInstance().GetFont("Geist Mono"));
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "%s", obj.name.c_str());
      ImGui::PopFont();
      ImGui::PopStyleColor(1); // Pop header style

      // Object properties
      ImGui::Columns(2, "ObjectProperties", false);
      ImGui::SetColumnWidth(0, 80);

      ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Position:");
      ImGui::NextColumn();
      ImGui::Text("%.1f, %.1f", obj.position.x, obj.position.y);
      ImGui::NextColumn();

      // Display type-specific data (limit to most important properties)
      const auto& data = obj.GetAllData();
      int property_count = 0;
      for (const auto& item : data) {
        if (property_count >= 3) break; // Limit number of properties to reduce vertical size
        
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s:", item.first.c_str());
        ImGui::NextColumn();
        ImGui::Text("%s", item.second.c_str());
        ImGui::NextColumn();
        property_count++;
      }

      ImGui::Columns(1);
      ImGui::EndTooltip();
      ImGui::PopStyleColor(1); // Pop border style
      ImGui::PopStyleVar(2);
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