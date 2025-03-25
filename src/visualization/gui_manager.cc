#include "visualization/gui_manager.h"

GuiManager::GuiManager()
    : window_(nullptr),
      glsl_version_("#version 130"),
      clear_color_(ImVec4(0.45f, 0.55f, 0.60f, 1.00f)),
      show_demo_window_(true),
      show_another_window_(false),
      first_time_(true),
      dockspace_id_(0) {}

GuiManager::~GuiManager() {
  cleanup();
}

bool GuiManager::initialize() {
  // Set up error callback
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return false;

  // Decide GL+GLSL versions
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // Create window with graphics context
  window_ = glfwCreateWindow(1280, 720, "Visualization Window", nullptr, nullptr);
  if (window_ == nullptr) {
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(window_);
  glfwSwapInterval(1);  // Enable vsync

  setupImGui();

  return true;
}

void GuiManager::setupImGui() {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
  io.ConfigDockingWithShift = true;                      // Enable docking with shift key

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window_, true);
  ImGui_ImplOpenGL3_Init(glsl_version_);
}

void GuiManager::run() {
  // Main loop
  while (!glfwWindowShouldClose(window_)) {
    // Poll and handle events (inputs, window resize, etc.)
    glfwPollEvents();
    if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) != 0) {
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
    dockspace_id_ = ImGui::GetID("MainWindowDockspace");
    ImGui::DockSpace(dockspace_id_, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (first_time_) {
      setupDocking(viewport);
    }

    renderMenuBar();
    ImGui::End();  // End DockSpace

    renderLeftPanel();
    renderRightPanel();
    renderMainWindows();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(
      clear_color_.x * clear_color_.w,
      clear_color_.y * clear_color_.w,
      clear_color_.z * clear_color_.w,
      clear_color_.w
    );
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);
  }
}

void GuiManager::setupDocking(const ImGuiViewport* viewport) {
  first_time_ = false;
  ImGui::DockBuilderRemoveNode(dockspace_id_);
  ImGui::DockBuilderAddNode(dockspace_id_, ImGuiDockNodeFlags_DockSpace);
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
}

void GuiManager::renderMenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        glfwSetWindowShouldClose(window_, true);
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Demo Window", nullptr, &show_demo_window_);
      ImGui::MenuItem("Another Window", nullptr, &show_another_window_);
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }
}

void GuiManager::renderLeftPanel() {
  ImGui::Begin("Left Panel");
  ImGui::Text("Left Side Panel");
  ImGui::Separator();
  ImGui::TextWrapped("This panel could contain navigation, properties, or other controls.");

  if (ImGui::CollapsingHeader("Settings")) {
    ImGui::ColorEdit3("Background Color", (float*)&clear_color_);
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
    ImGui::ColorEdit3("clear color", (float*)&clear_color_);

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
  if (window_ != nullptr) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();

    window_ = nullptr;
  }
}

void GuiManager::glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}