#include <cstdint>

#include "visualization/object_manager.h"

#include "imgui.h"
#include "raymath.h"

ObjectManager::ObjectManager() {}

ObjectManager::~ObjectManager() {}

void ObjectManager::Initialize() {
  SetupDefaultObjects();
}

void ObjectManager::SetupDefaultObjects() {
  objects_.clear();

  // Add some default objects
  AddObject({300, 200}, 40, RED, "Red Circle");
  AddObject({400, 300}, 50, BLUE, "Blue Circle");
  AddObject({500, 250}, 30, GREEN, "Green Circle");
}

void ObjectManager::AddObject(
  const Vector2& position,
  float size,
  Color color,
  const std::string& name
) {
  objects_.emplace_back(position, size, color, name);
}

void ObjectManager::RemoveObject(size_t index) {
  if (index < objects_.size()) {
    objects_.erase(objects_.begin() + index);
  }
}

void ObjectManager::DrawObjects(bool use_transformation, Vector2 offset, float scale) {
  for (auto& obj : objects_) {
    Vector2 pos = obj.position;
    float sz = obj.size;

    if (use_transformation) {
      pos.x = obj.position.x * scale + offset.x;
      pos.y = obj.position.y * scale + offset.y;
      sz *= scale;
    }

    // Draw object at position (the transformation is handled by the camera in Canvas)
    DrawCircleV(pos, sz, obj.color);

    // Draw a small outline to make the object more visible
    DrawCircleLines(pos.x, pos.y, sz, ColorAlpha(WHITE, 0.3f));

    // Optionally draw the object name for better visibility
    if (sz > 20.0f) {
      DrawText(obj.name.c_str(), pos.x - sz / 2, pos.y - 5, 10, WHITE);
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
    bool is_hovered = dist <= obj.size;

    // Start dragging
    if (mouse_down && is_hovered) {
      obj.dragging = true;
    }

    // Stop dragging
    if (mouse_released) {
      obj.dragging = false;
    }

    // Update position if dragging
    if (obj.dragging) {
      obj.position = world_mouse_pos;
    }

    // Display tooltip when hovering
    if (is_hovered) {
      ImGui::BeginTooltip();
      ImGui::Text("%s", obj.name.c_str());
      ImGui::Text("Position: (%.1f, %.1f)", obj.position.x, obj.position.y);
      ImGui::Text("Size: %.1f", obj.size);
      ImGui::EndTooltip();
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
    AddObject(pos, size, color, name);
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
        obj.color = (Color){(unsigned char)(color[0] * 255),
                            (unsigned char)(color[1] * 255),
                            (unsigned char)(color[2] * 255),
                            (unsigned char)(color[3] * 255)};
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
