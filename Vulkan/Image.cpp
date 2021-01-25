#include "Image.h"

#include "Buffer.h"

namespace Vulkan {

Image::Image(vk::Device device, const vk::PhysicalDevice physicalDevice, const vk::ImageViewType type, const uint32_t width, const uint32_t height, const uint32_t mipLevels, vk::SampleCountFlagBits numSamples, const vk::Format format, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage, const vk::MemoryPropertyFlags properties)
: m_Device(device)
, m_Type(type)
{
   vk::ImageType imageType = vk::ImageType::e2D;
   switch (type) {
      case vk::ImageViewType::e1D:
      case vk::ImageViewType::e1DArray:
         imageType = vk::ImageType::e1D;
         break;
      case vk::ImageViewType::e2D:
      case vk::ImageViewType::e2DArray:
      case vk::ImageViewType::eCube:
      case vk::ImageViewType::eCubeArray:
         imageType = vk::ImageType::e2D;
         break;
      case vk::ImageViewType::e3D:
         imageType = vk::ImageType::e3D;
         break;
   }

   vk::ImageCreateFlags flags = {};
   switch (type) {
      case vk::ImageViewType::e1D:
      case vk::ImageViewType::e1DArray:
      case vk::ImageViewType::e2D:
      case vk::ImageViewType::e3D:
         flags = {};
         break;
      case vk::ImageViewType::e2DArray:
         flags = vk::ImageCreateFlagBits::e2DArrayCompatible;
         break;
      case vk::ImageViewType::eCube:
      case vk::ImageViewType::eCubeArray:
         flags = vk::ImageCreateFlagBits::eCubeCompatible;
         break;
   }

   uint32_t arrayLayers = 1u;
   switch (type) {
      case vk::ImageViewType::eCube:
      case vk::ImageViewType::eCubeArray:
         arrayLayers = 6;
         break;
   }

   m_Image = m_Device.createImage({
      flags                            /*flags*/,
      imageType                        /*imageType*/,
      format                           /*format*/,
      {width, height, 1}               /*extent*/,
      mipLevels                        /*mipLevels*/,
      arrayLayers                      /*arrayLayers*/,
      numSamples                       /*samples*/,
      tiling                           /*tiling*/,
      usage                            /*usage*/,
      vk::SharingMode::eExclusive      /*sharingMode*/,
      0                                /*queueFamilyIndexCount*/,
      nullptr                          /*pQueueFamilyIndices*/,
      vk::ImageLayout::eUndefined      /*initialLayout*/
   });

   vk::MemoryRequirements memRequirements = m_Device.getImageMemoryRequirements(m_Image);
   m_Memory = m_Device.allocateMemory({
      memRequirements.size                                        /*allocationSize*/,
      Buffer::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)  /*memoryTypeIndex*/
   });
   m_Device.bindImageMemory(m_Image, m_Memory, 0);
}


Image::Image(vk::Device device, const vk::Image& image)
: m_Device(device)
, m_Type(vk::ImageViewType::e2D)
, m_Image(image)
{}


Image::Image(Image&& that) {
   *this = std::move(that);
}


Image::~Image() {
   if (m_Device) {
      if (m_ImageView) {
         m_Device.destroy(m_ImageView);
         m_ImageView = nullptr;
      }
      if(m_Memory) {
         //
         // only destroy the image if it has some memory
         // i.e. we allocated the image, so we destroy it.
         // as opposed to images that were created (and are destroyed) by
         // the swap chain.
         if (m_Image) {
            m_Device.destroy(m_Image);
            m_Image = nullptr;
         }
         m_Device.freeMemory(m_Memory);
         m_Memory = nullptr;
      }
   }
}


Image& Image::operator=(Image&& that) {
   if (this != &that) {
      m_Device = that.m_Device;
      m_Image = that.m_Image;
      m_ImageView = that.m_ImageView;
      that.m_Device = nullptr;
      that.m_Image = nullptr;
      that.m_ImageView = nullptr;
   }
   return *this;
}


void Image::CreateImageView(const vk::Format format, const vk::ImageAspectFlags imageAspect, const uint32_t mipLevels) {
   m_ImageView = m_Device.createImageView({
      {}                                 /*flags*/,
      m_Image                            /*image*/,
      m_Type                             /*viewType*/,
      format                             /*format*/,
      {}                                 /*components*/,
      {
         imageAspect                     /*aspectMask*/,
         0                                  /*baseMipLevel*/,
         mipLevels                          /*levelCount*/,
         0                                  /*baseArrayLevel*/,
         VK_REMAINING_ARRAY_LAYERS          /*layerCount*/
      }                                  /*subresourceRange*/
   });
}


void Image::DestroyImageView() {
   if (m_Device && m_ImageView) {
      m_Device.destroy(m_ImageView);
      m_ImageView = nullptr;
   }
}

}