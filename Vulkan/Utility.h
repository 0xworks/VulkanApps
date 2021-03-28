#pragma once

#include "SwapChainSupportDetails.h"

#include <vulkan/vulkan.hpp>
#include <vector>

#define NON_COPYABLE(ClassName)                       \
   ClassName(const ClassName&) = delete;              \
   ClassName(ClassName&&) = delete;                   \
   ClassName& operator = (const ClassName&) = delete; \
   ClassName& operator = (ClassName&&) = delete

namespace Vulkan {

void glfwErrorCallback(const int error, const char* const description);

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanErrorCallback(
   VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
   VkDebugUtilsMessageTypeFlagsEXT messageType,
   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
   void* pUserData
);


void CheckLayerSupport(std::vector<const char*> layers);

void CheckInstanceExtensionSupport(std::vector<const char*> extensions);

bool CheckDeviceExtensionSupport(vk::PhysicalDevice physicalDevice, std::vector<const char*> extensions);

SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);

bool HasStencilComponent(vk::Format format);

std::vector<char> ReadFile(const std::string& filename);

uint32_t AlignedSize(uint32_t value, uint32_t alignment);

}