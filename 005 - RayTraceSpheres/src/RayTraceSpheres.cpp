#include "RayTraceSpheres.h"

#include "Bindings.h"
#include "Core.h"
#include "GeometryInstance.h"
#include "Material.h"
#include "Utility.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <random>

#define M_PI 3.14159265358979323846f

std::unique_ptr<Vulkan::Application> CreateApplication(int argc, const char* argv[]) {
   return std::make_unique<RayTraceSpheres>(argc, argv);
}


inline float random_float() {
   static std::uniform_real_distribution<float> distribution(0.0, 1.0);
   static std::mt19937 generator;
   static std::function<float()> rand_generator = std::bind(distribution, generator);
   return rand_generator();
}


Material Lambertian(glm::vec3 albedo) {
   return Material {glm::vec4{albedo, 1 }, MATERIAL_LAMBERTIAN};
}


Material Metallic(glm::vec3 albedo, float roughness) {
   return Material {glm::vec4{albedo, 1 }, MATERIAL_METALLIC, roughness};
}


Material Dielectric(float refractiveIndex) {
   return Material {glm::one<vec4>(), MATERIAL_DIELECTRIC, 0.0, refractiveIndex};
}


RayTraceSpheres::RayTraceSpheres(int argc, const char* argv[])
: Vulkan::Application {
   { "Example 1", VK_MAKE_VERSION(1,0,0) },
#ifdef _DEBUG
   /*enableValidation=*/true
#else
   /*enableValidation=*/false
#endif
}
, m_bindir { argv[0] }
, m_UniformBufferObject { glm::identity<mat4>(), glm::identity<mat4>(), 0 }
{
   m_bindir.remove_filename();
   Init();
}


RayTraceSpheres::~RayTraceSpheres() {
   DestroyDescriptorSets();
   DestroyDescriptorPool();
   DestroyShaderBindingTable();
   DestroyPipeline();
   DestroyPipelineLayout();
   DestroyDescriptorSetLayout();
   DestroyUniformBuffers();
   DestroyStorageImages();
   DestroyScene();
}


