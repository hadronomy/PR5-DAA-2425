#pragma once

#include <string>
#include <thread>

#include <fmt/core.h>

// ImGui and OpenGL headers
#include <GLFW/glfw3.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "command_handler.h"
#include "command_registry.h"

/**
 * Command handler for visualize command
 * Opens a visualization window using Dear ImGui
 */
class VisualizeCommand : public CommandHandler {
 public:
  VisualizeCommand(bool verbose) : CommandHandler(verbose) {}

  bool execute() override;

  /**
   * Register this command with the command registry
   */
  static void registerCommand(CommandRegistry& registry);

  // ImGui helper functions
  static void glfw_error_callback(int error, const char* description);

 private:
  std::string path_;
};

inline bool VisualizeCommand::execute() {
  // Set up error callback
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return false;

  // Decide GL+GLSL versions
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // Create window with graphics context
  GLFWwindow* window = glfwCreateWindow(1280, 720, "Visualization Window", nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
  io.ConfigDockingWithShift = false;                     // Enable docking without shift key

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Docking layout state
  bool first_time = true;
  ImGuiID dockspace_id = 0;

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    // Poll and handle events (inputs, window resize, etc.)
    glfwPollEvents();
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
      // Wait a bit when window is minimized to avoid high CPU usage
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create dockspace over the entire window
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // Submit the DockSpace
    dockspace_id = ImGui::GetID("MainWindowDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Initialize docking layout on first frame
    if (first_time) {
      first_time = false;
      ImGui::DockBuilderRemoveNode(dockspace_id);
      ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

      // Split the dockspace into sections
      ImGuiID dock_main_id = dockspace_id;
      ImGuiID dock_left_id =
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
      ImGuiID dock_right_id =
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

      // Dock windows
      ImGui::DockBuilderDockWindow("Left Panel", dock_left_id);
      ImGui::DockBuilderDockWindow("Right Panel", dock_right_id);
      ImGui::DockBuilderDockWindow("Hello, world!", dock_main_id);
      ImGui::DockBuilderDockWindow("Another Window", dock_main_id);
      ImGui::DockBuilderFinish(dockspace_id);
    }

    // Optional top menu bar
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit", "Alt+F4")) {
          glfwSetWindowShouldClose(window, true);
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);
        ImGui::MenuItem("Another Window", nullptr, &show_another_window);
        ImGui::EndMenu();
      }

      ImGui::EndMenuBar();
    }

    ImGui::End();  // End DockSpace

    // Left panel
    ImGui::Begin("Left Panel");
    ImGui::Text("Left Side Panel");
    ImGui::Separator();
    ImGui::TextWrapped("This panel could contain navigation, properties, or other controls.");

    if (ImGui::CollapsingHeader("Settings")) {
      ImGui::ColorEdit3("Background Color", (float*)&clear_color);
    }

    ImGui::End();

    // Right panel
    ImGui::Begin("Right Panel");
    ImGui::Text("Right Side Panel");
    ImGui::Separator();
    ImGui::TextWrapped("This panel could show details, properties, or other information.");

    // Display some stats
    ImGui::Text("Performance");
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    ImGui::End();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()!)
    if (show_demo_window)
      ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!"

      ImGui::Text("This is some useful text.");  // Display some text
      ImGui::Checkbox(
        "Demo Window", &show_demo_window
      );  // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider
      ImGui::ColorEdit3("clear color", (float*)&clear_color);  // Edit 3 floats representing a color

      if (ImGui::Button("Button"))  // Buttons return true when clicked
        counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::End();
    }

    // 3. Show another simple window
    if (show_another_window) {
      ImGui::Begin("Another Window", &show_another_window);  // Pass a pointer to our bool variable
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me"))
        show_another_window = false;
      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(
      clear_color.x * clear_color.w,
      clear_color.y * clear_color.w,
      clear_color.z * clear_color.w,
      clear_color.w
    );
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return true;
}

// Static error callback function implementation
inline void VisualizeCommand::glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}