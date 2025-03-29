#pragma once

#include <map>
#include <string>
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

 private:
  std::vector<GraphicalObject> objects_;

  // Helper methods for drawing different shapes
  void DrawShape(const Vector2& position, float size, ObjectShape shape, Color color);
};

}  // namespace visualization
}  // namespace daa