std::vector<const char*> RayTraceSpheres::GetRequiredInstanceExtensions() {
   return {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
}


std::vector<const char*> RayTraceSpheres::GetRequiredDeviceExtensions() {
   return {
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
      VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
   };
}


vk::PhysicalDeviceFeatures RayTraceSpheres::GetRequiredPhysicalDeviceFeatures(vk::PhysicalDeviceFeatures availableFeatures) {
   vk::PhysicalDeviceFeatures features;
   if (availableFeatures.samplerAnisotropy) {
      features.setSamplerAnisotropy(true);
   } else {
      ASSERT(false, "Device does not support sampler anisotropy")
   }
   if (availableFeatures.fillModeNonSolid) {
      features.setFillModeNonSolid(true);
   } else {
      ASSERT(false, "Device does not support fill mode non solid")
   }
   if (availableFeatures.shaderInt64) {
      features.setShaderInt64(true);
   } else {
      ASSERT(false, "Device does not support shader int64")
   }
   return features;
}


void* RayTraceSpheres::GetRequiredPhysicalDeviceFeaturesEXT() {

   static vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures;
   bufferDeviceAddressFeatures.bufferDeviceAddress = true;

   static vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;
   indexingFeatures.runtimeDescriptorArray = true;
   indexingFeatures.pNext = &bufferDeviceAddressFeatures;

   static vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures;
   accelerationStructureFeatures.accelerationStructure = true;
   accelerationStructureFeatures.pNext = &indexingFeatures;

   static vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures;
   rayTracingFeatures.rayTracingPipeline = true;
   rayTracingFeatures.pNext = &accelerationStructureFeatures;

   return &rayTracingFeatures;
}


void RayTraceSpheres::Init() {
   Vulkan::Application::Init();

   m_Eye = {8.0f, 2.0f, 2.0f};
   m_Direction = {-8.0f, -2.0f, -2.0f};
   m_Up = {0.0f, 1.0f, 0.0f};

   m_Constants = {
      8,
      0.0,
      glm::length(m_Direction)
   };

   m_Direction = glm::normalize(m_Direction);

   auto properties = m_PhysicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
   m_RayTracingProperties = properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

   // Check requested push constant size against hardware limit
   // Specs require 128 bytes, so if the device complies our push constant buffer should always fit into memory
   if (properties.get<vk::PhysicalDeviceProperties2>().properties.limits.maxPushConstantsSize < sizeof(Constants)) {
      throw std::runtime_error("(Push)Constants too large");
   }

   CreateScene();
   CreateStorageImages();
   CreateUniformBuffers();
   CreateDescriptorSetLayout();
   CreatePipelineLayout();
   CreatePipeline();
   CreateShaderBindingTable();
   CreateDescriptorPool();
   CreateDescriptorSets();
   RecordCommandBuffers();
}


void RayTraceSpheres::CreateScene() {

   std::vector<Vertex> vertices;
   std::vector<uint32_t> indices;

   // Load models
   {
      tinyobj::attrib_t attrib;
      std::vector<tinyobj::shape_t> shapes;
      std::vector<tinyobj::material_t> materials;
      std::string warn, err;

      if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (m_bindir / "Assets" / "Models" / "sphere.obj").string().c_str())) {
         throw std::runtime_error(warn + err);
      }


      std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

      for (const auto& shape : shapes) {
         for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {
               {
                  attrib.vertices[3 * index.vertex_index + 0],
                  attrib.vertices[3 * index.vertex_index + 1],
                  attrib.vertices[3 * index.vertex_index + 2],
               } /*Pos*/,
               {
                  attrib.normals[3 * index.normal_index + 0],
                  attrib.normals[3 * index.normal_index + 1],
                  attrib.normals[3 * index.normal_index + 2],
               } /*Normal*/,
            };

            if (uniqueVertices.count(vertex) == 0) {
               uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
               vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
         }
      }
   }

   // Create Vertex buffer
   {
      vk::DeviceSize size = vertices.size() * sizeof(Vertex);

      Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
      stagingBuffer.CopyFromHost(0, size, vertices.data());

      m_VertexBuffer = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, vk::MemoryPropertyFlagBits::eDeviceLocal);
      CopyBuffer(stagingBuffer.m_Buffer, m_VertexBuffer->m_Buffer, 0, 0, size);
   }

   // Create Index buffer
   {
      uint32_t count = static_cast<uint32_t>(indices.size());
      vk::DeviceSize size = count * sizeof(uint32_t);

      Vulkan::Buffer stagingBuffer = {
         m_Device,
         m_PhysicalDevice,
         size,
         vk::BufferUsageFlagBits::eTransferSrc,
         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
      };
      stagingBuffer.CopyFromHost(0, size, indices.data());

      m_IndexBuffer = std::make_unique<Vulkan::IndexBuffer>(m_Device, m_PhysicalDevice, size, count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, vk::MemoryPropertyFlagBits::eDeviceLocal);
      CopyBuffer(stagingBuffer.m_Buffer, m_IndexBuffer->m_Buffer, 0, 0, size);
   }

   // Create Transform buffer (if you want to instance the objects in BLAS)
   {
      //    std::array<glm::mat3x4, 1> transforms = {
      //       glm::mat3x4 {
      //          1.0f, 0.0f, 0.0f, 0.0f,
      //          0.0f, 1.0f, 0.0f, 0.0f,
      //          0.0f, 0.0f, 1.0f, 0.0f,
      //       }
      //    };
      // 
      //    vk::DeviceSize size = transforms.size() * sizeof(glm::mat3x4);
      // 
      //    Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
      //    stagingBuffer.CopyFromHost(0, size, transforms.data());
      // 
      //    m_TransformBuffer = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
      //    CopyBuffer(stagingBuffer.m_Buffer, m_TransformBuffer->m_Buffer, 0, 0, size);
   }

   // Could create multiple geometries here (on same vertex/index buffer) by specifying the offsets and counts.
   // You can also instantiate the same geometry more than once (using the transformData and transformOffset)
   Vulkan::GeometryGroup geometryGroup;
   geometryGroup.AddTrianglesIndexed(m_VertexBuffer->GetBufferDeviceAddress(), 0, sizeof(Vertex), vertices.size(), m_IndexBuffer->GetBufferDeviceAddress(), 0, indices.size(), 0, vertices.size());

   CreateBottomLevelAccelerationStructures(geometryGroup);

   // create random scene
   uint32_t i = 0;
   std::vector<vk::AccelerationStructureInstanceKHR> instances;
   std::vector<Material> materials;

   // large background sphere
   instances.emplace_back(
      VkTransformMatrixKHR {
         1000.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1000.0f, 0.0f, -1000.0f,
         0.0f, 0.0f, 1000.0f, -0.0f,
      }                              /*transform*/,
      i++                            /*instance index*/,
      0xff                           /*visibility mask*/,
      0                              /*hit group index*/,
      vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
      m_BLAS.back().m_DeviceAddress
   );
   materials.emplace_back(Lambertian({0.5, 0.5, 0.5}));

   for (int a = -11; a < 11; a++) {
      for (int b = -11; b < 11; b++) {
         float choose_mat = random_float();
         vec3 center(a + 0.9f * random_float(), 0.2f, b + 0.9f * random_float());
         if ((center - vec3(4.0f, 0.2f, 0.0f)).length() > 0.9f) {
            instances.emplace_back(
               VkTransformMatrixKHR{
                  0.2f, 0.0f, 0.0f, center.x,
                  0.0f, 0.2f, 0.0f, center.y,
                  0.0f, 0.0f, 0.2f, center.z,
               }                              /*transform*/,
               i++                            /*instance index*/,
               0xff                           /*visibility mask*/,
               0                              /*hit group index*/,
               vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
               m_BLAS.back().m_DeviceAddress
            );
            if (choose_mat < 0.8) {
               // diffuse
               materials.emplace_back(Lambertian(
                  {random_float() * random_float(), random_float() * random_float(), random_float() * random_float()}
               ));
            } else if (choose_mat < 0.95) {
               // metal
               materials.emplace_back(Metallic(
                  {0.5 * (1 + random_float()), 0.5 * (1 + random_float()), 0.5 * (1 + random_float())},
                  0.5f * random_float()
               ));
            } else {
               // glass
               materials.emplace_back(Dielectric(1.5f));
            }
         }
      }
   }

   // the three main spheres...
   instances.emplace_back(
      VkTransformMatrixKHR{
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 1.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
      }                              /*transform*/,
      i++                            /*instance index*/,
      0xff                           /*visibility mask*/,
      0                              /*hit group index*/,
      vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
      m_BLAS.back().m_DeviceAddress
   );
   materials.emplace_back(Dielectric(1.5f));

   instances.emplace_back(
      VkTransformMatrixKHR{
         1.0f, 0.0f, 0.0f, -4.0f,
         0.0f, 1.0f, 0.0f, 1.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
      }                              /*transform*/,
      i++                            /*instance index*/,
      0xff                           /*visibility mask*/,
      0                              /*hit group index*/,
      vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
      m_BLAS.back().m_DeviceAddress
   );
   materials.emplace_back(Lambertian({0.4, 0.2, 0.1}));

   instances.emplace_back(
      VkTransformMatrixKHR{
         1.0f, 0.0f, 0.0f, 4.0f,
         0.0f, 1.0f, 0.0f, 1.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
      }                              /*transform*/,
      4                              /*instance index*/,
      0xff                           /*visibility mask*/,
      0                              /*hit group index*/,
      vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
      m_BLAS.back().m_DeviceAddress
   );
   materials.emplace_back(Metallic({0.7, 0.6, 0.5}, 0.0));

   // Create Instances buffer
   Vulkan::Buffer instanceBuffer{
      m_Device,
      m_PhysicalDevice,
      sizeof(vk::AccelerationStructureInstanceKHR) * instances.size(),
      vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
   };
   instanceBuffer.CopyFromHost(0, VK_WHOLE_SIZE, instances.data());

   // Create Materials buffer
   {
      vk::DeviceSize size = materials.size() * sizeof(Material);

      Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
      stagingBuffer.CopyFromHost(0, size, materials.data());

      m_MaterialBuffer = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
      CopyBuffer(stagingBuffer.m_Buffer, m_MaterialBuffer->m_Buffer, 0, 0, size);
   }

   Vulkan::GeometryGroup group;
   group.AddInstances(instanceBuffer.GetBufferDeviceAddress(), 0, false, instances.size());
   CreateTopLevelAccelerationStructure(group);

}


