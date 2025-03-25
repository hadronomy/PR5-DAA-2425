#include <ctime>
#include <iostream>
#include <vector>

#define IMGUI_IMPL_VULKAN_USE_VOLK

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

#include "visualization/vulkan_renderer.h"

#include <stdio.h>
#include <stdlib.h>

VulkanRenderer::VulkanRenderer() {}

VulkanRenderer::~VulkanRenderer() {
  shutdown();
}

void VulkanRenderer::GlfwErrorCallback(int error, const char* description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void VulkanRenderer::CheckVkResult(VkResult err) {
  if (err == VK_SUCCESS)
    return;

  const char* error_msg = "Unknown error";
  switch (err) {
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      error_msg = "VK_ERROR_OUT_OF_HOST_MEMORY";
      break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      error_msg = "VK_ERROR_OUT_OF_DEVICE_MEMORY";
      break;
    case VK_ERROR_INITIALIZATION_FAILED:
      error_msg = "VK_ERROR_INITIALIZATION_FAILED";
      break;
    case VK_ERROR_DEVICE_LOST:
      error_msg = "VK_ERROR_DEVICE_LOST";
      break;
    case VK_ERROR_MEMORY_MAP_FAILED:
      error_msg = "VK_ERROR_MEMORY_MAP_FAILED";
      break;
    case VK_ERROR_LAYER_NOT_PRESENT:
      error_msg = "VK_ERROR_LAYER_NOT_PRESENT";
      break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      error_msg = "VK_ERROR_EXTENSION_NOT_PRESENT";
      break;
    case VK_ERROR_FEATURE_NOT_PRESENT:
      error_msg = "VK_ERROR_FEATURE_NOT_PRESENT";
      break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      error_msg = "VK_ERROR_INCOMPATIBLE_DRIVER";
      break;
    case VK_ERROR_TOO_MANY_OBJECTS:
      error_msg = "VK_ERROR_TOO_MANY_OBJECTS";
      break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      error_msg = "VK_ERROR_FORMAT_NOT_SUPPORTED";
      break;
    case VK_ERROR_SURFACE_LOST_KHR:
      error_msg = "VK_ERROR_SURFACE_LOST_KHR";
      break;
    case VK_SUBOPTIMAL_KHR:
      error_msg = "VK_SUBOPTIMAL_KHR";
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
      error_msg = "VK_ERROR_OUT_OF_DATE_KHR";
      break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      error_msg = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
      break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      error_msg = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
      break;
    default:
      break;
  }

  fprintf(stderr, "[vulkan] Error: VkResult = %d (%s)\n", err, error_msg);
  if (err < 0)
    abort();
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::DebugReport(
  VkDebugReportFlagsEXT flags,
  VkDebugReportObjectTypeEXT objectType,
  uint64_t object,
  size_t location,
  int32_t messageCode,
  const char* pLayerPrefix,
  const char* pMessage,
  void* pUserData
) {

  (void)flags;
  (void)object;
  (void)location;
  (void)messageCode;
  (void)pUserData;
  (void)pLayerPrefix;
  fprintf(
    stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage
  );
  return VK_FALSE;
}

bool VulkanRenderer::IsExtensionAvailable(
  const ImVector<VkExtensionProperties>& properties,
  const char* extension
) {
  for (const VkExtensionProperties& p : properties)
    if (strcmp(p.extensionName, extension) == 0)
      return true;
  return false;
}

void VulkanRenderer::SetupVulkan(ImVector<const char*>& instance_extensions) {
  VkResult err;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
  volkInitialize();
#endif
  // Create Vulkan Instance
  {
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    // Enumerate available extensions (This part is good to keep for checking)
    uint32_t properties_count;
    ImVector<VkExtensionProperties> properties;
    vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
    CheckVkResult(err);

    // --- Use GLFW to get required instance extensions ---
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    if (!glfw_extensions) {
      std::cerr << "Error: glfwGetRequiredInstanceExtensions failed." << std::endl;
      abort();  // Handle error (GLFW couldn't get extensions)
    }

    for (uint32_t i = 0; i < glfw_extension_count; ++i) {
      instance_extensions.push_back(glfw_extensions[i]);
      std::cout << "GLFW required extension: " << glfw_extensions[i] << std::endl;
    }
    // --- End GLFW extensions ---

    // Enable required extensions (keep this, but make sure GLFW extensions are added)
    if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
      instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
      instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif

    // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
    const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = layers;
    instance_extensions.push_back("VK_EXT_debug_report");
#endif

    // Create Vulkan Instance
    std::cout << "Creating Vulkan instance..." << std::endl;
    create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
    create_info.ppEnabledExtensionNames = instance_extensions.Data;
    err = vkCreateInstance(&create_info, allocator_, &instance_);
    CheckVkResult(err);  // VERY IMPORTANT: Check VkResult after vkCreateInstance
    std::cout << "Vulkan instance created" << std::endl;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkLoadInstance(instance_);
#endif

    // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
    auto f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT
    )vkGetInstanceProcAddr(instance_, "vkCreateDebugReportCallbackEXT");
    IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
    VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
    debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debug_report_ci.pfnCallback = DebugReport;
    debug_report_ci.pUserData = nullptr;
    err = f_vkCreateDebugReportCallbackEXT(instance_, &debug_report_ci, allocator_, &debug_report_);
    CheckVkResult(err);
#endif
  }

  // Select Physical Device (GPU)
  physical_device_ = ImGui_ImplVulkanH_SelectPhysicalDevice(instance_);

  // We need to ensure physical_device_ is valid before continuing
  if (physical_device_ == VK_NULL_HANDLE) {
    std::cerr << "Failed to find a suitable GPU!" << std::endl;
    abort();  // Or handle more gracefully
  }

  IM_ASSERT(physical_device_ != VK_NULL_HANDLE);
  std::cout << "Physical device selected" << std::endl;

  std::cout << "Selecting queue family" << std::endl;
  // Select graphics queue family
  queue_family_ = ImGui_ImplVulkanH_SelectQueueFamilyIndex(physical_device_);

  IM_ASSERT(queue_family_ != (uint32_t)-1);
  std::cout << "Queue family selected" << std::endl;

  // Create Logical Device (with 1 queue)
  {
    ImVector<const char*> device_extensions;
    device_extensions.push_back("VK_KHR_swapchain");

    // Enumerate physical device extension
    uint32_t properties_count;
    ImVector<VkExtensionProperties> properties;
    vkEnumerateDeviceExtensionProperties(physical_device_, nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    vkEnumerateDeviceExtensionProperties(
      physical_device_, nullptr, &properties_count, properties.Data
    );
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
      device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    const float queue_priority[] = {1.0f};
    VkDeviceQueueCreateInfo queue_info[1] = {};
    queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[0].queueFamilyIndex = queue_family_;
    queue_info[0].queueCount = 1;
    queue_info[0].pQueuePriorities = queue_priority;

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;  // Fixed: use 1 instead of sizeof calculation
    create_info.pQueueCreateInfos = queue_info;
    create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
    create_info.ppEnabledExtensionNames = device_extensions.Data;

    err = vkCreateDevice(physical_device_, &create_info, allocator_, &device_);
    CheckVkResult(err);
    vkGetDeviceQueue(device_, queue_family_, 0, &queue_);
  }

  // Create Descriptor Pool
  {
    VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE
      },
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 0;
    for (int i = 0; i < IM_ARRAYSIZE(pool_sizes); i++)
      pool_info.maxSets += pool_sizes[i].descriptorCount;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    err = vkCreateDescriptorPool(device_, &pool_info, allocator_, &descriptor_pool_);
    CheckVkResult(err);
  }

  std::cout << "Vulkan setup completed successfully" << std::endl;
}

// Add this to your VulkanRenderer class implementation
VkPhysicalDevice VulkanRenderer::ImGui_ImplVulkanH_SelectPhysicalDevice(VkInstance instance) {
  uint32_t gpu_count;
  VkResult err = vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
  CheckVkResult(err);  // Use your class's error checking function

  if (gpu_count == 0) {
    std::cerr << "No GPUs found with Vulkan support" << std::endl;
    return VK_NULL_HANDLE;
  }

  std::vector<VkPhysicalDevice> gpus(gpu_count);
  err = vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());
  CheckVkResult(err);

  // Print available GPUs for debugging
  std::cout << "Available GPUs: " << gpu_count << std::endl;
  for (uint32_t i = 0; i < gpu_count; i++) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(gpus[i], &properties);
    std::cout << "  " << i << ": " << properties.deviceName << " (";

    switch (properties.deviceType) {
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        std::cout << "Integrated GPU";
        break;
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        std::cout << "Discrete GPU";
        break;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        std::cout << "Virtual GPU";
        break;
      case VK_PHYSICAL_DEVICE_TYPE_CPU:
        std::cout << "CPU";
        break;
      default:
        std::cout << "Unknown";
        break;
    }

    std::cout << ")" << std::endl;
  }

  // Try to find a discrete GPU first
  for (VkPhysicalDevice& device : gpus) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      std::cout << "Selected discrete GPU: " << properties.deviceName << std::endl;
      return device;
    }
  }

  // If no discrete GPU found, use the first available GPU
  if (!gpus.empty()) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(gpus[0], &properties);
    std::cout << "No discrete GPU found. Using: " << properties.deviceName << std::endl;
    return gpus[0];
  }

  return VK_NULL_HANDLE;
}

