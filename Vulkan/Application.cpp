#include "Application.h"
#include "Log.h"
#include "Utility.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtx/rotate_vector.hpp>

#include <set>

namespace Vulkan {

Application::Application(const ApplicationSettings& settings, const bool enableValidation)
: m_Settings(settings)
, m_EnableValidation(enableValidation)
{}


Application::~Application() {
   DestroyPipelineCache();
   DestroySyncObjects();
   DestroyCommandBuffers();
   DestroyCommandPool();
   DestroyFrameBuffers();
   DestroyRenderPass();
   DestroyDepthStencil();
   DestroyImageViews();
   DestroySwapChain(m_SwapChain);
   DestroyDevice();
   DestroySurface();
   DestroyInstance();
   DestroyWindow();
}


void Application::Run() {
   glfwSetTime(m_LastTime);
   while (!glfwWindowShouldClose(m_Window)) {
      glfwPollEvents();
      //
      // TODO: do nothing if window is minimized
      double currentTime = glfwGetTime();
      Update(currentTime - m_LastTime);
      RenderFrame();
      m_LastTime = currentTime;
   }
   m_Device.waitIdle();
}


void Application::OnKey(const int key, const int scancode, const int action, const int mods) {
}


void Application::OnCursorPos(const double xpos, const double ypos) {
}


void Application::OnMouseButton(const int button, const int action, const int mods) {
   if (button == GLFW_MOUSE_BUTTON_LEFT) {
      if (action == GLFW_PRESS) {
         m_LeftMouseDown = true;
         glfwGetCursorPos(m_Window, &m_MouseX, &m_MouseY);
      } else if (action == GLFW_RELEASE) {
         m_LeftMouseDown = false;
      }
   }
}


void Application::Init() {
   glfwSetErrorCallback(glfwErrorCallback);
   if (!glfwInit()) {
      throw std::runtime_error("glfwInit() failed");
   }
   if (!glfwVulkanSupported()) {
      throw std::runtime_error("glfwVulkanSupported() failed");
   }
   CreateWindow();
   CreateInstance();
   CreateSurface();
   SelectPhysicalDevice();
   CreateDevice();
   CreateSwapChain();
   CreateImageViews();
   CreateDepthStencil();
   CreateRenderPass();
   CreateFrameBuffers();
   CreateCommandPool();
   CreateCommandBuffers();
   CreateSyncObjects();
   CreatePipelineCache();
   // TODO: UI overlay
}


// void Application::SelectDepthFormat() {
//    // Since all depth formats may be optional, we need to find a suitable depth format to use
//    // Start with the highest precision packed format
//    std::vector<vk::Format> depthFormats = {
//       vk::Format::eD32SfloatS8Uint,
//       vk::Format::eD32Sfloat,
//       vk::Format::eD24UnormS8Uint,
//       vk::Format::eD16UnormS8Uint,
//       vk::Format::eD16Unorm
//    };
//    for (const auto& format : depthFormats) {
//       vk::FormatProperties formatProps = m_PhysicalDevice.getFormatProperties(format);
//       // Format must support depth stencil attachment for optimal tiling
//       if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
//          m_DepthFormat = format;
//       }
//    }
// 
//    throw std::runtime_error("Could not find suitable depth format!");
// }


void Application::CreateWindow() {
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, m_Settings.IsResizable ? GLFW_TRUE : GLFW_FALSE);

   const auto monitor = m_Settings.IsFullScreen ? glfwGetPrimaryMonitor() : nullptr;

   m_Window = glfwCreateWindow(m_Settings.WindowWidth, m_Settings.WindowHeight, m_Settings.ApplicationName, monitor, nullptr);
   if (!m_Window) {
      throw std::runtime_error("failed to create window");
   }

   // TODO: load window icon here...

   if (!m_Settings.IsCursorEnabled) {
      glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
   }

   glfwSetWindowUserPointer(m_Window, this);
   glfwSetKeyCallback(m_Window, glfwKeyCallback);
   glfwSetCursorPosCallback(m_Window, glfwCursorPosCallback);
   glfwSetMouseButtonCallback(m_Window, glfwMouseButtonCallback);
   glfwSetFramebufferSizeCallback(m_Window, glfwFramebufferResizeCallback);
}


void Application::DestroyWindow() {
   glfwDestroyWindow(m_Window);
   m_Window = nullptr;
}


void Application::CreateInstance() {
   vk::DynamicLoader dl;
   PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
   VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

   std::vector<const char*> layers = {"VK_LAYER_LUNARG_monitor"};
   if (m_EnableValidation) {
      layers.push_back("VK_LAYER_KHRONOS_validation");
   }
   CheckLayerSupport(layers);

   std::vector<const char*> extensions = GetRequiredInstanceExtensions();

   uint32_t glfwExtensionCount = 0;
   const char** glfwExtensions;
   glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

   extensions.insert(extensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);

   if (m_EnableValidation) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
   }

   CheckInstanceExtensionSupport(extensions);

   vk::ApplicationInfo appInfo = {
      m_Settings.ApplicationName,
      m_Settings.ApplicationVersion,
      "Vulkan::Application",
      VK_MAKE_VERSION(1, 0, 0),
      VK_API_VERSION_1_0
   };

   vk::DebugUtilsMessengerCreateInfoEXT debugCI {
      {},
      {vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError},
      {vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance},
      VulkanErrorCallback,
      nullptr
   };

   vk::InstanceCreateInfo instanceCI = {
      {},
      &appInfo,
      static_cast<uint32_t>(layers.size()),
      layers.data(),
      static_cast<uint32_t>(extensions.size()),
      extensions.data()
   };

   if (m_EnableValidation) {
      instanceCI.pNext = &debugCI;
   }

   m_Instance = vk::createInstance(instanceCI);
   VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);

   if (m_EnableValidation) {
      m_DebugUtilsMessengerEXT = m_Instance.createDebugUtilsMessengerEXT(debugCI);
   }
}


void Application::DestroyInstance() {
   if (m_Instance) {
      if (m_DebugUtilsMessengerEXT) {
         m_Instance.destroy(m_DebugUtilsMessengerEXT);
         m_DebugUtilsMessengerEXT = nullptr;
      }
      m_Instance.destroy();
      m_Instance = nullptr;
   }
}