void RayTraceSpheres::DestroyScene() {
   DestroyTopLevelAccelerationStructure();
   DestroyBottomLevelAccelerationStructures();
   m_MaterialBuffer.reset(nullptr);
   m_TransformBuffer.reset(nullptr);
   m_TransformBuffer.reset(nullptr);
   m_IndexBuffer.reset(nullptr);
   m_VertexBuffer.reset(nullptr);
}


void RayTraceSpheres::CreateStorageImages() {
   m_OutputImage = std::make_unique<Vulkan::Image>(m_Device, m_PhysicalDevice, vk::ImageViewType::e2D, m_Extent.width, m_Extent.height, 1, vk::SampleCountFlagBits::e1, m_Format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal);
   m_OutputImage->CreateImageView(m_Format, vk::ImageAspectFlagBits::eColor, 1);
   TransitionImageLayout(m_OutputImage->m_Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1);

   m_AccumumlationImage = std::make_unique<Vulkan::Image>(m_Device, m_PhysicalDevice, vk::ImageViewType::e2D, m_Extent.width, m_Extent.height, 1, vk::SampleCountFlagBits::e1, vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal);
   m_AccumumlationImage->CreateImageView(vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor, 1);
   TransitionImageLayout(m_AccumumlationImage->m_Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1);
}


void RayTraceSpheres::DestroyStorageImages() {
   m_AccumumlationImage.reset(nullptr);
   m_OutputImage.reset(nullptr);
}


