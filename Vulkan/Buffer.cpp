#include "Buffer.h"
#include "Core.h"

namespace Vulkan {

Buffer::Buffer(vk::Device device, const vk::PhysicalDevice physicalDevice, const vk::DeviceSize size, const vk::BufferUsageFlags usage, const vk::MemoryPropertyFlags memoryProperties)
: m_Device(device)
, m_Size(size)
, m_Usage(usage)
, m_Properties(memoryProperties)
{
   vk::BufferCreateInfo ci = {
      {}                                         /*flags*/,
      size                                       /*size*/,
      usage                                      /*usage*/,
      vk::SharingMode::eExclusive                /*sharingMode*/,
      0                                          /*queueFamilyIndexCount*/,
      nullptr                                    /*pQueueFamilyIndices*/
   };
   m_Buffer = m_Device.createBuffer(ci);

   const auto requirements = m_Device.getBufferMemoryRequirements(m_Buffer);
   vk::MemoryAllocateInfo ai = {
      requirements.size                                                        /*allocationSize*/,
      FindMemoryType(physicalDevice, requirements.memoryTypeBits, memoryProperties)  /*memoryTypeIndex*/
   };
   vk::MemoryAllocateFlagsInfo afi = {
      vk::MemoryAllocateFlagBits::eDeviceAddress /*flags*/,
      {}                                         /*deviceMask*/
   };
   if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
      ai.setPNext(&afi);
   }
   m_Memory = m_Device.allocateMemory(ai);
   m_Device.bindBufferMemory(m_Buffer, m_Memory, 0);
   m_Descriptor.buffer = m_Buffer;
   m_Descriptor.offset = 0;
   m_Descriptor.range = size;
}


Buffer::Buffer(Buffer&& that) {
   *this = std::move(that);
}


Buffer& Buffer::operator=(Buffer&& that) {
   if (this != &that) {
      m_Device = that.m_Device;
      m_Buffer = that.m_Buffer;
      m_Memory = that.m_Memory;
      m_Descriptor = that.m_Descriptor;
      m_Size = that.m_Size;
      m_Usage = that.m_Usage;
      m_Properties = that.m_Properties;
      that.m_Device = nullptr;
      that.m_Buffer = nullptr;
      that.m_Memory = nullptr;
      that.m_Descriptor = vk::DescriptorBufferInfo{};
      that.m_Size = 0;
      that.m_Usage = {};
      that.m_Properties = {};
   }
   return *this;
}


Buffer::~Buffer() {
   if (m_Device) {
      if (m_Buffer) {
         m_Device.destroy(m_Buffer);
         m_Buffer = nullptr;
      }
      if (m_Memory) {
         m_Device.freeMemory(m_Memory);
         m_Memory = nullptr;
      }
   }
}


vk::DeviceAddress Buffer::GetBufferDeviceAddress() const {
   return m_Device.getBufferAddress({ m_Buffer });
}


uint32_t Buffer::FindMemoryType(const vk::PhysicalDevice physicalDevice, const uint32_t typeFilter, const vk::MemoryPropertyFlags properties) {
   vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
   for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
      if ((typeFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
         return i;
      }
   }
   throw std::runtime_error("failed to find suitable memory type!");
}


void Buffer::CopyFromHost(const vk::DeviceSize offset, const vk::DeviceSize sizeArg, const void* pData) {
   vk::DeviceSize size = (sizeArg == VK_WHOLE_SIZE) ? m_Size : sizeArg;
   CORE_ASSERT(size <= m_Size, "Cannot copy in excess of buffer size!");
   void* pDataDst = m_Device.mapMemory(m_Memory, offset, size);
   memcpy(pDataDst, pData, static_cast<size_t>(size));
   m_Device.unmapMemory(m_Memory);
}


IndexBuffer::IndexBuffer(vk::Device device, const vk::PhysicalDevice physicalDevice, const vk::DeviceSize size, const uint32_t count, const vk::BufferUsageFlags usage, const vk::MemoryPropertyFlags properties)
: Buffer(device, physicalDevice, size, usage, properties)
, m_Count(count)
{}


}