std::vector<const char*> Vulkan::Application::GetRequiredInstanceExtensions() {
   return {};
}


void Application::CreateSurface() {
   VkSurfaceKHR surface;
   if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &surface) != VK_SUCCESS) {
      throw std::runtime_error("failed to create window surface!");
   }
   m_Surface = surface;
}


void Application::DestroySurface() {
   if (m_Instance && m_Surface) {
      m_Instance.destroy(m_Surface);
      m_Surface = nullptr;
   }
}


vk::SurfaceFormatKHR Application::SelectSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
   for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
         return availableFormat;
      }
   }
   return availableFormats[0];
}


vk::PresentModeKHR Application::SelectPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
   for (const auto& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
         return availablePresentMode;
      }
   }
   return vk::PresentModeKHR::eFifo;
}


void Application::SelectPhysicalDevice() {
   uint32_t deviceCount = 0;
   std::vector<vk::PhysicalDevice> physicalDevices = m_Instance.enumeratePhysicalDevices();
   for (const auto& physicalDevice : physicalDevices) {
      if (IsPhysicalDeviceSuitable(physicalDevice)) {
         m_PhysicalDevice = physicalDevice;
         m_PhysicalDeviceProperties = m_PhysicalDevice.getProperties();
         m_PhysicalDeviceFeatures = m_PhysicalDevice.getFeatures();
         m_PhysicalDeviceMemoryProperties = m_PhysicalDevice.getMemoryProperties();
         m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);
         m_DepthFormat = FindSupportedFormat(
            {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD16Unorm},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
         );
         break;
      }
   }
   if (!m_PhysicalDevice) {
      throw std::runtime_error("failed to find a suitable GPU!");
   }
}


vk::PhysicalDeviceFeatures Application::GetRequiredPhysicalDeviceFeatures(vk::PhysicalDeviceFeatures) {
   return {};
}


void* Application::GetRequiredPhysicalDeviceFeaturesEXT() {
   return nullptr;
}


bool Application::IsPhysicalDeviceSuitable(vk::PhysicalDevice physicalDevice) {
   bool extensionsSupported = false;
   bool swapChainAdequate = false;
   QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
   if (indices.IsComplete()) {
      extensionsSupported = CheckDeviceExtensionSupport(physicalDevice, GetRequiredDeviceExtensions());
      if (extensionsSupported) {
         SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, m_Surface);
         swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
      }
   }
   return indices.IsComplete() && extensionsSupported && swapChainAdequate;
}


void Application::CreateDevice() {
   float queuePriority = 1.0f;

   std::vector<vk::DeviceQueueCreateInfo> deviceQueueCIs;
   std::set<uint32_t> uniqueQueueFamilies = {m_QueueFamilyIndices.GraphicsFamily.value(), m_QueueFamilyIndices.PresentFamily.value()};

   for (uint32_t queueFamily : uniqueQueueFamilies) {
      deviceQueueCIs.emplace_back(
         vk::DeviceQueueCreateFlags {},
         queueFamily,
         1,
         &queuePriority
      );
   }

   std::vector<const char*> deviceExtensions = GetRequiredDeviceExtensions();

   // We always need swap chain extension
   deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

   m_EnabledPhysicalDeviceFeatures = GetRequiredPhysicalDeviceFeatures(m_PhysicalDeviceFeatures);

   vk::DeviceCreateInfo ci = {
      {},
      static_cast<uint32_t>(deviceQueueCIs.size())     /*queueCreateInfoCount*/,
      deviceQueueCIs.data()                            /*pQueueCreateInfos*/,
      0                                                /*enabledLayerCount*/,
      nullptr                                          /*ppEnabledLayerNames*/,
      static_cast<uint32_t>(deviceExtensions.size())   /*enabledExtensionCount*/,
      deviceExtensions.data()                          /*ppEnabledExtensionNames*/,
      &m_EnabledPhysicalDeviceFeatures                 /*pEnabledFeatures*/
   };
   ci.pNext = GetRequiredPhysicalDeviceFeaturesEXT();

   std::vector<const char*> layers = {"VK_LAYER_KHRONOS_validation"};
   if (m_EnableValidation) {
      ci.enabledLayerCount = static_cast<uint32_t>(layers.size());
      ci.ppEnabledLayerNames = layers.data();
   }

   m_Device = m_PhysicalDevice.createDevice(ci);

   m_GraphicsQueue = m_Device.getQueue(m_QueueFamilyIndices.GraphicsFamily.value(), 0);
   m_PresentQueue = m_Device.getQueue(m_QueueFamilyIndices.PresentFamily.value(), 0);
}


void Application::DestroyDevice() {
   if (m_Device) {
      m_Device.destroy();
      m_Device = nullptr;
   }
}


std::vector<const char*> Application::GetRequiredDeviceExtensions() {
   return {};
}


