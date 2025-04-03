#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "raylib.h"

namespace daa {
namespace visualization {

// Define shape types for objects
enum class ObjectShape { CIRCLE, SQUARE, TRIANGLE, PENTAGON, HEXAGON };

class GraphicalObject {
 public:
  GraphicalObject(
    const Vector2& position,
    float size,
    Color color,
    const std::string& name,
    ObjectShape shape = ObjectShape::CIRCLE
  )
      : position(position), size(size), color(color), name(name), shape(shape), dragging(false) {}

  // Add additional data for problem-specific information
  void AddData(const std::string& key, const std::string& value) { data_[key] = value; }

  bool HasData(const std::string& key) const { return data_.find(key) != data_.end(); }

  std::string GetData(const std::string& key) const {
    auto it = data_.find(key);
    return (it != data_.end()) ? it->second : "";
  }

  const std::map<std::string, std::string>& GetAllData() const { return data_; }

  Vector2 position;
  float size;
  Color color;
  std::string name;
  ObjectShape shape;
  bool dragging;
  // Add route group identifier and hover state
  std::string group_id;
  float hover_alpha = 1.0f;  // For smooth transitions

 private:
  std::map<std::string, std::string> data_;
};

class ObjectManager {
 public:
  ObjectManager();
  ~ObjectManager();

  void Initialize();
  void SetupDefaultObjects();

  // Enhanced object management
  GraphicalObject& AddObject(
    const Vector2& position,
    float size,
    Color color,
    const std::string& name,
    ObjectShape shape = ObjectShape::CIRCLE
  );
  void RemoveObject(size_t index);
  void ClearObjects();

  // Rendering
  void DrawObjects(bool use_transformation = true, Vector2 offset = {0, 0}, float scale = 1.0f);

  // Interaction
  void
    HandleObjectInteraction(const Vector2& world_mouse_pos, bool mouse_down, bool mouse_released);

  // UI
  void RenderControlWindow(bool* p_open);

  // Access objects
  const std::vector<GraphicalObject>& GetObjects() const { return objects_; }

  // Add a line between two points with group ID for route identification
  void AddLine(
    const Vector2& start,
    const Vector2& end,
    Color color,
    float thickness = 2.0f,
    const std::string& label = "",
    const std::string& group_id = ""
  );

  // Add a dashed line between two points with group ID for route identification
  void AddDashedLine(
    const Vector2& start,
    const Vector2& end,
    Color color,
    float thickness = 2.0f,
    float dash_length = 5.0f,
    const std::string& label = "",
    const std::string& group_id = ""
  );

  // Associate node with a route group
  void AssociateNodeWithGroup(const std::string& node_id, const std::string& group_id);

  // Clear all node-to-group associations
  void ClearNodeGroupAssociations();

  // Clear node-to-group associations for a specific group
  void ClearNodeGroupAssociations(const std::string& group_id);

  // Route selection methods
  void SetSelectedRouteGroup(const std::string& group_id);
  void ClearSelectedRouteGroup();
  const std::string& GetSelectedRouteGroup() const { return selected_group_id_; }

 private:
  std::vector<GraphicalObject> objects_;
  std::string hovered_group_id_;
  std::string selected_group_id_;  // For explicit route selection
  std::unordered_map<std::string, std::unordered_set<std::string>> node_to_group_map_;
  float hover_transition_speed_ = 5.0f;  // Speed of hover effect transition

  // Helper methods for drawing different shapes
  void DrawShape(const Vector2& position, float size, ObjectShape shape, Color color);

  // Helper method to draw an arrow along a line
  void DrawArrowAlongLine(
    const Vector2& start,
    const Vector2& end,
    float distance_from_start,
    float arrow_size,
    Color color
  );

  // Helper method to detect if mouse is near a line
  bool IsPointNearLine(
    const Vector2& point,
    const Vector2& line_start,
    const Vector2& line_end,
    float threshold
  );

  // Apply hover effects to objects based on groups
  void UpdateHoverEffects(float delta_time);
};

}  // namespace visualization
}  // namespace daa