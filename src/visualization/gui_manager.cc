
// Include raylib headers only in the implementation file
#include "imgui_internal.h"

#include "raylib.h"
#include "rlImGui.h"

#include "visualization/gui_manager.h"


GuiManager::GuiManager()
    : screen_width_(1280),
      screen_height_(720),
      clear_color_(DARKGRAY),
      show_demo_window_(true),
      show_another_window_(false),
      first_time_(true),
      dockspace_id_(0) {}

GuiManager::~GuiManager() {
  cleanup();
}

bool GuiManager::initialize() {
  // Initialize raylib window
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(screen_width_, screen_height_, "Visualization Window");
  SetTargetFPS(60);  // Set target framerate

  setupImGui();

  return true;
}

void GuiManager::setupImGui() {
  // Setup ImGui context with rlImGui
  rlImGuiSetup(true);

  // Enable docking if available
#ifdef IMGUI_HAS_DOCK
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui::GetIO().ConfigDockingWithShift = true;  // Enable docking with shift key
#endif
}

void GuiManager::run() {
  // Main loop
  while (!WindowShouldClose()) {
    // Start drawing frame
    BeginDrawing();

    // Clear background with the set color
    ClearBackground(clear_color_);

    // Draw anything that should appear behind the ImGui interface
    DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, GetScreenHeight() * 0.45f, DARKGREEN);

    // Start ImGui rendering
    rlImGuiBegin();

    // Create dockspace over the entire window (if docking is available)
#ifdef IMGUI_HAS_DOCK
    ImGui::DockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_PassthruCentralNode);

    if (first_time_) {
      setupDocking();
    }
#endif

    renderMenuBar();
    renderLeftPanel();
    renderRightPanel();
    renderMainWindows();

    // End ImGui rendering
    rlImGuiEnd();

    // End drawing frame
    EndDrawing();
  }
}

void GuiManager::setupDocking() {
  first_time_ = false;

#ifdef IMGUI_HAS_DOCK
  dockspace_id_ = ImGui::GetID("MainWindowDockspace");
  ImGui::DockBuilderRemoveNode(dockspace_id_);
  ImGui::DockBuilderAddNode(dockspace_id_, ImGuiDockNodeFlags_DockSpace);

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::DockBuilderSetNodeSize(dockspace_id_, viewport->Size);

  // Split the dockspace into sections
  ImGuiID dock_main_id = dockspace_id_;
  ImGuiID dock_left_id =
    ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
  ImGuiID dock_right_id =
    ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

  // Dock windows
  ImGui::DockBuilderDockWindow("Left Panel", dock_left_id);
  ImGui::DockBuilderDockWindow("Right Panel", dock_right_id);
  ImGui::DockBuilderDockWindow("Hello, world!", dock_main_id);
  ImGui::DockBuilderDockWindow("Another Window", dock_main_id);
  ImGui::DockBuilderFinish(dockspace_id_);
#endif
}

void GuiManager::renderMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        CloseWindow();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Demo Window", nullptr, &show_demo_window_);
      ImGui::MenuItem("Another Window", nullptr, &show_another_window_);
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

void GuiManager::renderLeftPanel() {
  ImGui::Begin("Left Panel");
  ImGui::Text("Left Side Panel");
  ImGui::Separator();
  ImGui::TextWrapped("This panel could contain navigation, properties, or other controls.");

  if (ImGui::CollapsingHeader("Settings")) {
    float color[4] = {
      clear_color_.r / 255.0f,
      clear_color_.g / 255.0f,
      clear_color_.b / 255.0f,
      clear_color_.a / 255.0f
    };

    if (ImGui::ColorEdit3("Background Color", color)) {
      clear_color_ = (Color){(unsigned char)(color[0] * 255),
                             (unsigned char)(color[1] * 255),
                             (unsigned char)(color[2] * 255),
                             (unsigned char)(color[3] * 255)};
    }
  }

  ImGui::End();
}

void GuiManager::renderRightPanel() {
  ImGui::Begin("Right Panel");
  ImGui::Text("Right Side Panel");
  ImGui::Separator();
  ImGui::TextWrapped("This panel could show details, properties, or other information.");

  // Display some stats
  ImGuiIO& io = ImGui::GetIO();
  ImGui::Text("Performance");
  ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
  ImGui::Text("Raylib FPS: %d", GetFPS());

  ImGui::End();
}

void GuiManager::renderMainWindows() {
  // 1. Show the big demo window
  if (show_demo_window_)
    ImGui::ShowDemoWindow(&show_demo_window_);

  // 2. Show a simple window that we create ourselves
  {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");  // Create a window

    ImGui::Text("This is some useful text.");  // Display some text
    ImGui::Checkbox("Demo Window", &show_demo_window_);
    ImGui::Checkbox("Another Window", &show_another_window_);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

    float color[4] = {
      clear_color_.r / 255.0f,
      clear_color_.g / 255.0f,
      clear_color_.b / 255.0f,
      clear_color_.a / 255.0f
    };

    if (ImGui::ColorEdit3("clear color", color)) {
      clear_color_ = (Color){(unsigned char)(color[0] * 255),
                             (unsigned char)(color[1] * 255),
                             (unsigned char)(color[2] * 255),
                             (unsigned char)(color[3] * 255)};
    }

    if (ImGui::Button("Button"))  // Buttons return true when clicked
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::End();
  }

  // 3. Show another simple window
  if (show_another_window_) {
    ImGui::Begin("Another Window", &show_another_window_);
    ImGui::Text("Hello from another window!");
    if (ImGui::Button("Close Me"))
      show_another_window_ = false;
    ImGui::End();
  }
}

void GuiManager::cleanup() {
  // Shutdown ImGui context with rlImGui
  rlImGuiShutdown();

  // Close raylib window
  CloseWindow();
}

float GuiManager::scaleToDPI(float value) {
  return GetWindowScaleDPI().x * value;
}

int GuiManager::scaleToDPI(int value) {
  return int(GetWindowScaleDPI().x * value);
}