void Application::CreateSwapChain() {
   vk::SwapchainKHR oldSwapChain = m_SwapChain;

   SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_PhysicalDevice, m_Surface);

   vk::SurfaceFormatKHR surfaceFormat = SelectSurfaceFormat(swapChainSupport.Formats);
   vk::PresentModeKHR presentMode = SelectPresentMode(swapChainSupport.PresentModes);
   m_Format = surfaceFormat.format;
   m_Extent = SelectSwapExtent(swapChainSupport.Capabilities);

   uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
   if ((swapChainSupport.Capabilities.maxImageCount > 0) && (imageCount > swapChainSupport.Capabilities.maxImageCount)) {
      imageCount = swapChainSupport.Capabilities.maxImageCount;
   }

   // Find the transformation of the surface
   vk::SurfaceTransformFlagBitsKHR preTransform;
   if (swapChainSupport.Capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
      // We prefer a non-rotated transform
      preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
   } else {
      preTransform = swapChainSupport.Capabilities.currentTransform;
   }

   // Find a supported composite alpha format (not all devices support alpha opaque)
   vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
   // Select the first composite alpha format available
   std::array<vk::CompositeAlphaFlagBitsKHR, 4> compositeAlphaFlags = {
      vk::CompositeAlphaFlagBitsKHR::eOpaque,
      vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
      vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
      vk::CompositeAlphaFlagBitsKHR::eInherit
   };
   for (const auto& compositeAlphaFlag : compositeAlphaFlags) {
      if (swapChainSupport.Capabilities.supportedCompositeAlpha & compositeAlphaFlag) {
         compositeAlpha = compositeAlphaFlag;
         break;
      };
   }

   vk::SwapchainCreateInfoKHR ci;
   ci.surface = m_Surface;
   ci.minImageCount = imageCount;
   ci.imageFormat = m_Format;
   ci.imageColorSpace = surfaceFormat.colorSpace;
   ci.imageExtent = m_Extent;
   ci.imageArrayLayers = 1;
   ci.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
   ci.preTransform = preTransform;
   ci.compositeAlpha = compositeAlpha;
   ci.presentMode = presentMode;
   ci.clipped = true;
   ci.oldSwapchain = oldSwapChain;

   uint32_t queueFamilyIndices[] = {m_QueueFamilyIndices.GraphicsFamily.value(), m_QueueFamilyIndices.PresentFamily.value()};
   if (m_QueueFamilyIndices.GraphicsFamily != m_QueueFamilyIndices.PresentFamily) {
      ci.imageSharingMode = vk::SharingMode::eConcurrent;
      ci.queueFamilyIndexCount = 2;
      ci.pQueueFamilyIndices = queueFamilyIndices;
   }

   // Enable transfer source on swap chain images if supported
   if (swapChainSupport.Capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc) {
      ci.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
   }

   // Enable transfer destination on swap chain images if supported
   if (swapChainSupport.Capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
      ci.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
   }

   m_SwapChain = m_Device.createSwapchainKHR(ci);
   DestroySwapChain(oldSwapChain);
   std::vector<vk::Image> swapChainImages = m_Device.getSwapchainImagesKHR(m_SwapChain);
   for (const auto& image : swapChainImages) {
      m_SwapChainImages.emplace_back(m_Device, image);
   }
}


void Application::DestroySwapChain(vk::SwapchainKHR& swapChain) {
   if (m_Device && swapChain) {
      m_SwapChainImages.clear();
      m_Device.destroy(swapChain);
      swapChain = nullptr;
   }
}


void Application::CreateImageViews() {
   for (auto& image : m_SwapChainImages) {
      image.CreateImageView(m_Format, vk::ImageAspectFlagBits::eColor, 1);
   }
}


void Application::DestroyImageViews() {
   if (m_Device) {
      for (auto& image : m_SwapChainImages) {
         image.DestroyImageView();
      }
   }
}


vk::Extent2D Application::SelectSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
   if (capabilities.currentExtent.width != UINT32_MAX) {
      return capabilities.currentExtent;
   } else {
      int width;
      int height;
      glfwGetFramebufferSize(m_Window, &width, &height);
      vk::Extent2D actualExtent = {
          static_cast<uint32_t>(width),
          static_cast<uint32_t>(height)
      };

      actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

      return actualExtent;
   }
}


