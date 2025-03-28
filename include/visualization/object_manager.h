#pragma once

#include <string>
#include <vector>

#include "raylib.h"

namespace daa {
namespace visualization {

// Forward declarations
class Canvas;

class ObjectManager {
 public:
  struct Object2D {
    Vector2 position;
    float size;
    Color color;
    std::string name;
    bool dragging;

    Object2D(Vector2 pos, float sz, Color col, const std::string& nm)
        : position(pos), size(sz), color(col), name(nm), dragging(false) {}
  };

  ObjectManager();
  ~ObjectManager();

  void Initialize();
  void DrawObjects(bool use_transformation = false, Vector2 offset = {0, 0}, float scale = 1.0f);
  void
    HandleObjectInteraction(const Vector2& world_mouse_pos, bool mouse_down, bool mouse_released);
  void RenderControlWindow(bool* p_open);

  // Object operations
  void AddObject(const Vector2& position, float size, Color color, const std::string& name);
  void RemoveObject(size_t index);
  size_t GetObjectCount() const { return objects_.size(); }
  Object2D& GetObject(size_t index) { return objects_[index]; }

 private:
  std::vector<Object2D> objects_;

  void SetupDefaultObjects();
};

}  // namespace visualization
}  // namespace daa