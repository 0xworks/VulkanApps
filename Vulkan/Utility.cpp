#include "Utility.h"

#include "Log.h"

#include <fstream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Vulkan {

void glfwErrorCallback(const int error, const char* const description) {
   CORE_LOG_ERROR("GLFW: {0} (code: {1})", description, error);
}


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanErrorCallback(
   VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
   VkDebugUtilsMessageTypeFlagsEXT messageType,
   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
   void* pUserData
) {
   switch (messageSeverity) {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
         CORE_LOG_TRACE("VULKAN VALIDATION: {0}", pCallbackData->pMessage);
         break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
         CORE_LOG_INFO("VULKAN VALIDATION: {0}", pCallbackData->pMessage);
         break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
         CORE_LOG_WARN("VULKAN VALIDATION: {0}", pCallbackData->pMessage);
         break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
         CORE_LOG_ERROR("VULKAN VALIDATION: {0}", pCallbackData->pMessage);
         break;
   }
   return VK_FALSE;
}


// 
// 
// std::vector<const char*> GetRequiredDeviceExtensions() {
//    static std::vector<const char*> deviceExtensions = {
//        VK_KHR_SWAPCHAIN_EXTENSION_NAME
//    };
//    return deviceExtensions;
// }


void CheckLayerSupport(std::vector<const char*> layers) {
   std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
   for (const auto& layer : layers) {
      bool layerFound = false;
      for (const auto& availableLayer : availableLayers) {
         if (strcmp(layer, availableLayer.layerName) == 0) {
            layerFound = true;
            break;
         }
      }
      if (!layerFound) {
         CORE_LOG_INFO("available layers:");
         for (const auto& layer : availableLayers) {
            CORE_LOG_INFO("\t{0}", layer.layerName);
         }

         std::string msg {"layer '"};
         msg += layer;
         msg += "' requested but is not available!";
         throw std::runtime_error(msg);
      }
   }
}


void CheckInstanceExtensionSupport(std::vector<const char*> extensions) {
   std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
   for (const auto& extension : extensions) {
      bool extensionFound = false;
      for (const auto& availableExtension : availableExtensions) {
         if (strcmp(extension, availableExtension.extensionName) == 0) {
            extensionFound = true;
            break;
         }
      }
      if (!extensionFound) {
         CORE_LOG_INFO("available extensions:");
         for (const auto& extension : availableExtensions) {
            CORE_LOG_INFO("\t{0}", extension.extensionName);
         }
         std::string msg {"extension '"};
         msg += extension;
         msg += "' requested but is not available!";
         throw std::runtime_error(msg);
      }
   }
}


bool CheckDeviceExtensionSupport(vk::PhysicalDevice physicalDevice, std::vector<const char*> extensions) {
   bool allExtensionsFound = true;
   std::vector<vk::ExtensionProperties> availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
   for (const auto& extension : extensions) {
      bool extensionFound = false;
      for (const auto& availableExtension : availableExtensions) {
         if (strcmp(extension, availableExtension.extensionName) == 0) {
            extensionFound = true;
            break;
         }
      }
      if (!extensionFound) {
         allExtensionsFound = false;
         break;
      }
   }
   return allExtensionsFound;
}


SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
   return {
      device.getSurfaceCapabilitiesKHR(surface),
      device.getSurfaceFormatsKHR(surface),
      device.getSurfacePresentModesKHR(surface)
   };
}


bool HasStencilComponent(vk::Format format) {
   return (format == vk::Format::eD32SfloatS8Uint) || (format == vk::Format::eD24UnormS8Uint);
}


std::vector<char> ReadFile(const std::string& filename) {
   std::ifstream file(filename, std::ios::ate | std::ios::binary);

   if (!file.is_open()) {
      throw std::runtime_error("failed to open file '" + filename + "'");
   }

   size_t fileSize = (size_t)file.tellg();
   std::vector<char> buffer(fileSize);

   file.seekg(0);
   file.read(buffer.data(), fileSize);

   file.close();

   return buffer;
}

}