void RayTraceSpheres::CreateUniformBuffers() {
   vk::DeviceSize size = sizeof(UniformBufferObject);

   // We are potentially queuing up more than one command buffer for rendering at once.
   // (see m_Settings.MaxFramesInFlight).
   // This means that each command buffer needs its own uniform buffer.
   // (if they all shared one uniform buffer, then we might start updating the uniform buffer
   // when a previously queued command buffer hasn't yet finished rendering).
   //
   // If we had instead made sure that only one command buffer is ever rendering at once,
   // then we could get away with just one uniform buffer.

   //
   // TODO: experiment: We are creating the uniform buffers as hostVisible and hostCoherent
   //                   Would it be better to have them as device-local  (and copy after each update)?
   m_UniformBuffers.reserve(m_CommandBuffers.size());
   for (const auto& commandBuffer : m_CommandBuffers) {
      m_UniformBuffers.emplace_back(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
   }
}


void RayTraceSpheres::DestroyUniformBuffers() {
   m_UniformBuffers.clear();
}


void RayTraceSpheres::CreateDescriptorSetLayout() {
   // Setup layout of descriptors used in this example
   // Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
   // So every shader binding should map to one descriptor set layout binding

   vk::DescriptorSetLayoutBinding accelerationStructureLB = {
      BINDING_TLAS                                                                  /*binding*/,
      vk::DescriptorType::eAccelerationStructureKHR                                 /*descriptorType*/,
      1                                                                             /*descriptorCount*/,
      vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR /*stageFlags*/,
      nullptr                                                                       /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding accumulationImageLB = {
      BINDING_ACCUMULATIONIMAGE             /*binding*/,
      vk::DescriptorType::eStorageImage     /*descriptorType*/,
      1                                     /*descriptorCount*/,
      vk::ShaderStageFlagBits::eRaygenKHR   /*stageFlags*/,
      nullptr                               /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding outputImageLB = {
      BINDING_OUTPUTIMAGE                   /*binding*/,
      vk::DescriptorType::eStorageImage     /*descriptorType*/,
      1                                     /*descriptorCount*/,
      vk::ShaderStageFlagBits::eRaygenKHR   /*stageFlags*/,
      nullptr                               /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding uniformBufferLB = {
      BINDING_UNIFORMBUFFER                                                                                              /*binding*/,
      vk::DescriptorType::eUniformBuffer                                                                                 /*descriptorType*/,
      1                                                                                                                  /*descriptorCount*/,
      vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR  /*stageFlags*/,
      nullptr                                                                                                            /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding vertexBufferLB = {
      BINDING_VERTEXBUFFER                      /*binding*/,
      vk::DescriptorType::eStorageBuffer        /*descriptorType*/,
      1                                         /*descriptorCount*/,
      {vk::ShaderStageFlagBits::eClosestHitKHR} /*stageFlags*/,
      nullptr                                   /*pImmutableSamplers*/
   };
   
   vk::DescriptorSetLayoutBinding indexBufferLB = {
      BINDING_INDEXBUFFER                       /*binding*/,
      vk::DescriptorType::eStorageBuffer        /*descriptorType*/,
      1                                         /*descriptorCount*/,
      {vk::ShaderStageFlagBits::eClosestHitKHR} /*stageFlags*/,
      nullptr                                   /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding materialBufferLB = {
      BINDING_MATERIALBUFFER                    /*binding*/,
      vk::DescriptorType::eStorageBuffer        /*descriptorType*/,
      1                                         /*descriptorCount*/,
      {vk::ShaderStageFlagBits::eClosestHitKHR} /*stageFlags*/,
      nullptr                                   /*pImmutableSamplers*/
   };

   std::array<vk::DescriptorSetLayoutBinding, BINDING_NUMBINDINGS> layoutBindings = {
      accelerationStructureLB,
      accumulationImageLB,
      outputImageLB,
      uniformBufferLB,
      vertexBufferLB,
      indexBufferLB,
      materialBufferLB
   };

   m_DescriptorSetLayout = m_Device.createDescriptorSetLayout({
      {}                                           /*flags*/,
      static_cast<uint32_t>(layoutBindings.size()) /*bindingCount*/,
      layoutBindings.data()                        /*pBindings*/
   });
}


void RayTraceSpheres::DestroyDescriptorSetLayout() {
   if (m_Device && m_DescriptorSetLayout) {
      m_Device.destroy(m_DescriptorSetLayout);
      m_DescriptorSetLayout = nullptr;
   }
}


void RayTraceSpheres::CreatePipelineLayout() {
   // Create the pipeline layout that is used to generate the rendering pipelines that are based on the descriptor set layout
   // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused

   vk::PushConstantRange pushConstantRange = {
      vk::ShaderStageFlagBits::eRaygenKHR           /*stageFlags*/,
      0                                             /*offset*/,
      static_cast<uint32_t>(sizeof(Constants))      /*size*/
   };

   m_PipelineLayout = m_Device.createPipelineLayout({
      {}                       /*flags*/,
      1                        /*setLayoutCount*/,
      &m_DescriptorSetLayout   /*pSetLayouts*/,
      1                        /*pushConstantRangeCount*/,
      &pushConstantRange       /*pPushConstantRanges*/
   });
}


void RayTraceSpheres::DestroyPipelineLayout() {
   if (m_Device && m_PipelineLayout) {
      m_Device.destroy(m_PipelineLayout);
      m_PipelineLayout = nullptr;
   }
}

void RayTraceSpheres::CreatePipeline() {
   // Create the graphics pipeline used in this example
   // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
   // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
   // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

   const uint32_t shaderIndexRaygen = 0;
   const uint32_t shaderIndexMiss = 1;
   const uint32_t shaderIndexClosestHit = 2;
   const uint32_t maxRecursion = 4;

   vk::SpecializationMapEntry specializationMapEntry = {
      0 /*constantID*/,
      0 /*offset*/,
      sizeof(uint32_t) /*size*/
   };

   vk::SpecializationInfo specializationInfo = {
      1                         /*mapEntryCount*/,
      &specializationMapEntry   /*pMapEntries*/,
      sizeof(maxRecursion)      /*dataSize*/,
      &maxRecursion             /*pData*/
   };

   // Shaders
   auto rayGenCode = Vulkan::ReadFile((m_bindir / "Assets" / "Shaders" / "RayTrace.rgen.spv").string());
   auto missCode = Vulkan::ReadFile((m_bindir / "Assets" / "Shaders" / "RayTrace.rmiss.spv").string());
   auto closestHitCode = Vulkan::ReadFile((m_bindir / "Assets" / "Shaders" / "RayTrace.rchit.spv").string());


   std::array<vk::PipelineShaderStageCreateInfo, 3> shaderStages = {
      vk::PipelineShaderStageCreateInfo {
         {}                                  /*flags*/,
         vk::ShaderStageFlagBits::eRaygenKHR  /*stage*/,
         CreateShaderModule(rayGenCode)      /*module*/,
         "main"                              /*name*/,
         &specializationInfo                 /*pSpecializationInfo*/
      },
      vk::PipelineShaderStageCreateInfo {
         {}                                  /*flags*/,
         vk::ShaderStageFlagBits::eMissKHR    /*stage*/,
         CreateShaderModule(missCode)        /*module*/,
         "main"                              /*name*/,
         nullptr                             /*pSpecializationInfo*/
      },
      vk::PipelineShaderStageCreateInfo {
         {}                                        /*flags*/,
         vk::ShaderStageFlagBits::eClosestHitKHR   /*stage*/,
         CreateShaderModule(closestHitCode)        /*module*/,
         "main"                                    /*name*/,
         nullptr                                   /*pSpecializationInfo*/
      }
   };

   std::array<vk::RayTracingShaderGroupCreateInfoKHR, 3> groups = {
      vk::RayTracingShaderGroupCreateInfoKHR {
         vk::RayTracingShaderGroupTypeKHR::eGeneral /*type*/,
         0                                         /*generalShader*/,
         VK_SHADER_UNUSED_KHR                      /*closestHitShader*/,
         VK_SHADER_UNUSED_KHR                      /*anyHitShader*/,
         VK_SHADER_UNUSED_KHR                      /*intersectionShader*/
      },
      vk::RayTracingShaderGroupCreateInfoKHR {
         vk::RayTracingShaderGroupTypeKHR::eGeneral /*type*/,
         1                                         /*generalShader*/,
         VK_SHADER_UNUSED_KHR                      /*closestHitShader*/,
         VK_SHADER_UNUSED_KHR                      /*anyHitShader*/,
         VK_SHADER_UNUSED_KHR                      /*intersectionShader*/
      },
      vk::RayTracingShaderGroupCreateInfoKHR {
         vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup /*type*/,
         VK_SHADER_UNUSED_KHR                      /*generalShader*/,
         2                                         /*closestHitShader*/,
         VK_SHADER_UNUSED_KHR                      /*anyHitShader*/,
         VK_SHADER_UNUSED_KHR                      /*intersectionShader*/
      }
   };

   vk::RayTracingPipelineCreateInfoKHR pipelineCI = {
      {}                                         /*flags*/,
      static_cast<uint32_t>(shaderStages.size()) /*stageCount*/,
      shaderStages.data()                        /*pStages*/,
      static_cast<uint32_t>(groups.size())       /*groupCount*/,
      groups.data()                              /*pGroups*/,
      1                                          /*maxPipelineRayRecursionDepth*/,
      nullptr                                    /*pLibraryInfo*/,
      nullptr                                    /*pLibraryInterface*/,
      nullptr                                    /*pDynamicState*/,
      m_PipelineLayout                           /*layout*/,
      nullptr                                    /*basePipelineHandle*/,
      0                                          /*basePipelineIndex*/
   };

   // .value works around issue with implicit cast of ResultValue<T> (refer https://github.com/KhronosGroup/Vulkan-Hpp/issues/680)
   m_Pipeline = m_Device.createRayTracingPipelineKHR(nullptr, m_PipelineCache, pipelineCI).value;

   // Shader modules are no longer needed once the graphics pipeline has been createdvk
   for (auto& shaderStage : shaderStages) {
      DestroyShaderModule(shaderStage.module);
   }
}


void RayTraceSpheres::DestroyPipeline() {
   if (m_Device && m_Pipeline) {
      m_Device.destroy(m_Pipeline);
   }
}


void RayTraceSpheres::CreateShaderBindingTable() {
   // Create buffer for the shader binding table
   // Note that regardless of the shaderGroupHandleSize, the entries in the shader binding table must be aligned on multiples of m_RayTracingProperties.shaderGroupBaseAlignment
   const uint32_t handleSize = m_RayTracingProperties.shaderGroupHandleSize;
   const uint32_t handleSizeAligned = Vulkan::AlignedSize(m_RayTracingProperties.shaderGroupHandleSize, m_RayTracingProperties.shaderGroupBaseAlignment);
   const vk::DeviceSize tableSize = static_cast<vk::DeviceSize>(handleSizeAligned) * 3;
   const vk::DeviceSize handlesSize = static_cast<vk::DeviceSize>(handleSize) * 3;

   std::vector<uint8_t> shaderHandleStorage = m_Device.getRayTracingShaderGroupHandlesKHR<uint8_t>(m_Pipeline, 0, 3, handlesSize);

   std::vector<uint8_t> shaderBindingTable;
   shaderBindingTable.resize(tableSize);

   for (uint32_t i = 0; i < 3; ++i) {
      std::memcpy(shaderBindingTable.data() + (i * static_cast<size_t>(handleSizeAligned)), shaderHandleStorage.data() + (i * static_cast<size_t>(handleSize)), static_cast<size_t>(handleSize));
   }

   m_ShaderBindingTable = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, tableSize, vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, vk::MemoryPropertyFlagBits::eHostVisible);
   m_ShaderBindingTable->CopyFromHost(0, tableSize, shaderBindingTable.data());
}


void RayTraceSpheres::DestroyShaderBindingTable() {
   m_ShaderBindingTable.reset(nullptr);
}


void RayTraceSpheres::CreateDescriptorPool() {
   std::array<vk::DescriptorPoolSize, 4> typeCounts = {
      vk::DescriptorPoolSize {
         vk::DescriptorType::eAccelerationStructureKHR,
         static_cast<uint32_t>(m_SwapChainFrameBuffers.size())
      },
      vk::DescriptorPoolSize {
         vk::DescriptorType::eStorageImage,
         static_cast<uint32_t>(2 * m_SwapChainFrameBuffers.size())
      },
      vk::DescriptorPoolSize {
         vk::DescriptorType::eUniformBuffer,
         static_cast<uint32_t>(m_SwapChainFrameBuffers.size())
      },
      vk::DescriptorPoolSize {
         vk::DescriptorType::eStorageBuffer,
         static_cast<uint32_t>(3 * m_SwapChainFrameBuffers.size())
      }
   };

   vk::DescriptorPoolCreateInfo descriptorPoolCI = {
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet       /*flags*/,
      static_cast<uint32_t>(3 * m_SwapChainFrameBuffers.size())  /*maxSets*/,
      static_cast<uint32_t>(typeCounts.size())                   /*poolSizeCount*/,
      typeCounts.data()                                          /*pPoolSizes*/
   };
   m_DescriptorPool = m_Device.createDescriptorPool(descriptorPoolCI);
}


void RayTraceSpheres::DestroyDescriptorPool() {
   if (m_Device && m_DescriptorPool) {
      m_Device.destroy(m_DescriptorPool);
   }
}


void RayTraceSpheres::CreateDescriptorSets() {

   std::vector layouts(m_SwapChainFrameBuffers.size(), m_DescriptorSetLayout);
   vk::DescriptorSetAllocateInfo allocInfo = {
      m_DescriptorPool,
      static_cast<uint32_t>(layouts.size()),
      layouts.data()
   };
   m_DescriptorSets = m_Device.allocateDescriptorSets(allocInfo);

   // Update the descriptor set determining the shader binding points
   // For every binding point used in a shader there needs to be one
   // descriptor set matching that binding point

   for (uint32_t i = 0; i < m_SwapChainFrameBuffers.size(); ++i) {
      vk::WriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = {
         1                               /*accelerationStructureCount*/,
         &m_TLAS.m_AccelerationStructure /*pAccelerationStructures*/
      };

      vk::WriteDescriptorSet accelerationStructureWrite = {
         m_DescriptorSets[i]                           /*dstSet*/,
         BINDING_TLAS                                  /*dstBinding*/,
         0                                             /*dstArrayElement*/,
         1                                             /*descriptorCount*/,
         vk::DescriptorType::eAccelerationStructureKHR /*descriptorType*/,
         nullptr                                       /*pImageInfo*/,
         nullptr                                       /*pBufferInfo*/,
         nullptr                                       /*pTexelBufferView*/
      };
      accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;

      vk::DescriptorImageInfo accumulationImageDescriptor = {
         nullptr                             /*sampler*/,
         m_AccumumlationImage->m_ImageView   /*imageView*/,
         vk::ImageLayout::eGeneral           /*imageLayout*/
      };
      vk::WriteDescriptorSet accumulationImageWrite = {
         m_DescriptorSets[i]                          /*dstSet*/,
         BINDING_ACCUMULATIONIMAGE                    /*dstBinding*/,
         0                                            /*dstArrayElement*/,
         1                                            /*descriptorCount*/,
         vk::DescriptorType::eStorageImage            /*descriptorType*/,
         &accumulationImageDescriptor                 /*pImageInfo*/,
         nullptr                                      /*pBufferInfo*/,
         nullptr                                      /*pTexelBufferView*/
      };

      vk::DescriptorImageInfo outputImageDescriptor = {
         nullptr                       /*sampler*/,
         m_OutputImage->m_ImageView   /*imageView*/,
         vk::ImageLayout::eGeneral     /*imageLayout*/
      };
      vk::WriteDescriptorSet outputImageWrite = {
         m_DescriptorSets[i]                          /*dstSet*/,
         BINDING_OUTPUTIMAGE                          /*dstBinding*/,
         0                                            /*dstArrayElement*/,
         1                                            /*descriptorCount*/,
         vk::DescriptorType::eStorageImage            /*descriptorType*/,
         &outputImageDescriptor                       /*pImageInfo*/,
         nullptr                                      /*pBufferInfo*/,
         nullptr                                      /*pTexelBufferView*/
      };

      vk::WriteDescriptorSet uniformBufferWrite = {
         m_DescriptorSets[i]                          /*dstSet*/,
         BINDING_UNIFORMBUFFER                        /*dstBinding*/,
         0                                            /*dstArrayElement*/,
         1                                            /*descriptorCount*/,
         vk::DescriptorType::eUniformBuffer           /*descriptorType*/,
         nullptr                                      /*pImageInfo*/,
         &m_UniformBuffers[i].m_Descriptor            /*pBufferInfo*/,
         nullptr                                      /*pTexelBufferView*/
      };

      vk::DescriptorBufferInfo vertexBufferDescriptor = {
         m_VertexBuffer->m_Buffer /*buffer*/,
         0                        /*offset*/,
         VK_WHOLE_SIZE            /*range*/
      };
      vk::WriteDescriptorSet vertexBufferWrite = {
         m_DescriptorSets[i]                          /*dstSet*/,
         BINDING_VERTEXBUFFER                         /*dstBinding*/,
         0                                            /*dstArrayElement*/,
         1                                            /*descriptorCount*/,
         vk::DescriptorType::eStorageBuffer           /*descriptorType*/,
         nullptr                                      /*pImageInfo*/,
         &vertexBufferDescriptor                      /*pBufferInfo*/,
         nullptr                                      /*pTexelBufferView*/
      };

      vk::DescriptorBufferInfo indexBufferDescriptor = {
         m_IndexBuffer->m_Buffer  /*buffer*/,
         0                        /*offset*/,
         VK_WHOLE_SIZE            /*range*/
      };
      vk::WriteDescriptorSet indexBufferWrite = {
         m_DescriptorSets[i]                          /*dstSet*/,
         BINDING_INDEXBUFFER                          /*dstBinding*/,
         0                                            /*dstArrayElement*/,
         1                                            /*descriptorCount*/,
         vk::DescriptorType::eStorageBuffer           /*descriptorType*/,
         nullptr                                      /*pImageInfo*/,
         &indexBufferDescriptor                       /*pBufferInfo*/,
         nullptr                                      /*pTexelBufferView*/
      };

      vk::DescriptorBufferInfo materialBufferDescriptor = {
         m_MaterialBuffer->m_Buffer  /*buffer*/,
         0                           /*offset*/,
         VK_WHOLE_SIZE               /*range*/
      };
      vk::WriteDescriptorSet materialBufferWrite = {
         m_DescriptorSets[i]                          /*dstSet*/,
         BINDING_MATERIALBUFFER                       /*dstBinding*/,
         0                                            /*dstArrayElement*/,
         1                                            /*descriptorCount*/,
         vk::DescriptorType::eStorageBuffer           /*descriptorType*/,
         nullptr                                      /*pImageInfo*/,
         &materialBufferDescriptor                    /*pBufferInfo*/,
         nullptr                                      /*pTexelBufferView*/
      };

      std::array<vk::WriteDescriptorSet, BINDING_NUMBINDINGS> writeDescriptorSets = {
         accelerationStructureWrite,
         accumulationImageWrite,
         outputImageWrite,
         uniformBufferWrite,
         vertexBufferWrite,
         indexBufferWrite,
         materialBufferWrite
      };

      m_Device.updateDescriptorSets(writeDescriptorSets, nullptr);
   }
}


void RayTraceSpheres::DestroyDescriptorSets() {
   if (m_Device) {
      m_Device.freeDescriptorSets(m_DescriptorPool, m_DescriptorSets);
      m_DescriptorSets.clear();
   }
}


void RayTraceSpheres::RecordCommandBuffers() {
   // Record the command buffers that are submitted to the graphics queue at each render.
   // We record one commend buffer per frame buffer (this allows us to pre-record the command
   // buffers, as we can bind the command buffer to its frame buffer.
   // (as opposed to having just one command buffer that gets built and bound to appropriate frame buffer
   // at render time)
   vk::CommandBufferBeginInfo commandBufferBI = {
      {}      /*flags*/,
      nullptr /*pInheritanceInfo*/
   };

   vk::ImageSubresourceRange subresourceRange = {
      vk::ImageAspectFlagBits::eColor   /*aspectMask*/,
      0                                 /*baseMipLevel*/,
      1                                 /*levelCount*/,
      0                                 /*baseArrayLayer*/,
      1                                 /*layerCount*/
   };

   const uint32_t handleSizeAligned = Vulkan::AlignedSize(m_RayTracingProperties.shaderGroupHandleSize, m_RayTracingProperties.shaderGroupBaseAlignment);

   vk::DeviceAddress deviceAddress = m_ShaderBindingTable->GetBufferDeviceAddress();
   const vk::StridedDeviceAddressRegionKHR raygenShaderBindingTable = {
      deviceAddress + (handleSizeAligned * 0),
      handleSizeAligned         /*stride*/,
      handleSizeAligned * 1     /*size*/
   };

   const vk::StridedDeviceAddressRegionKHR missShaderBindingTable = {
      deviceAddress + (handleSizeAligned * 1),
      handleSizeAligned         /*stride*/,
      handleSizeAligned * 1     /*size*/
   };

   const vk::StridedDeviceAddressRegionKHR hitShaderBindingTable = {
      deviceAddress + (handleSizeAligned * 2),
      handleSizeAligned         /*stride*/,
      handleSizeAligned * 3     /*size*/
   };

   const vk::StridedDeviceAddressRegionKHR callableShaderBindingTable = {};

   for (uint32_t i = 0; i < m_CommandBuffers.size(); ++i) {
      vk::CommandBuffer& commandBuffer = m_CommandBuffers[i];
      commandBuffer.begin(commandBufferBI);
      commandBuffer.pushConstants<Constants>(m_PipelineLayout, vk::ShaderStageFlagBits::eRaygenKHR, 0, m_Constants);
      commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_Pipeline);
      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_PipelineLayout, 0, m_DescriptorSets[i], nullptr);  // (i)th command buffer is bound to the (i)th descriptor set

      commandBuffer.traceRaysKHR(
         raygenShaderBindingTable,
         missShaderBindingTable,
         hitShaderBindingTable,
         callableShaderBindingTable,
         m_Extent.width, m_Extent.height, 1
      );

      vk::ImageMemoryBarrier barrier = {
         {}                                    /*srcAccessMask*/,
         vk::AccessFlagBits::eTransferWrite    /*dstAccessMask*/,
         vk::ImageLayout::eUndefined           /*oldLayout*/,
         vk::ImageLayout::eTransferDstOptimal  /*newLayout*/,
         VK_QUEUE_FAMILY_IGNORED               /*srcQueueFamilyIndex*/,
         VK_QUEUE_FAMILY_IGNORED               /*dstQueueFamilyIndex*/,
         m_SwapChainImages[i].m_Image          /*image*/,
         subresourceRange
      };
      commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, barrier);

      barrier = {
         {}                                    /*srcAccessMask*/,
         vk::AccessFlagBits::eTransferRead     /*dstAccessMask*/,
         vk::ImageLayout::eGeneral             /*oldLayout*/,
         vk::ImageLayout::eTransferSrcOptimal  /*newLayout*/,
         VK_QUEUE_FAMILY_IGNORED               /*srcQueueFamilyIndex*/,
         VK_QUEUE_FAMILY_IGNORED               /*dstQueueFamilyIndex*/,
         m_OutputImage->m_Image               /*image*/,
         subresourceRange
      };
      commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, barrier);

      vk::ImageCopy copyRegion = {
         {vk::ImageAspectFlagBits::eColor, 0, 0, 1 } /*srcSubresource*/,
         {0, 0, 0}                                   /*srcOffset*/,
         {vk::ImageAspectFlagBits::eColor, 0, 0, 1 } /*dstSubresource*/,
         {0, 0, 0}                                   /*dstOffset*/,
         {m_Extent.width, m_Extent.height, 1}        /*extent*/
      };
      commandBuffer.copyImage(m_OutputImage->m_Image, vk::ImageLayout::eTransferSrcOptimal, m_SwapChainImages[i].m_Image, vk::ImageLayout::eTransferDstOptimal, copyRegion);

      barrier = {
         vk::AccessFlagBits::eTransferWrite    /*srcAccessMask*/,
         {}                                    /*dstAccessMask*/,
         vk::ImageLayout::eTransferDstOptimal  /*oldLayout*/,
         vk::ImageLayout::ePresentSrcKHR       /*newLayout*/,
         VK_QUEUE_FAMILY_IGNORED               /*srcQueueFamilyIndex*/,
         VK_QUEUE_FAMILY_IGNORED               /*dstQueueFamilyIndex*/,
         m_SwapChainImages[i].m_Image          /*image*/,
         subresourceRange
      };
      commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, barrier);

      barrier = {
         vk::AccessFlagBits::eTransferRead     /*srcAccessMask*/,
         {}                                    /*dstAccessMask*/,
         vk::ImageLayout::eTransferSrcOptimal  /*oldLayout*/,
         vk::ImageLayout::eGeneral             /*newLayout*/,
         VK_QUEUE_FAMILY_IGNORED               /*srcQueueFamilyIndex*/,
         VK_QUEUE_FAMILY_IGNORED               /*dstQueueFamilyIndex*/,
         m_OutputImage->m_Image               /*image*/,
         subresourceRange
      };
      commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, barrier);

      commandBuffer.end();
   }
}


