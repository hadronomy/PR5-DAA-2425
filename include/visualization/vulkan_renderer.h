#pragma once

#define IMGUI_IMPL_VULKAN_USE_VOLK
#define VK_NO_PROTOTYPES

#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// #ifdef IMGUI_IMPL_VULKAN_USE_VOLK
// #define VOLK_IMPLEMENTATION
// #include <volk.h>
// #endif

class VulkanRenderer {
 public:
  VulkanRenderer();
  ~VulkanRenderer();

  bool initialize();
  void run();
  void shutdown();

 private:
  // GLFW
  GLFWwindow* window_ = nullptr;

  // Vulkan objects (matching the example's global variables)
  VkAllocationCallbacks* allocator_ = nullptr;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  uint32_t queue_family_ = (uint32_t)-1;
  VkQueue queue_ = VK_NULL_HANDLE;
  VkDebugReportCallbackEXT debug_report_ = VK_NULL_HANDLE;
  VkPipelineCache pipeline_cache_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  ImGui_ImplVulkanH_Window main_window_data_;
  uint32_t min_image_count_ = 2;
  bool swap_chain_rebuild_ = false;

  // UI state
  bool show_demo_window_ = true;
  bool show_another_window_ = false;
  ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Helper functions (exactly matching the example)
  static void GlfwErrorCallback(int error, const char* description);
  static void CheckVkResult(VkResult err);
  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReport(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char* pLayerPrefix,
    const char* pMessage,
    void* pUserData
  );

  bool
    IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension);
  void SetupVulkan(ImVector<const char*>& instance_extensions);
  void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
  void CleanupVulkan();
  void CleanupVulkanWindow();
  void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
  void FramePresent(ImGui_ImplVulkanH_Window* wd);

  VkPhysicalDevice ImGui_ImplVulkanH_SelectPhysicalDevice(VkInstance instance);
  uint32_t ImGui_ImplVulkanH_SelectQueueFamilyIndex(VkPhysicalDevice physical_device);
};