uint32_t VulkanRenderer::ImGui_ImplVulkanH_SelectQueueFamilyIndex(VkPhysicalDevice physical_device
) {
  uint32_t count;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);

  std::vector<VkQueueFamilyProperties> queues_properties(count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, queues_properties.data());

  // Log available queue families for debugging
  std::cout << "Available queue families: " << count << std::endl;
  for (uint32_t i = 0; i < count; i++) {
    std::cout << "  " << i << ": ";
    if (queues_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      std::cout << "Graphics ";
    if (queues_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
      std::cout << "Compute ";
    if (queues_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
      std::cout << "Transfer ";
    if (queues_properties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
      std::cout << "Sparse ";
    if (queues_properties[i].queueFlags & VK_QUEUE_PROTECTED_BIT)
      std::cout << "Protected ";
    std::cout << "| Count: " << queues_properties[i].queueCount << std::endl;
  }

  // First try to find a queue family that supports both graphics and compute
  for (uint32_t i = 0; i < count; i++) {
    if ((queues_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        (queues_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      std::cout << "Selected queue family " << i << " (Graphics + Compute)" << std::endl;
      return i;
    }
  }

  // If not found, look for any queue family with graphics support
  for (uint32_t i = 0; i < count; i++) {
    if (queues_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      std::cout << "Selected queue family " << i << " (Graphics only)" << std::endl;
      return i;
    }
  }

  std::cerr << "Error: Could not find a suitable queue family!" << std::endl;
  return (uint32_t)-1;  // Return invalid value if no suitable queue is found
}

void VulkanRenderer::SetupVulkanWindow(
  ImGui_ImplVulkanH_Window* wd,
  VkSurfaceKHR surface,
  int width,
  int height
) {
  wd->Surface = surface;
  // Check for WSI support
  VkBool32 res;
  vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, queue_family_, wd->Surface, &res);
  if (res != VK_TRUE) {
    fprintf(stderr, "Error no WSI support on physical device 0\n");
    exit(-1);
  }

  // Select Surface Format
  const VkFormat requestSurfaceImageFormat[] = {
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_B8G8R8_UNORM,
    VK_FORMAT_R8G8B8_UNORM
  };
  const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

  std::cout << "Selecting surface format..." << std::endl;
  wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
    physical_device_,
    wd->Surface,
    requestSurfaceImageFormat,
    (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
    requestSurfaceColorSpace
  );

  std::cout << "Selecting present mode..." << std::endl;
  // Select Present Mode
  VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
  wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
    physical_device_, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes)
  );

  // Create SwapChain, RenderPass, Framebuffer, etc.
  IM_ASSERT(min_image_count_ >= 2);
  ImGui_ImplVulkanH_CreateOrResizeWindow(
    instance_,
    physical_device_,
    device_,
    wd,
    queue_family_,
    allocator_,
    width,
    height,
    min_image_count_
  );
}

void VulkanRenderer::CleanupVulkan() {
  vkDestroyDescriptorPool(device_, descriptor_pool_, allocator_);

#ifdef _DEBUG
  // Remove the debug report callback
  auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT
  )vkGetInstanceProcAddr(instance_, "vkDestroyDebugReportCallbackEXT");
  if (vkDestroyDebugReportCallbackEXT) {
    vkDestroyDebugReportCallbackEXT(instance_, debug_report_, allocator_);
  }
#endif

  vkDestroyDevice(device_, allocator_);
  vkDestroyInstance(instance_, allocator_);
}

void VulkanRenderer::CleanupVulkanWindow() {
  ImGui_ImplVulkanH_DestroyWindow(instance_, device_, &main_window_data_, allocator_);
}

void VulkanRenderer::FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data) {
  VkSemaphore image_acquired_semaphore =
    wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore =
    wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;

  VkResult err = vkAcquireNextImageKHR(
    device_, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex
  );
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    swap_chain_rebuild_ = true;
    return;
  }
  CheckVkResult(err);

  ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
  {
    err = vkWaitForFences(device_, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
    CheckVkResult(err);

    err = vkResetFences(device_, 1, &fd->Fence);
    CheckVkResult(err);
  }

  {
    err = vkResetCommandPool(device_, fd->CommandPool, 0);
    CheckVkResult(err);

    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
    CheckVkResult(err);
  }

  {
    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = wd->RenderPass;
    info.framebuffer = fd->Framebuffer;
    info.renderArea.extent.width = wd->Width;
    info.renderArea.extent.height = wd->Height;
    info.clearValueCount = 1;
    info.pClearValues = &wd->ClearValue;

    vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

  // Submit command buffer
  vkCmdEndRenderPass(fd->CommandBuffer);
  {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &image_acquired_semaphore;
    info.pWaitDstStageMask = &wait_stage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &fd->CommandBuffer;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &render_complete_semaphore;

    err = vkEndCommandBuffer(fd->CommandBuffer);
    CheckVkResult(err);

    err = vkQueueSubmit(queue_, 1, &info, fd->Fence);
    CheckVkResult(err);
  }
}

void VulkanRenderer::FramePresent(ImGui_ImplVulkanH_Window* wd) {
  if (swap_chain_rebuild_)
    return;

  VkSemaphore render_complete_semaphore =
    wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  VkPresentInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = &render_complete_semaphore;
  info.swapchainCount = 1;
  info.pSwapchains = &wd->Swapchain;
  info.pImageIndices = &wd->FrameIndex;

  VkResult err = vkQueuePresentKHR(queue_, &info);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    swap_chain_rebuild_ = true;
    return;
  }
  CheckVkResult(err);

  wd->SemaphoreIndex =
    (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;  // Now we can use the next set of semaphores
}

bool VulkanRenderer::initialize() {
  try {
    std::cout << "Starting initialization..." << std::endl;
    // Setup window
    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit()) {
      std::cerr << "Failed to initialize GLFW" << std::endl;
      return false;
    }
    std::cout << "GLFW initialized" << std::endl;

    // Create window with Vulkan context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(1280, 720, "Dear ImGui Vulkan Visualization", nullptr, nullptr);
    if (!window_) {
      std::cerr << "Failed to create GLFW window" << std::endl;
      return false;
    }
    std::cout << "GLFW window created" << std::endl;

    if (!glfwVulkanSupported()) {
      std::cerr << "GLFW: Vulkan Not Supported" << std::endl;
      return false;
    }
    std::cout << "Vulkan is supported" << std::endl;

    // Setup Vulkan
    ImVector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
      extensions.push_back(glfw_extensions[i]);

    std::cout << "Setting up Vulkan..." << std::endl;
    SetupVulkan(extensions);
    std::cout << "Vulkan setup complete" << std::endl;

    // Create Window Surface
    std::cout << "Creating surface..." << std::endl;
    VkResult err = glfwCreateWindowSurface(instance_, window_, allocator_, &surface_);
    CheckVkResult(err);
    std::cout << "Surface created" << std::endl;

    // Create Framebuffers
    std::cout << "Setting up Vulkan window..." << std::endl;
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    ImGui_ImplVulkanH_Window* wd = &main_window_data_;
    SetupVulkanWindow(wd, surface_, w, h);
    std::cout << "Vulkan window setup complete" << std::endl;

    // Setup Dear ImGui context
    std::cout << "Setting up ImGui..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look
    // identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      style.WindowRounding = 0.0f;
      style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    std::cout << "Initializing Vulkan backend..." << std::endl;
    ImGui_ImplGlfw_InitForVulkan(window_, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance_;
    init_info.PhysicalDevice = physical_device_;
    init_info.Device = device_;
    init_info.QueueFamily = queue_family_;
    init_info.Queue = queue_;
    init_info.PipelineCache = pipeline_cache_;
    init_info.DescriptorPool = descriptor_pool_;
    init_info.RenderPass = wd->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = min_image_count_;
    init_info.ImageCount = wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = allocator_;
    init_info.CheckVkResultFn = CheckVkResult;
    ImGui_ImplVulkan_Init(&init_info);
    std::cout << "Vulkan backend initialized" << std::endl;

    // Execute a GPU command to upload ImGui font textures
    std::cout << "Uploading fonts..." << std::endl;
    VkCommandPool command_pool = main_window_data_.Frames[main_window_data_.FrameIndex].CommandPool;
    VkCommandBuffer command_buffer =
      main_window_data_.Frames[main_window_data_.FrameIndex].CommandBuffer;

    err = vkResetCommandPool(device_, command_pool, 0);
    CheckVkResult(err);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    err = vkBeginCommandBuffer(command_buffer, &begin_info);
    CheckVkResult(err);

    ImGui_ImplVulkan_CreateFontsTexture();

    err = vkEndCommandBuffer(command_buffer);
    CheckVkResult(err);

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;

    err = vkQueueSubmit(queue_, 1, &end_info, VK_NULL_HANDLE);
    CheckVkResult(err);

    err = vkDeviceWaitIdle(device_);
    CheckVkResult(err);

    // Clear font textures from CPU memory
    ImGui_ImplVulkan_DestroyFontsTexture();
    std::cout << "Fonts uploaded" << std::endl;

    std::cout << "Initialization complete!" << std::endl;
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Exception during initialization: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cerr << "Unknown exception during initialization" << std::endl;
    return false;
  }
}

void VulkanRenderer::run() {
  // Main loop
  while (!glfwWindowShouldClose(window_)) {
    // Poll and handle events (inputs, window resize, etc.)
    glfwPollEvents();

    // Resize swap chain?
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    if (width > 0 && height > 0 &&
        (swap_chain_rebuild_ || main_window_data_.Width != width ||
         main_window_data_.Height != height)) {
      ImGui_ImplVulkan_SetMinImageCount(min_image_count_);
      ImGui_ImplVulkanH_CreateOrResizeWindow(
        instance_,
        physical_device_,
        device_,
        &main_window_data_,
        queue_family_,
        allocator_,
        width,
        height,
        min_image_count_
      );
      main_window_data_.FrameIndex = 0;
      swap_chain_rebuild_ = false;
    }

    // Skip rendering when minimized
    if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED)) {
      // Implement a basic sleep if minimized
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 10 * 1000 * 1000;  // 10ms
      nanosleep(&ts, nullptr);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window
    if (show_demo_window_)
      ImGui::ShowDemoWindow(&show_demo_window_);

    // 2. Show a simple window
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");

      ImGui::Text("This is some useful text.");
      ImGui::Checkbox("Demo Window", &show_demo_window_);
      ImGui::Checkbox("Another Window", &show_another_window_);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
      ImGui::ColorEdit3("clear color", (float*)&clear_color_);

      if (ImGui::Button("Button"))
        counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text(
        "Application average %.3f ms/frame (%.1f FPS)",
        1000.0f / ImGui::GetIO().Framerate,
        ImGui::GetIO().Framerate
      );
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

    // Rendering
    ImGui::Render();
    ImDrawData* main_draw_data = ImGui::GetDrawData();
    const bool main_is_minimized =
      (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);

    // Update clear color
    main_window_data_.ClearValue.color.float32[0] = clear_color_.x * clear_color_.w;
    main_window_data_.ClearValue.color.float32[1] = clear_color_.y * clear_color_.w;
    main_window_data_.ClearValue.color.float32[2] = clear_color_.z * clear_color_.w;
    main_window_data_.ClearValue.color.float32[3] = clear_color_.w;

    if (!main_is_minimized)
      FrameRender(&main_window_data_, main_draw_data);

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }

    // Present Main Platform Window
    if (!main_is_minimized)
      FramePresent(&main_window_data_);
  }

  // Cleanup
  VkResult err = vkDeviceWaitIdle(device_);
  CheckVkResult(err);
}

void VulkanRenderer::shutdown() {
  // Avoid double shutdown
  if (!window_)
    return;

  // Cleanup
  VkResult err = vkDeviceWaitIdle(device_);
  CheckVkResult(err);

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  CleanupVulkanWindow();
  CleanupVulkan();

  vkDestroySurfaceKHR(instance_, surface_, allocator_);
  glfwDestroyWindow(window_);
  glfwTerminate();

  // Mark as shut down
  window_ = nullptr;
}