void Application::CreateDepthStencil() {
   // TODO anti-aliasing
   m_DepthImage = std::make_unique<Image>(m_Device, m_PhysicalDevice, vk::ImageViewType::e2D, m_Extent.width, m_Extent.height, 1, vk::SampleCountFlagBits::e1, m_DepthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
   m_DepthImage->CreateImageView(m_DepthFormat, vk::ImageAspectFlagBits::eDepth, 1);
}


void Application::DestroyDepthStencil() {
   m_DepthImage.reset(nullptr);
}


void Application::CreateRenderPass() {
   std::vector<vk::AttachmentDescription> attachments = {
      {
         {}                                         /*flags*/,
         m_Format                                   /*format*/,
         vk::SampleCountFlagBits::e1                /*samples*/,   // TODO: anti-aliasing
         vk::AttachmentLoadOp::eClear               /*loadOp*/,
         vk::AttachmentStoreOp::eStore              /*storeOp*/,
         vk::AttachmentLoadOp::eDontCare            /*stencilLoadOp*/,
         vk::AttachmentStoreOp::eDontCare           /*stencilStoreOp*/,
         vk::ImageLayout::eUndefined                /*initialLayout*/,
         vk::ImageLayout::ePresentSrcKHR            /*finalLayout*/     // anti-aliasing = vk::ImageLayout::eColorAttachmentOptimal here
      },
      {
         {}                                              /*flags*/,
         m_DepthFormat                                   /*format*/,
         vk::SampleCountFlagBits::e1                     /*samples*/,   // TODO: anti-aliasing
         vk::AttachmentLoadOp::eClear                    /*loadOp*/,
         vk::AttachmentStoreOp::eStore                   /*storeOp*/,
         vk::AttachmentLoadOp::eClear                    /*stencilLoadOp*/,
         vk::AttachmentStoreOp::eDontCare                /*stencilStoreOp*/,
         vk::ImageLayout::eUndefined                     /*initialLayout*/,
         vk::ImageLayout::eDepthStencilAttachmentOptimal /*finalLayout*/
      }
      //{
      //    {}                                              /*flags*/,
      //    m_Format                                        /*format*/,
      //    vk::SampleCountFlagBits::e1                     /*samples*/,
      //    vk::AttachmentLoadOp::eDontCare                 /*loadOp*/,
      //    vk::AttachmentStoreOp::eStore                   /*storeOp*/,
      //    vk::AttachmentLoadOp::eDontCare                 /*stencilLoadOp*/,
      //    vk::AttachmentStoreOp::eDontCare                /*stencilStoreOp*/,
      //    vk::ImageLayout::eUndefined                     /*initialLayout*/,
      //    vk::ImageLayout::ePresentSrcKHR                 /*finalLayout*/
      //}
   };

   vk::AttachmentReference colorAttachmentRef = {
      0,
      vk::ImageLayout::eColorAttachmentOptimal
   };

   vk::AttachmentReference depthAttachmentRef = {
      1,
      vk::ImageLayout::eDepthStencilAttachmentOptimal
   };

   //    vk::AttachmentReference resolveAttachmentRef = {
   //       2,
   //       vk::ImageLayout::eColorAttachmentOptimal
   //    };

   vk::SubpassDescription subpass = {
      {}                               /*flags*/,
      vk::PipelineBindPoint::eGraphics /*pipelineBindPoint*/,
      0                                /*inputAttachmentCount*/,
      nullptr                          /*pInputAttachments*/,
      1                                /*colorAttachmentCount*/,
      &colorAttachmentRef              /*pColorAttachments*/,
      nullptr, //&resolveAttachmentRef /*pResolveAttachments*/,
      &depthAttachmentRef              /*pDepthStencilAttachment*/,
      0                                /*preserveAttachmentCount*/,
      nullptr                          /*pPreserveAttachments*/
   };

   std::vector<vk::SubpassDependency> dependencies = {
      {
         VK_SUBPASS_EXTERNAL                                                                    /*srcSubpass*/,
         0                                                                                      /*dstSubpass*/,
         vk::PipelineStageFlagBits::eBottomOfPipe                                               /*srcStageMask*/,
         vk::PipelineStageFlagBits::eColorAttachmentOutput                                      /*dstStageMask*/,
         vk::AccessFlagBits::eMemoryRead                                                        /*srcAccessMask*/,
         vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite   /*dstAccessMask*/,
         vk::DependencyFlagBits::eByRegion                                                      /*dependencyFlags*/
      },
      {
         0                                                                                      /*srcSubpass*/,
         VK_SUBPASS_EXTERNAL                                                                    /*dstSubpass*/,
         vk::PipelineStageFlagBits::eColorAttachmentOutput                                      /*srcStageMask*/,
         vk::PipelineStageFlagBits::eBottomOfPipe                                               /*dstStageMask*/,
         vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite   /*srcAccessMask*/,
         vk::AccessFlagBits::eMemoryRead                                                        /*dstAccessMask*/,
         vk::DependencyFlagBits::eByRegion                                                      /*dependencyFlags*/
      }
   };

   m_RenderPass = m_Device.createRenderPass({
      {}                                         /*flags*/,
      static_cast<uint32_t>(attachments.size())  /*attachmentCount*/,
      attachments.data()                         /*pAttachments*/,
      1                                          /*subpassCount*/,
      & subpass                                   /*pSubpasses*/,
      static_cast<uint32_t>(dependencies.size()) /*dependencyCount*/,
      dependencies.data()                        /*pDependencies*/
      });
}


void Application::DestroyRenderPass() {
   if (m_Device && m_RenderPass) {
      m_Device.destroy(m_RenderPass);
      m_RenderPass = nullptr;
   }
}


void Application::CreateFrameBuffers() {
   std::array<vk::ImageView, 2> attachments = {
      nullptr,
      m_DepthImage->m_ImageView
   };
   vk::FramebufferCreateInfo ci = {
      {}                                        /*flags*/,
      m_RenderPass                              /*renderPass*/,
      static_cast<uint32_t>(attachments.size()) /*attachmentCount*/,
      attachments.data()                        /*pAttachments*/,
      m_Extent.width                            /*width*/,
      m_Extent.height                           /*height*/,
      1                                         /*layers*/
   };

   m_SwapChainFrameBuffers.reserve(m_SwapChainImages.size());
   for (const auto& swapChainImage : m_SwapChainImages) {
      attachments[0] = swapChainImage.m_ImageView;
      m_SwapChainFrameBuffers.push_back(m_Device.createFramebuffer(ci));
   }
}


void Application::DestroyFrameBuffers() {
   if (m_Device) {
      for (auto frameBuffer : m_SwapChainFrameBuffers) {
         m_Device.destroy(frameBuffer);
      }
      m_SwapChainFrameBuffers.clear();
   }
}


void Application::CreateCommandPool() {
   m_CommandPool = m_Device.createCommandPool({
      {vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
      m_QueueFamilyIndices.GraphicsFamily.value()
      });
}


void Application::DestroyCommandPool() {
   if (m_Device && m_CommandPool) {
      m_Device.destroy(m_CommandPool);
      m_CommandPool = nullptr;
   }
}


void Application::CreateCommandBuffers() {
   m_CommandBuffers = m_Device.allocateCommandBuffers({
      m_CommandPool                                           /*commandPool*/,
      vk::CommandBufferLevel::ePrimary                        /*level*/,
      static_cast<uint32_t>(m_SwapChainFrameBuffers.size())   /*commandBufferCount*/
   });
}


void Application::DestroyCommandBuffers() {
   if (m_Device && m_CommandPool) {
      m_Device.freeCommandBuffers(m_CommandPool, m_CommandBuffers);
      m_CommandBuffers.clear();
   }
}


void Application::CreateSyncObjects() {
   m_ImageAvailableSemaphores.reserve(m_Settings.MaxFramesInFlight);
   m_RenderFinishedSemaphores.reserve(m_Settings.MaxFramesInFlight);
   m_InFlightFences.reserve(m_Settings.MaxFramesInFlight);

   vk::FenceCreateInfo ci = {
      {vk::FenceCreateFlagBits::eSignaled}
   };

   for (uint32_t i = 0; i < m_Settings.MaxFramesInFlight; ++i) {
      m_ImageAvailableSemaphores.emplace_back(m_Device.createSemaphore({}));
      m_RenderFinishedSemaphores.emplace_back(m_Device.createSemaphore({}));
      m_InFlightFences.emplace_back(m_Device.createFence(ci));
   }
}


void Application::DestroySyncObjects() {
   if (m_Device) {
      for (auto semaphore : m_ImageAvailableSemaphores) {
         m_Device.destroy(semaphore);
      }
      m_ImageAvailableSemaphores.clear();

      for (auto semaphore : m_RenderFinishedSemaphores) {
         m_Device.destroy(semaphore);
      }
      m_RenderFinishedSemaphores.clear();

      for (auto fence : m_InFlightFences) {
         m_Device.destroy(fence);
      }
      m_InFlightFences.clear();
   }
}


void Application::CreatePipelineCache() {
   m_PipelineCache = m_Device.createPipelineCache({});
}


void Application::DestroyPipelineCache() {
   if (m_Device && m_PipelineCache) {
      m_Device.destroy(m_PipelineCache);
   }
}


void Application::Update(double dt) {
   auto deltaTime = static_cast<float>(dt);

   // TODO: abstract this into a camera controller
   if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS) {
      m_Eye += deltaTime * m_Direction;
   } else if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS) {
      m_Eye -= deltaTime * m_Direction;
   }

   if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS) {
      m_Eye -= deltaTime * glm::cross(m_Direction, m_Up);
   } else if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS) {
      m_Eye += deltaTime * glm::cross(m_Direction, m_Up);
   }

   if (glfwGetKey(m_Window, GLFW_KEY_R) == GLFW_PRESS) {
      m_Eye += deltaTime * m_Up * glm::length(m_Direction);
   } else if (glfwGetKey(m_Window, GLFW_KEY_F) == GLFW_PRESS) {
      m_Eye -= deltaTime * m_Up * glm::length(m_Direction);
   }

   if (m_LeftMouseDown) {
      double xpos, ypos;
      glfwGetCursorPos(m_Window, &xpos, &ypos);
      auto deltaX = static_cast<float>(m_MouseX - xpos);
      auto deltaY = static_cast<float>(m_MouseY - ypos);
      m_MouseX = xpos;
      m_MouseY = ypos;

      // depending on deltaX, rotate about the y-axis.
      m_Direction = glm::rotateY(m_Direction, deltaTime * deltaX);
      m_Up = glm::rotateY(m_Up, deltaTime * deltaX);

      // depending on deltaY, rotate about m_Direction x m_Up
      glm::vec3 right = glm::cross(m_Direction, m_Up);
      m_Direction = glm::rotate(m_Direction, deltaTime * deltaY, right);
      m_Up = glm::rotate(m_Up, deltaTime * deltaY, right);

   }
}


