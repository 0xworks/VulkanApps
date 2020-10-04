#pragma once

#include "Buffer.h"
#include "GeometryInstance.h"
#include "Image.h"
#include "QueueFamilyIndices.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

struct GLFWwindow;

namespace Vulkan {

struct ApplicationSettings {
   const char* ApplicationName = "Vulkan::Application";
   uint32_t ApplicationVersion = VK_MAKE_VERSION(0,1,0);
   uint32_t MaxFramesInFlight = 2;
   uint32_t WindowWidth = 800;
   uint32_t WindowHeight = 600;
   bool IsResizable = true;
   bool IsFullScreen = false;
   bool IsCursorEnabled = true;
};


struct AccelerationStructure {
   AccelerationStructure() = default;
   AccelerationStructure(vk::AccelerationStructureInfoNV accelerationStructureInfo) : m_AccelerationStructureInfo(accelerationStructureInfo) {}
   vk::AccelerationStructureInfoNV m_AccelerationStructureInfo;
   vk::AccelerationStructureNV m_AccelerationStructure;
   vk::DeviceMemory m_Memory;
   uint64_t m_Handle = 0;
};


class Application {

public:
   Application(const ApplicationSettings& settings, const bool enableValidation);
   virtual ~Application();

   void Run();

   virtual void OnKey(const int key, const int scancode, const int action, const int mods);
   virtual void OnCursorPos(const double xpos, const double ypos);
   virtual void OnMouseButton(const int button, const int action, const int mods);

protected:
   // Initialise self.
   // This will create most of the vulkan objects required for a working app.
   // At a minimum, derived app must override Init() to provide the graphics pipeline.
   // The derived app does not have to (but can) override individual vulkan components
   // (such as the default depth stencil, renderpass, framebuffers, etc.) by providing
   // implementations for the various virtual CreateXXXX methods
   virtual void Init();

   virtual void CreateWindow();
   virtual void DestroyWindow();

   virtual void CreateInstance();
   virtual void DestroyInstance();

   // Return any extra instance extensions required by derived app.
   // Base implementation already requires:
   //    glfw extensions
   //    VK_EXT_debug_utils
   // So only extensions additional to these need be returned.
   virtual std::vector<const char*> GetRequiredInstanceExtensions();

   virtual void CreateSurface();
   virtual void DestroySurface();

   // Choose surface format from given available formats
   // Base implementation looks for eB8G8R8A8Unorm
   virtual vk::SurfaceFormatKHR SelectSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

   // Choose present mode from given available present modes
   // Base implementation chooses mailbox if available, else fifo
   virtual vk::PresentModeKHR SelectPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

   virtual void SelectPhysicalDevice();

   // Return physical device features required by derived app, given the specified
   // set of available features.
   // Base implementation returns nothing
   virtual vk::PhysicalDeviceFeatures GetRequiredPhysicalDeviceFeatures(vk::PhysicalDeviceFeatures availableFeatures);

   // Return physical device extension features required by derived app.
   // This is passed as the pNext parameter of device create info
   // Base implementation returns nullptr
   virtual void* GetRequiredPhysicalDeviceFeaturesEXT();

   virtual bool IsPhysicalDeviceSuitable(vk::PhysicalDevice physicalDevice);

   virtual void CreateDevice();
   virtual void DestroyDevice();

   // Return any extra device extensions required by derived app.
   // Base implementation returns nothing.
   virtual std::vector<const char*> GetRequiredDeviceExtensions();


   ////////////////////////////////////////////////
   //
   // swap chain stuff
   virtual void CreateSwapChain();
   virtual void DestroySwapChain(vk::SwapchainKHR& swapChain);

   virtual void CreateImageViews();
   virtual void DestroyImageViews();

   virtual vk::Extent2D SelectSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
   //
   //
   //////////////////////////////////////////////

   virtual void CreateDepthStencil();
   virtual void DestroyDepthStencil();

   virtual void CreateRenderPass(); // depends on swapchain (specifically m_Format and m_Extent), and on depth stencil (specifically m_DepthFormat)
   virtual void DestroyRenderPass();

   virtual void CreateFrameBuffers(); // depends on renderpass
   virtual void DestroyFrameBuffers();

   virtual void CreateCommandPool();
   virtual void DestroyCommandPool();

   virtual void CreateCommandBuffers();   // by default, we allocate one command buffer per framebuffer.  Derived app may do something different
   virtual void DestroyCommandBuffers();

   virtual void CreateSyncObjects();
   virtual void DestroySyncObjects();

   virtual void CreatePipelineCache();
   virtual void DestroyPipelineCache();

   virtual void Update(double deltaTime);

   virtual void BeginFrame();

   virtual void RenderFrame();