void RayTraceSpheres::Update(double deltaTime) {
   __super::Update(deltaTime);
   glm::mat4 projection = glm::perspective(m_FoVRadians, static_cast<float>(m_Extent.width) / static_cast<float>(m_Extent.height), 0.01f, 100.0f);
   projection[1][1] *= -1;
   glm::mat4 modelView = glm::lookAt(m_Eye, m_Eye + m_Direction, m_Up);

   m_UniformBufferObject.projInverse = glm::inverse(projection);
   m_UniformBufferObject.viewInverse = glm::inverse(modelView);

   if (
      (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_R) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_F) == GLFW_PRESS) ||
      m_LeftMouseDown
   ) {
      m_UniformBufferObject.accumulatedFrameCount = 0;
   }
   ++m_UniformBufferObject.accumulatedFrameCount;
}


void RayTraceSpheres::RenderFrame() {
   BeginFrame();
   m_UniformBuffers[m_CurrentImage].CopyFromHost(0, sizeof(UniformBufferObject), &m_UniformBufferObject);
   EndFrame();
}


void RayTraceSpheres::OnWindowResized() {
   __super::OnWindowResized();
   DestroyDescriptorSets();
   CreateStorageImages();
   CreateDescriptorSets();
   RecordCommandBuffers();
   m_UniformBufferObject.accumulatedFrameCount = 0;
}