void Application::BeginFrame() {
   auto rv = m_Device.acquireNextImageKHR(m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], nullptr);

   if (rv.result == vk::Result::eErrorOutOfDateKHR) {
      OnWindowResized();
      return;
   } else if ((rv.result != vk::Result::eSuccess) && (rv.result != vk::Result::eSuboptimalKHR)) {
      throw std::runtime_error("failed to acquire swap chain image!");
   }

   // acquireNextImage returns as soon as it has decided which image is the next one.
   // That doesn't necessarily mean the image is available for either the CPU or the GPU to start doing stuff to it,
   // it's just that we now know which image is *going to be* the next one.
   // The semaphore that was passed in gets signaled when the image really is available,
   // and so (later, when we submit frame to the graphics queue) we'll tell the GPU to wait for that semaphore before starting render
   m_CurrentImage = rv.value;

   // Wait until we know GPU has finished with the command buffer we are about to use...
   // Note that m_CurrentFrame and m_CurrentImage are not necessarily equal (particularly if we have, say, 3 swap chain images, and 2 frames-in-flight)
   // However, we do know that the GPU has finished with m_CurrentImage'th command buffer so long as the m_CurrentFrame'th fence is signaled
   auto result = m_Device.waitForFences(m_InFlightFences[m_CurrentFrame], true, UINT64_MAX);
}


void Application::RenderFrame() {
}


void Application::EndFrame() {
   vk::PipelineStageFlags waitStages[] = {{vk::PipelineStageFlagBits::eColorAttachmentOutput}};
   vk::SubmitInfo si = {
      1                                             /*waitSemaphoreCount*/,
      &m_ImageAvailableSemaphores[m_CurrentFrame]   /*pWaitSemaphores*/,
      waitStages                                    /*pWaitDstStageMask*/,
      1                                             /*commandBufferCount*/,
      &m_CommandBuffers[m_CurrentImage]             /*pCommandBuffers*/,
      1                                             /*signalSemaphoreCount*/,
      &m_RenderFinishedSemaphores[m_CurrentFrame]   /*pSignalSemaphores*/
   };

   m_Device.resetFences(m_InFlightFences[m_CurrentFrame]);
   m_GraphicsQueue.submit(si, m_InFlightFences[m_CurrentFrame]);

   vk::PresentInfoKHR pi = {
      1                                            /*waitSemaphoreCount*/,
      &m_RenderFinishedSemaphores[m_CurrentFrame]  /*pWaitSemaphores*/,
      1                                            /*swapchainCount*/,
      &m_SwapChain                                 /*pSwapchains*/,
      &m_CurrentImage                              /*pImageIndices*/,
      nullptr                                      /*pResults*/
   };

   // do not use cpp wrappers here.
   // The problem is that VK_ERROR_OUT_OF_DATE_KHR is an exception in the cpp wrapper, rather
   // than a valid return code.
   // Of course, you could try..catch but that seems quite an inefficient way to do it.
   //auto result = m_PresentQueue.presentKHR(pi);
   auto result = vkQueuePresentKHR(m_PresentQueue, &(VkPresentInfoKHR)pi);
   if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_WantResize) {
      OnWindowResized();
   } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
   }

   m_CurrentFrame = ++m_CurrentFrame % m_Settings.MaxFramesInFlight;
}


void Application::OnWindowResized() {
   m_Device.waitIdle();
   DestroyImageViews();
   CreateSwapChain();
   CreateImageViews();

   DestroyDepthStencil();
   CreateDepthStencil();

   DestroyFrameBuffers();
   CreateFrameBuffers();

   // TODO: resize UI overlay?

   // Command buffers need to be recreated as they may store references to the recreated frame buffers
   DestroyCommandBuffers();
   CreateCommandBuffers();
   m_WantResize = false;
}


vk::ShaderModule Application::CreateShaderModule(std::vector<char> code) {
   vk::ShaderModuleCreateInfo ci = {
      {},
      code.size(),
      reinterpret_cast<const uint32_t*>(code.data())
   };

   return m_Device.createShaderModule(ci);
}


void Application::DestroyShaderModule(vk::ShaderModule& module) {
   if (m_Device) {
      if (module) {
         m_Device.destroy(module);
         module = nullptr;
      }
   }
}