   virtual void EndFrame();

   virtual void OnWindowResized();

protected:

   vk::ShaderModule CreateShaderModule(std::vector<char> code);
   void DestroyShaderModule(vk::ShaderModule& module);

   QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice);

   vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

   void SubmitSingleTimeCommands(const std::function<void(vk::CommandBuffer)>& action);

   void CopyBuffer(vk::Buffer src, vk::Buffer dst, const vk::DeviceSize srcOffset, const vk::DeviceSize dstOffset, const vk::DeviceSize size);

   void TransitionImageLayout(vk::Image image, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout, const uint32_t mipLevels);

   void CopyBufferToImage(vk::Buffer buffer, vk::Image image, const uint32_t width, const uint32_t height);

   void GenerateMIPMaps(vk::Image image, const vk::Format format, const uint32_t width, const uint32_t height, uint32_t mipLevels);

   ///////////////////////////////
   // Ray tracing stuff

   void CreateBottomLevelAccelerationStructures(vk::ArrayProxy<const std::vector<vk::GeometryNV>> geometryGroups);
   void DestroyBottomLevelAccelerationStructures();

   void CreateTopLevelAccelerationStructure(vk::ArrayProxy<const Vulkan::GeometryInstance> geometryInstances);
   void DestroyTopLevelAccelerationStructure();

   void BuildAccelerationStructures(vk::ArrayProxy<const Vulkan::GeometryInstance> geometryInstances);
   //
   //////////////////////////////

protected:
   ApplicationSettings m_Settings;
   bool m_EnableValidation;

   GLFWwindow* m_Window = nullptr;

   vk::Instance m_Instance;
   vk::DebugUtilsMessengerEXT m_DebugUtilsMessengerEXT;
   vk::SurfaceKHR m_Surface;

   vk::PhysicalDevice m_PhysicalDevice;
   vk::PhysicalDeviceProperties m_PhysicalDeviceProperties;
   vk::PhysicalDeviceFeatures m_PhysicalDeviceFeatures;                 // features that are available on the selected physical device
   vk::PhysicalDeviceFeatures m_EnabledPhysicalDeviceFeatures;          // features that have been enabled
   vk::PhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
   QueueFamilyIndices m_QueueFamilyIndices;

   vk::Device m_Device;

   vk::Queue m_GraphicsQueue;
   vk::Queue m_PresentQueue;

   // swap chain stuff (encapsulate?) ///////////////
   vk::Format m_Format = vk::Format::eUndefined;
   vk::Extent2D m_Extent;
   vk::SwapchainKHR m_SwapChain;
   std::vector<Image> m_SwapChainImages;
   bool m_WantResize = false;
   //////////////////////////////////////////////////

   vk::Format m_DepthFormat;
   std::unique_ptr<Image> m_DepthImage;

   vk::RenderPass m_RenderPass;

   std::vector<vk::Framebuffer> m_SwapChainFrameBuffers;

   vk::CommandPool m_CommandPool;
   std::vector<vk::CommandBuffer> m_CommandBuffers;

   uint32_t m_CurrentFrame = 0; // which frame (up to MaxFramesInFlight) are we currently rendering
   uint32_t m_CurrentImage = 0; // which swap chain image are we currently rendering to
   std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
   std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
   std::vector<vk::Fence> m_InFlightFences;

   vk::PipelineCache m_PipelineCache;

   double m_LastTime = 0.0;

   ////////////////////////////
   // Ray tracing stuff
   std::vector<AccelerationStructure> m_BLAS;
   AccelerationStructure m_TLAS;
   //
   ///////////////////////////


   ////////////////////////////
   // TODO: abstract this into a camera controller object...
   //
   glm::vec3 m_Eye = {0.0f, 0.0f, 4.0f};
   glm::vec3 m_Direction = glm::normalize(glm::vec3 {0.0f, 0.0f, -4.0f});
   glm::vec3 m_Up = glm::normalize(glm::vec3 {0.0f, 1.0f, 0.0f});
   float m_FoVRadians = glm::radians(45.0f);
   //
   ////////////////////////////////

   double m_MouseX = 0.0;
   double m_MouseY = 0.0;
   bool m_LeftMouseDown = false;
   friend class Buffer;
   friend class Image;
   friend void glfwKeyCallback(GLFWwindow*, const int, const int, const int, const int);
   friend void glfwCursorPosCallback(GLFWwindow*, const double, const double);
   friend void glfwMouseButtonCallback(GLFWwindow*, const int, const int, const int);
   friend void glfwFramebufferResizeCallback(GLFWwindow*, const int, const int);
};

}


// Must be defined in client
extern std::unique_ptr<Vulkan::Application> CreateApplication(int argc, const char* argv[]);