QueueFamilyIndices Application::FindQueueFamilies(vk::PhysicalDevice physicalDevice) {
   QueueFamilyIndices indices;

   std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
   int i = 0;
   for (const auto& queueFamily : queueFamilies) {
      if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
         indices.GraphicsFamily = i;
      }

      if (physicalDevice.getSurfaceSupportKHR(i, m_Surface)) {
         indices.PresentFamily = i;
      }

      if (indices.IsComplete()) {
         break;
      }

      ++i;
   }

   return indices;
}


vk::Format Application::FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
   for (auto format : candidates) {
      vk::FormatProperties props = m_PhysicalDevice.getFormatProperties(format);
      if ((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) {
         return format;
      } else if ((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features)) {
         return format;
      }
   }
   throw std::runtime_error("failed to find supported format!");
}


void Application::SubmitSingleTimeCommands(const std::function<void(vk::CommandBuffer)>& action) {
   std::vector<vk::CommandBuffer> commandBuffers = m_Device.allocateCommandBuffers({
      m_CommandPool                    /*commandPool*/,
      vk::CommandBufferLevel::ePrimary /*level*/,
      1                                /*commandBufferCount*/
   });

   commandBuffers[0].begin({VULKAN_HPP_NAMESPACE::CommandBufferUsageFlagBits::eOneTimeSubmit});
   action(commandBuffers[0]);
   commandBuffers[0].end();

   vk::SubmitInfo si;
   si.commandBufferCount = 1;
   si.pCommandBuffers = commandBuffers.data();
   m_GraphicsQueue.submit(si, nullptr);
   m_GraphicsQueue.waitIdle();
   m_Device.freeCommandBuffers(m_CommandPool, commandBuffers);
}


void Application::CopyBuffer(vk::Buffer src, vk::Buffer dst, const vk::DeviceSize srcOffset, const vk::DeviceSize dstOffset, const vk::DeviceSize size) {
   SubmitSingleTimeCommands([src, dst, srcOffset, dstOffset, size] (vk::CommandBuffer cmd) {
      vk::BufferCopy copyRegion = {
         srcOffset,
         dstOffset,
         size
      };
      cmd.copyBuffer(src, dst, copyRegion);
   });
}


void Application::TransitionImageLayout(vk::Image image, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout, const uint32_t mipLevels) {
   SubmitSingleTimeCommands([image, oldLayout, newLayout, mipLevels] (vk::CommandBuffer cmd) {
      vk::ImageMemoryBarrier barrier = {
         {}                                  /*srcAccessMask*/,
         {}                                  /*dstAccessMask*/,
         oldLayout                           /*oldLayout*/,
         newLayout                           /*newLayout*/,
         VK_QUEUE_FAMILY_IGNORED             /*srcQueueFamilyIndex*/,
         VK_QUEUE_FAMILY_IGNORED             /*dstQueueFamilyIndex*/,
         image                               /*image*/,
         {
            vk::ImageAspectFlagBits::eColor     /*aspectMask*/,
            0                                   /*baseMipLevel*/,
            mipLevels                           /*levelCount*/,
            0                                   /*baseArrayLayer*/,
            VK_REMAINING_ARRAY_LAYERS           /*layerCount*/
         }                                   /*subresourceRange*/
      };

      vk::PipelineStageFlags sourceStage;
      vk::PipelineStageFlags destinationStage;

      if (oldLayout == (vk::ImageLayout::eUndefined) && (newLayout == vk::ImageLayout::eTransferDstOptimal)) {
         barrier.srcAccessMask = {};
         barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
         sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
         destinationStage = vk::PipelineStageFlagBits::eTransfer;
      } else if ((oldLayout == vk::ImageLayout::eUndefined) && (newLayout == vk::ImageLayout::eGeneral)) {
         barrier.srcAccessMask = {};
         barrier.dstAccessMask = {};
         sourceStage = vk::PipelineStageFlagBits::eAllCommands;
         destinationStage = vk::PipelineStageFlagBits::eAllCommands;
      } else if ((oldLayout == vk::ImageLayout::eTransferDstOptimal) && (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)) {
         barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
         barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
         sourceStage = vk::PipelineStageFlagBits::eTransfer;
         destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
      } else if ((oldLayout == vk::ImageLayout::eGeneral) && (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)) {
         barrier.srcAccessMask = {};
         barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
         sourceStage = vk::PipelineStageFlagBits::eAllCommands;
         destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
      } else {
         throw std::invalid_argument("unsupported layout transition!");
      }

      cmd.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);
   });
}


void Application::CopyBufferToImage(vk::Buffer buffer, vk::Image image, const uint32_t width, const uint32_t height) {
   SubmitSingleTimeCommands([buffer, image, width, height] (vk::CommandBuffer cmd) {
      vk::BufferImageCopy region = {
         0                                    /*bufferOffset*/,
         0                                    /*bufferRowLength*/,
         0                                    /*bufferImageHeight*/,
         vk::ImageSubresourceLayers {
            vk::ImageAspectFlagBits::eColor      /*aspectMask*/,
            0                                    /*mipLevel*/,
            0                                    /*baseArrayLayer*/,
            1                                    /*layerCount*/
         }                                    /*imageSubresource*/,
         {0, 0, 0}                            /*imageOffset*/,
         {width, height, 1}                   /*imageExtent*/
      };
      cmd.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
   });
}


void Application::GenerateMIPMaps(vk::Image image, const vk::Format format, const uint32_t width, const uint32_t height, uint32_t mipLevels) {
   // Check if image format supports linear blitting
   vk::FormatProperties formatProperties = m_PhysicalDevice.getFormatProperties(format);
   if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
      throw std::runtime_error("texture image format does not support linear blitting!");
   }

   SubmitSingleTimeCommands([image, width, height, mipLevels] (vk::CommandBuffer cmd) {
      vk::ImageMemoryBarrier barrier = {
         {}                                   /*srcAccessMask*/,
         {}                                   /*dstAccessMask*/,
         vk::ImageLayout::eUndefined          /*oldLayout*/,
         vk::ImageLayout::eUndefined          /*newLayout*/,
         VK_QUEUE_FAMILY_IGNORED              /*srcQueueFamilyIndex*/,
         VK_QUEUE_FAMILY_IGNORED              /*dstQueueFamilyIndex*/,
         image                                /*image*/,
         vk::ImageSubresourceRange {
            {vk::ImageAspectFlagBits::eColor}    /*aspectMask*/,
            0                                    /*baseMipLevel*/,
            1                                    /*levelCount*/,
            0                                    /*baseArrayLayer*/,
            VK_REMAINING_ARRAY_LAYERS            /*layerCount*/
         }                                    /*subresourceRange*/
      };

      int32_t mipWidth = static_cast<int32_t>(width);
      int32_t mipHeight = static_cast<int32_t>(height);

      for (uint32_t i = 1; i < mipLevels; i++) {
         barrier.subresourceRange.baseMipLevel = i - 1;
         barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
         barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
         barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
         barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, barrier);

         vk::ImageBlit blit;
         blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
         blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
         blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
         blit.srcSubresource.mipLevel = i - 1;
         blit.srcSubresource.baseArrayLayer = 0;
         blit.srcSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;
         blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
         blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
         blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
         blit.dstSubresource.mipLevel = i;
         blit.dstSubresource.baseArrayLayer = 0;
         blit.dstSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;
         cmd.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

         barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
         barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
         barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
         barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
         cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, barrier);

         if (mipWidth > 1) {
            mipWidth /= 2;
         }
         if (mipHeight > 1) {
            mipHeight /= 2;
         }
      }
      barrier.subresourceRange.baseMipLevel = mipLevels - 1;
      barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
      barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
      barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
      cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, barrier);
   });
}


void Application::CreateBottomLevelAccelerationStructures(vk::ArrayProxy<const std::vector<vk::GeometryNV>> geometryGroups) {
   //
   // one BLAS per vector of geometries in geometryGroups
   //
   // We create all of the BLAS, and then allocate one buffer for all of them (each one is offset into buffer)
   //
   for (const auto& geometries : geometryGroups) {
      vk::AccelerationStructureInfoNV accelerationStructureInfo = {
         vk::AccelerationStructureTypeNV::eBottomLevel                /*type*/,
         vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace   /*flags*/,
         0                                                            /*instanceCount*/,
         static_cast<uint32_t>(geometries.size())                     /*geometryCount*/,
         geometries.data()                                            /*pGeometries*/
      };
      m_BLAS.emplace_back(accelerationStructureInfo);

      vk::AccelerationStructureCreateInfoNV accelerationStructureCI = {
         0                                         /*compactedSize*/,
         m_BLAS.back().m_AccelerationStructureInfo /*info*/
      };
      m_BLAS.back().m_AccelerationStructure = m_Device.createAccelerationStructureNV(accelerationStructureCI);
   }

   std::vector<vk::MemoryRequirements2> memoryRequirements;
   for (const auto& blas : m_BLAS) {
      memoryRequirements.emplace_back(
         m_Device.getAccelerationStructureMemoryRequirementsNV({
            vk::AccelerationStructureMemoryRequirementsTypeNV::eObject /*type*/,
            blas.m_AccelerationStructure                               /*accelerationStructure*/
         })
      );
   }

   vk::DeviceSize totalMemorySize = 0;
   uint32_t memoryType = 0;
   for (const auto& memoryRequirement : memoryRequirements) {
      totalMemorySize += memoryRequirement.memoryRequirements.size;
      memoryType = memoryRequirement.memoryRequirements.memoryTypeBits;
   }

   vk::DeviceMemory objectMemory = m_Device.allocateMemory({
      totalMemorySize,
      Buffer::FindMemoryType(m_PhysicalDevice, memoryType, vk::MemoryPropertyFlagBits::eDeviceLocal)
   });

   vk::DeviceSize memoryOffset = 0;
   uint32_t i = 0;
   for (auto& blas : m_BLAS) {
      blas.m_Memory = objectMemory;
      vk::BindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo = {
         blas.m_AccelerationStructure /*accelerationStructure*/,
         blas.m_Memory                /*memory*/,
         memoryOffset                 /*memoryOffset*/,
         0                            /*deviceIndexCount*/,
         nullptr                      /*pDeviceIndices*/
      };
      m_Device.bindAccelerationStructureMemoryNV(accelerationStructureMemoryInfo);

      // This ends up instantiating the wrong template.  compiler bug?
      //m_Device.getAccelerationStructureHandleNV<uint64_t>(blas.m_AccelerationStructure, blas.m_Handle);
      //
      // workaround:
      vk::ArrayProxy<uint64_t> handle(blas.m_Handle);
      m_Device.getAccelerationStructureHandleNV<uint64_t>(blas.m_AccelerationStructure, handle);
      memoryOffset += memoryRequirements[i++].memoryRequirements.size;
   }
}


void Application::DestroyBottomLevelAccelerationStructures() {
   if (m_Device) {
      for (auto& blas : m_BLAS) {
         // Workaround bug in vulkan.hpp (from Vulkan SDK version 1.2.141.2). Refer https://github.com/KhronosGroup/Vulkan-Hpp/issues/633
         //m_Device.destroy(blas.m_AccelerationStructure);
         m_Device.destroyAccelerationStructureNV(blas.m_AccelerationStructure);
         blas.m_AccelerationStructure = nullptr;
         blas.m_Handle = 0;
      }
      if (m_BLAS.size() > 0) {
         if (m_BLAS.front().m_Memory) {
            m_Device.free(m_BLAS.front().m_Memory);
         }
      }
   }
}


void Application::CreateTopLevelAccelerationStructure(vk::ArrayProxy<const Vulkan::GeometryInstance> geometryInstances) {
   m_TLAS.m_AccelerationStructureInfo = {
      vk::AccelerationStructureTypeNV::eTopLevel                   /*type*/,
      vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace   /*flags*/,
      geometryInstances.size()                                     /*instanceCount*/,
      0                                                            /*geometryCount*/,
      nullptr                                                      /*pGeometries*/
   };

   m_TLAS.m_AccelerationStructure = m_Device.createAccelerationStructureNV({
      0                                  /*compactedSize*/,
      m_TLAS.m_AccelerationStructureInfo /*info*/
   });

   vk::MemoryRequirements2 memoryRequirementsTLAS = m_Device.getAccelerationStructureMemoryRequirementsNV({
      vk::AccelerationStructureMemoryRequirementsTypeNV::eObject /*type*/,
      m_TLAS.m_AccelerationStructure                             /*accelerationStructure*/
   });

   m_TLAS.m_Memory = m_Device.allocateMemory({
      memoryRequirementsTLAS.memoryRequirements.size /*allocationSize*/,
      Buffer::FindMemoryType(m_PhysicalDevice, memoryRequirementsTLAS.memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
   });

   vk::BindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo = {
      m_TLAS.m_AccelerationStructure /*accelerationStructure*/,
      m_TLAS.m_Memory                /*memory*/,
      0                              /*memoryOffset*/,
      0                              /*deviceIndexCount*/,
      nullptr                        /*pDeviceIndices*/
   };
   m_Device.bindAccelerationStructureMemoryNV(accelerationStructureMemoryInfo);
   vk::ArrayProxy<uint64_t> handle(m_TLAS.m_Handle);
   m_Device.getAccelerationStructureHandleNV<uint64_t>(m_TLAS.m_AccelerationStructure, handle);
}


void Application::DestroyTopLevelAccelerationStructure() {
   if (m_Device && m_TLAS.m_AccelerationStructure) {

      // Workaround bug in vulkan.hpp (from Vulkan SDK version 1.2.141.2). Refer https://github.com/KhronosGroup/Vulkan-Hpp/issues/633
      //m_Device.destroy(m_TLAS.m_AccelerationStructure);
      m_Device.destroyAccelerationStructureNV(m_TLAS.m_AccelerationStructure);

      m_TLAS.m_AccelerationStructure = nullptr;
      if (m_TLAS.m_Memory) {
         m_Device.free(m_TLAS.m_Memory);
         m_TLAS.m_Memory = nullptr;
      }
      m_TLAS.m_Handle = 0;
   }
}


void Application::BuildAccelerationStructures(vk::ArrayProxy<const Vulkan::GeometryInstance> geometryInstances) {
   std::vector<vk::MemoryRequirements2> memoryRequirements;
   for (const auto& blas : m_BLAS) {
      memoryRequirements.emplace_back(
         m_Device.getAccelerationStructureMemoryRequirementsNV({
            vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch /*type*/,
            blas.m_AccelerationStructure                                     /*accelerationStructure*/
         })
      );
   }

   vk::DeviceSize totalMemorySize = 0;
   for (const auto& memoryRequirement : memoryRequirements) {
      totalMemorySize += memoryRequirement.memoryRequirements.size;
   }

   Vulkan::Buffer scratchBufferBLAS = {
      m_Device,
      m_PhysicalDevice,
      totalMemorySize,
      vk::BufferUsageFlagBits::eRayTracingNV,
      vk::MemoryPropertyFlagBits::eDeviceLocal
   };

   Vulkan::Buffer instanceBuffer = {
      m_Device,
      m_PhysicalDevice,
      sizeof(Vulkan::GeometryInstance) * geometryInstances.size(),
      vk::BufferUsageFlagBits::eRayTracingNV,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
   };
   instanceBuffer.CopyFromHost(0, sizeof(Vulkan::GeometryInstance) * geometryInstances.size(), geometryInstances.data());

   vk::MemoryRequirements2 memoryRequirementsTLAS = m_Device.getAccelerationStructureMemoryRequirementsNV({
      vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch /*type*/,
      m_TLAS.m_AccelerationStructure                                   /*accelerationStructure*/
   });

   Vulkan::Buffer scratchBufferTLAS = {
      m_Device,
      m_PhysicalDevice,
      memoryRequirementsTLAS.memoryRequirements.size,
      vk::BufferUsageFlagBits::eRayTracingNV,
      vk::MemoryPropertyFlagBits::eDeviceLocal
   };

   SubmitSingleTimeCommands([&memoryRequirements, &scratchBufferBLAS, &instanceBuffer, &scratchBufferTLAS, this] (vk::CommandBuffer cmd) {
      vk::DeviceSize scratchOffset = 0;
      uint32_t i = 0;
      for (const auto& blas : m_BLAS) {
         // each build here has its own section of scratch memory.
         // so we can queue them all up and do not need a memory barrier between BLAS builds
         cmd.buildAccelerationStructureNV(blas.m_AccelerationStructureInfo, nullptr, 0, false, blas.m_AccelerationStructure, nullptr, scratchBufferBLAS.m_Buffer, scratchOffset);
         scratchOffset += memoryRequirements[i++].memoryRequirements.size;
      }

      // GPU must wait for BLAS build to finish before doing TLAS build
      // Also, you need to make sure the geometry instances have finished being copied to GPU before doing the TLAS build
      // but that is already covered because instanceBuffer.CopyFromHost() above blocks until the GPU has done it
      vk::MemoryBarrier memoryBarrier = {
         vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV /*srcAccesMask*/,
         vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV /*dstAccessMask*/
      };
      cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, {}, memoryBarrier, nullptr, nullptr);

      cmd.buildAccelerationStructureNV(m_TLAS.m_AccelerationStructureInfo, instanceBuffer.m_Buffer, 0, false, m_TLAS.m_AccelerationStructure, nullptr, scratchBufferTLAS.m_Buffer, 0);
   });
}


void glfwKeyCallback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods) {
   const auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
   app->OnKey(key, scancode, action, mods);
}


void glfwCursorPosCallback(GLFWwindow* window, const double xpos, const double ypos) {
   const auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
   app->OnCursorPos(xpos, ypos);
}


void glfwMouseButtonCallback(GLFWwindow* window, const int button, const int action, const int mods) {
   const auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
   app->OnMouseButton(button, action, mods);
}

void glfwFramebufferResizeCallback(GLFWwindow* window, const int width, const int height) {
   auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
   app->m_WantResize = true;
}


}