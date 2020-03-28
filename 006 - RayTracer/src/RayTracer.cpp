#include "RayTracer.h"

#include "Bindings.glsl"

using uint = uint32_t;
#include "Constants.glsl"

#include "GeometryInstance.h"
#include "Sphere.h"

using mat4 = glm::mat4;
using uint = uint32_t;
#include "UniformBufferObject.glsl"

#include "Utility.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <random>

#define M_PI 3.14159265358979323846f

std::unique_ptr<Vulkan::Application> CreateApplication(int argc, const char* argv[]) {
   return std::make_unique<RayTracer>(argc, argv);
}


inline float RandomFloat() {
   static std::uniform_real_distribution<float> distribution(0.0, 1.0);
   static std::mt19937 generator;
   static std::function<float()> rand_generator = std::bind(distribution, generator);
   return rand_generator();
}


RayTracer::RayTracer(int argc, const char* argv[])
: Vulkan::Application {
   { "Ray Tracer", VK_MAKE_VERSION(1,0,0) },
#ifdef _DEBUG
   /*enableValidation=*/true
#else
   /*enableValidation=*/false
#endif
}
, m_bindir { argv[0] }
{
   m_bindir.remove_filename();
   Init();
}


RayTracer::~RayTracer() {
   DestroyDescriptorSets();
   DestroyDescriptorPool();
   DestroyShaderBindingTable();
   DestroyPipeline();
   DestroyPipelineLayout();
   DestroyDescriptorSetLayout();
   DestroyUniformBuffers();
   DestroyStorageImages();
   DestroyAccelerationStructures();
   DestroyIndexBuffer();
   DestroyVertexBuffer();
}


std::vector<const char*> RayTracer::GetRequiredInstanceExtensions() {
   return {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
}


std::vector<const char*> RayTracer::GetRequiredDeviceExtensions() {
   return {VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, VK_NV_RAY_TRACING_EXTENSION_NAME, VK_KHR_MAINTENANCE3_EXTENSION_NAME};
}


void RayTracer::Init() {
   Vulkan::Application::Init();

   m_Eye = {8.0f, 2.0f, 2.0f};
   m_Direction = {-8.0f, -2.0f, -2.0f};
   m_Up = {0.0f, 1.0f, 0.0f};

   m_Direction = glm::normalize(m_Direction);

   auto properties = m_PhysicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPropertiesNV>();
   m_RayTracingProperties = properties.get<vk::PhysicalDeviceRayTracingPropertiesNV>();

   // Check requested push constant size against hardware limit
   // Specs require 128 bytes, so if the device complies our push constant buffer should always fit into memory
   if (properties.get<vk::PhysicalDeviceProperties2>().properties.limits.maxPushConstantsSize < sizeof(Constants)) {
      throw std::runtime_error("(Push)Constants too large");
   }

   CreateScene();
   CreateVertexBuffer();
   CreateIndexBuffer();
   CreateAccelerationStructures();
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


void RayTracer::CreateScene() {

   uint32_t sphereModel = m_Scene.AddModel({(m_bindir / "Assets" / "Models" / "sphere.obj").string().c_str()});
   uint32_t cubeModel = m_Scene.AddModel({(m_bindir / "Assets" / "Models" / "cube.obj").string().c_str()});

   // large background sphere...
   Sphere background = {
      sphereModel,
      {0.0f, -1000.0f, 0.0f},
      1000.0f,
      Lambertian({0.5, 0.5, 0.5})
   };

   m_Scene.AddInstance(background);

   // small random spheres
   for (int a = -11; a < 11; a++) {
      for (int b = -11; b < 11; b++) {
         float chooseMaterial = RandomFloat();
         vec3 centre(a + 0.9f * RandomFloat(), 0.2f, b + 0.9f * RandomFloat());
         if (
            (glm::length(centre - vec3(-4.0f, 0.2f, 0.0f)) > 0.9f) &&
            (glm::length(centre - vec3(0.0f, 0.2f, 0.0f)) > 0.9f) &&
            (glm::length(centre - vec3(4.0f, 0.2f, 0.0f)) > 0.9f)
         ) {
            Material material;
            if (chooseMaterial < 0.8) {
               // diffuse
               material = Lambertian({RandomFloat() * RandomFloat(),RandomFloat() * RandomFloat(), RandomFloat() * RandomFloat()});
            } else if (chooseMaterial < 0.95) {
               // metal
               material = Metallic({0.5 * (1 + RandomFloat()), 0.5 * (1 + RandomFloat()), 0.5 * (1 + RandomFloat())}, 0.5f * RandomFloat());
            } else {
               material = Dielectric(1.5f);
            }
            m_Scene.AddInstance(Sphere(sphereModel, centre, 0.2f, material));
         }
      }
   }

   // the three main spheres...
   m_Scene.AddInstance(Sphere(sphereModel, {0.0, 1.0, 0.0}, 1.0f, Dielectric(1.5f)));
   m_Scene.AddInstance(Sphere(cubeModel, {-4.0, 1.0, 0.0}, 1.0f, Lambertian({0.4, 0.2, 0.1})));
   m_Scene.AddInstance(Sphere(sphereModel, {4.0, 1.0, 0.0}, 1.0f, Metallic({0.7, 0.6, 0.5}, 0.0)));
}


void RayTracer::CreateVertexBuffer() {
   std::vector<Vertex> vertices;

   // for each model in scene, pack its vertices into vertex buffer
   for (const auto& model : m_Scene.Models()) {
      vertices.insert(vertices.end(), model.Vertices().begin(), model.Vertices().end());
   }

   vk::DeviceSize size = vertices.size() * sizeof(Vertex);
   Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
   stagingBuffer.CopyFromHost(0, size, vertices.data());

   m_VertexBuffer = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
   CopyBuffer(stagingBuffer.m_Buffer, m_VertexBuffer->m_Buffer, 0, 0, size);
}


void RayTracer::DestroyVertexBuffer() {
   m_VertexBuffer.reset(nullptr);
}


void RayTracer::CreateIndexBuffer() {
   std::vector<uint32_t> indices;

   // for each model in scene, pack its indices into index buffer
   for(size_t i = 0, numModels = m_Scene.Models().size(); i < numModels; ++i) {
      const auto& model = m_Scene.Models().at(i);
      const auto& indexOffset = m_Scene.ModelIndexOffsets().at(i);
      for (const auto& index : model.Indices()) {
         indices.push_back(indexOffset + index);
      }
   }

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

   m_IndexBuffer = std::make_unique<Vulkan::IndexBuffer>(m_Device, m_PhysicalDevice, size, count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
   CopyBuffer(stagingBuffer.m_Buffer, m_IndexBuffer->m_Buffer, 0, 0, size);
}



void RayTracer::CreateAccelerationStructures() {
   for (size_t i = 0, numModels = m_Scene.Models().size(); i < numModels; ++i) {
      vk::GeometryNV geometry = {
         vk::GeometryTypeNV::eTriangles                               /*geometryType*/,
         vk::GeometryDataNV {
            {
               m_VertexBuffer->m_Buffer                                  /*vertexData*/,
               m_Scene.ModelVertexOffsets().at(i)                        /*vertexOffset*/,
               static_cast<uint32_t>(m_Scene.Models().at(i).Vertices().size())                    /*vertexCount*/,
               static_cast<vk::DeviceSize>(sizeof(Vertex))               /*vertexStride*/,
               vk::Format::eR32G32B32Sfloat                              /*vertexFormat*/,
               m_IndexBuffer->m_Buffer                                   /*indexData*/,
               m_Scene.ModelIndexOffsets().at(i)                         /*indexOffset*/,
               static_cast<uint32_t>(m_Scene.Models().at(i).Indices().size())                    /*indexCount*/,
               vk::IndexType::eUint32                                    /*indexType*/,
               nullptr                                                   /*transformData*/,
               0                                                         /*transformOffset*/
            }                                                         /*triangles*/,
            {
               nullptr                                                   /*aabbData*/,
               0                                                         /*numAABBs*/,
               0                                                         /*stride*/,
               0                                                         /*offset*/
            }                                                         /*aabbs*/
         }                                                            /*geometry*/,
         vk::GeometryFlagBitsNV::eOpaque                              /*flags*/
      };

      AddBottomLevelAccelerationStructure(geometry);
   }

   std::vector<Vulkan::GeometryInstance> geometryInstances;
   std::vector<Material> materials;
   uint32_t i = 0;

   geometryInstances.reserve(m_Scene.Instances().size());
   materials.reserve(m_Scene.Instances().size());
   for (const auto& instance : m_Scene.Instances()) {
      geometryInstances.emplace_back(
         instance.GetTransform(),
         i++                            /*instance index*/,
         0xff                           /*visibility mask*/,
         0                              /*hit group index*/,
         static_cast<uint32_t>(vk::GeometryInstanceFlagBitsNV::eTriangleCullDisable),
         m_BLAS.at(instance.GetModelIndex()).m_Handle
      );
      materials.emplace_back(instance.GetMaterial());
   };

   // Create Materials buffer
   {
      vk::DeviceSize size = materials.size() * sizeof(Material);

      Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
      stagingBuffer.CopyFromHost(0, size, materials.data());

      m_MaterialBuffer = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
      CopyBuffer(stagingBuffer.m_Buffer, m_MaterialBuffer->m_Buffer, 0, 0, size);
   }

   // Each geometry instance instantiates all of the geometries that are in the BLAS that the instance refers to.
   // If you want to instantiate geometries independently of each other, then they need to be in different BLASs
   // Apparently, a general rule is that the fewer BLASs the better. 
   CreateTopLevelAccelerationStructure(geometryInstances);
}


void RayTracer::DestroyAccelerationStructures() {
   DestroyTopLevelAccelerationStructure();
   DestroyBottomLevelAccelerationStructures();
   m_MaterialBuffer.reset(nullptr);
}


void RayTracer::DestroyIndexBuffer() {
   m_IndexBuffer.reset(nullptr);
}


void RayTracer::CreateStorageImages() {
   m_OutputImage = std::make_unique<Vulkan::Image>(m_Device, m_PhysicalDevice, m_Extent.width, m_Extent.height, 1, vk::SampleCountFlagBits::e1, m_Format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal);
   m_OutputImage->CreateImageView(m_Format, vk::ImageAspectFlagBits::eColor, 1);
   TransitionImageLayout(m_OutputImage->m_Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1);

   m_AccumumlationImage = std::make_unique<Vulkan::Image>(m_Device, m_PhysicalDevice, m_Extent.width, m_Extent.height, 1, vk::SampleCountFlagBits::e1, vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eStorage, vk::MemoryPropertyFlagBits::eDeviceLocal);
   m_AccumumlationImage->CreateImageView(vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor, 1);
   TransitionImageLayout(m_AccumumlationImage->m_Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1);
}


void RayTracer::DestroyStorageImages() {
   m_AccumumlationImage.reset(nullptr);
   m_OutputImage.reset(nullptr);
}


void RayTracer::CreateUniformBuffers() {
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


void RayTracer::DestroyUniformBuffers() {
   m_UniformBuffers.clear();
}


void RayTracer::CreateDescriptorSetLayout() {
   // Setup layout of descriptors used in this example
   // Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
   // So every shader binding should map to one descriptor set layout binding

   vk::DescriptorSetLayoutBinding accelerationStructureLB = {
      BINDING_TLAS                                                                 /*binding*/,
      vk::DescriptorType::eAccelerationStructureNV                                 /*descriptorType*/,
      1                                                                            /*descriptorCount*/,
      vk::ShaderStageFlagBits::eRaygenNV | vk::ShaderStageFlagBits::eClosestHitNV  /*stageFlags*/,
      nullptr                                                                      /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding accumulationImageLB = {
      BINDING_ACCUMULATIONIMAGE             /*binding*/,
      vk::DescriptorType::eStorageImage     /*descriptorType*/,
      1                                     /*descriptorCount*/,
      vk::ShaderStageFlagBits::eRaygenNV    /*stageFlags*/,
      nullptr                               /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding outputImageLB = {
      BINDING_OUTPUTIMAGE                   /*binding*/,
      vk::DescriptorType::eStorageImage     /*descriptorType*/,
      1                                     /*descriptorCount*/,
      vk::ShaderStageFlagBits::eRaygenNV    /*stageFlags*/,
      nullptr                               /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding uniformBufferLB = {
      BINDING_UNIFORMBUFFER                                                                                           /*binding*/,
      vk::DescriptorType::eUniformBuffer                                                                              /*descriptorType*/,
      1                                                                                                               /*descriptorCount*/,
      vk::ShaderStageFlagBits::eRaygenNV | vk::ShaderStageFlagBits::eClosestHitNV | vk::ShaderStageFlagBits::eMissNV  /*stageFlags*/,
      nullptr                                                                                                         /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding vertexBufferLB = {
      BINDING_VERTEXBUFFER                      /*binding*/,
      vk::DescriptorType::eStorageBuffer        /*descriptorType*/,
      1                                         /*descriptorCount*/,
      {vk::ShaderStageFlagBits::eClosestHitNV}  /*stageFlags*/,
      nullptr                                   /*pImmutableSamplers*/
   };
   
   vk::DescriptorSetLayoutBinding indexBufferLB = {
      BINDING_INDEXBUFFER                       /*binding*/,
      vk::DescriptorType::eStorageBuffer        /*descriptorType*/,
      1                                         /*descriptorCount*/,
      {vk::ShaderStageFlagBits::eClosestHitNV}  /*stageFlags*/,
      nullptr                                   /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding materialBufferLB = {
      BINDING_MATERIALBUFFER                    /*binding*/,
      vk::DescriptorType::eStorageBuffer        /*descriptorType*/,
      1                                         /*descriptorCount*/,
      {vk::ShaderStageFlagBits::eClosestHitNV}  /*stageFlags*/,
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


void RayTracer::DestroyDescriptorSetLayout() {
   if (m_Device && m_DescriptorSetLayout) {
      m_Device.destroy(m_DescriptorSetLayout);
      m_DescriptorSetLayout = nullptr;
   }
}


void RayTracer::CreatePipelineLayout() {
   // Create the pipeline layout that is used to generate the rendering pipelines that are based on the descriptor set layout
   // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused

   vk::PushConstantRange pushConstantRange = {
      vk::ShaderStageFlagBits::eRaygenNV            /*stageFlags*/,
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


void RayTracer::DestroyPipelineLayout() {
   if (m_Device && m_PipelineLayout) {
      m_Device.destroy(m_PipelineLayout);
      m_PipelineLayout = nullptr;
   }
}

void RayTracer::CreatePipeline() {
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
         vk::ShaderStageFlagBits::eRaygenNV  /*stage*/,
         CreateShaderModule(rayGenCode)      /*module*/,
         "main"                              /*name*/,
         &specializationInfo                 /*pSpecializationInfo*/
      },
      vk::PipelineShaderStageCreateInfo {
         {}                                  /*flags*/,
         vk::ShaderStageFlagBits::eMissNV    /*stage*/,
         CreateShaderModule(missCode)        /*module*/,
         "main"                              /*name*/,
         nullptr                             /*pSpecializationInfo*/
      },
      vk::PipelineShaderStageCreateInfo {
         {}                                        /*flags*/,
         vk::ShaderStageFlagBits::eClosestHitNV    /*stage*/,
         CreateShaderModule(closestHitCode)        /*module*/,
         "main"                                    /*name*/,
         nullptr                                   /*pSpecializationInfo*/
      }
   };

   std::array<vk::RayTracingShaderGroupCreateInfoNV, 3> groups = {
      vk::RayTracingShaderGroupCreateInfoNV {
         vk::RayTracingShaderGroupTypeNV::eGeneral /*type*/,
         0                                         /*generalShader*/,
         VK_SHADER_UNUSED_NV                       /*closestHitShader*/,
         VK_SHADER_UNUSED_NV                       /*anyHitShader*/,
         VK_SHADER_UNUSED_NV                       /*intersectionShader*/
      },
      vk::RayTracingShaderGroupCreateInfoNV {
         vk::RayTracingShaderGroupTypeNV::eGeneral /*type*/,
         1                                         /*generalShader*/,
         VK_SHADER_UNUSED_NV                       /*closestHitShader*/,
         VK_SHADER_UNUSED_NV                       /*anyHitShader*/,
         VK_SHADER_UNUSED_NV                       /*intersectionShader*/
      },
      vk::RayTracingShaderGroupCreateInfoNV {
         vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup /*type*/,
         VK_SHADER_UNUSED_NV                       /*generalShader*/,
         2                                         /*closestHitShader*/,
         VK_SHADER_UNUSED_NV                       /*anyHitShader*/,
         VK_SHADER_UNUSED_NV                       /*intersectionShader*/
      }
   };

   vk::RayTracingPipelineCreateInfoNV pipelineCI = {
      {}                                         /*flags*/,
      static_cast<uint32_t>(shaderStages.size()) /*stageCount*/,
      shaderStages.data()                        /*pStages*/,
      static_cast<uint32_t>(groups.size())       /*groupCount*/,
      groups.data()                              /*pGroups*/,
      1                                          /*maxRecursionDepth*/,
      m_PipelineLayout                           /*layout*/,
      nullptr                                    /*basePipelineHandle*/,
      0                                          /*basePipelineIndex*/
   };

   m_Pipeline = m_Device.createRayTracingPipelineNV(m_PipelineCache, pipelineCI);

   // Shader modules are no longer needed once the graphics pipeline has been createdvk
   for (auto& shaderStage : shaderStages) {
      DestroyShaderModule(shaderStage.module);
   }
}


void RayTracer::DestroyPipeline() {
   if (m_Device && m_Pipeline) {
      m_Device.destroy(m_Pipeline);
   }
}


void RayTracer::CreateShaderBindingTable() {
   // Create buffer for the shader binding table
   const uint32_t sbtSize = m_RayTracingProperties.shaderGroupHandleSize * 3;

   m_ShaderBindingTable = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, sbtSize, vk::BufferUsageFlagBits::eRayTracingNV, vk::MemoryPropertyFlagBits::eHostVisible);

   std::vector<uint8_t> shaderHandleStorage;
   shaderHandleStorage.resize(sbtSize);
   m_Device.getRayTracingShaderGroupHandlesNV<uint8_t>(m_Pipeline, 0, 3, shaderHandleStorage);
   m_ShaderBindingTable->CopyFromHost(0, sbtSize, shaderHandleStorage.data());
}


void RayTracer::DestroyShaderBindingTable() {
   m_ShaderBindingTable.reset(nullptr);
}


void RayTracer::CreateDescriptorPool() {
   std::array<vk::DescriptorPoolSize, 4> typeCounts = {
      vk::DescriptorPoolSize {
         vk::DescriptorType::eAccelerationStructureNV,
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


void RayTracer::DestroyDescriptorPool() {
   if (m_Device && m_DescriptorPool) {
      m_Device.destroy(m_DescriptorPool);
   }
}


void RayTracer::CreateDescriptorSets() {

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
      vk::WriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo = {
         1                               /*accelerationStructureCount*/,
         &m_TLAS.m_AccelerationStructure /*pAccelerationStructures*/
      };

      vk::WriteDescriptorSet accelerationStructureWrite = {
         m_DescriptorSets[i]                          /*dstSet*/,
         BINDING_TLAS                                 /*dstBinding*/,
         0                                            /*dstArrayElement*/,
         1                                            /*descriptorCount*/,
         vk::DescriptorType::eAccelerationStructureNV /*descriptorType*/,
         nullptr                                      /*pImageInfo*/,
         nullptr                                      /*pBufferInfo*/,
         nullptr                                      /*pTexelBufferView*/
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


void RayTracer::DestroyDescriptorSets() {
   if (m_Device) {
      m_Device.freeDescriptorSets(m_DescriptorPool, m_DescriptorSets);
      m_DescriptorSets.clear();
   }
}


void RayTracer::RecordCommandBuffers() {
   // Record the command buffers that are submitted to the graphics queue at each render.
   // We record one commend buffer per frame buffer (this allows us to pre-record the command
   // buffers, as we can bind the command buffer to its frame buffer here
   // (as opposed to having just one command buffer that gets built and then bound to appropriate frame buffer
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

   // TODO: You probably don't actually want these things to be "constants"
   //       all of them are things that you might want to be able to change
   //       (without re-recording the entire command buffer)
   //       Could just shove them into the uniform buffer object instead.
   Constants constants = {
      8,                           // max ray bounces
      0.0,                         // lens aperture
      glm::length(m_Direction)     // lens focal length
   };

   for (uint32_t i = 0; i < m_CommandBuffers.size(); ++i) {
      vk::CommandBuffer& commandBuffer = m_CommandBuffers[i];
      commandBuffer.begin(commandBufferBI);
      commandBuffer.pushConstants<Constants>(m_PipelineLayout, vk::ShaderStageFlagBits::eRaygenNV, 0, constants);
      commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingNV, m_Pipeline);
      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV, m_PipelineLayout, 0, m_DescriptorSets[i], nullptr);  // (i)th command buffer is bound to the (i)th descriptor set

      vk::DeviceSize bindingOffsetRayGenShader = static_cast<vk::DeviceSize>(m_RayTracingProperties.shaderGroupHandleSize) * 0;
      vk::DeviceSize bindingOffsetMissShader = static_cast<vk::DeviceSize>(m_RayTracingProperties.shaderGroupHandleSize) * 1;
      vk::DeviceSize bindingOffsetHitShader = static_cast<vk::DeviceSize>(m_RayTracingProperties.shaderGroupHandleSize) * 2;
      vk::DeviceSize bindingStride = m_RayTracingProperties.shaderGroupHandleSize;

      commandBuffer.traceRaysNV(
         m_ShaderBindingTable->m_Buffer, bindingOffsetRayGenShader,
         m_ShaderBindingTable->m_Buffer, bindingOffsetMissShader, bindingStride,
         m_ShaderBindingTable->m_Buffer, bindingOffsetHitShader, bindingStride,
         nullptr, 0, 0,
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


void RayTracer::Update(double deltaTime) {
   __super::Update(deltaTime);

   if (
      (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_R) == GLFW_PRESS) ||
      (glfwGetKey(m_Window, GLFW_KEY_F) == GLFW_PRESS) ||
      m_LeftMouseDown
   ) {
      m_AccumulatedImageCount = 0;
   }
   ++m_AccumulatedImageCount;
}


void RayTracer::RenderFrame() {

   glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(m_Extent.width) / static_cast<float>(m_Extent.height), 0.01f, 100.0f);
   projection[1][1] *= -1;
   glm::mat4 modelView = glm::lookAt(m_Eye, m_Eye + m_Direction, m_Up);

   UniformBufferObject ubo = {
      glm::inverse(modelView),
      glm::inverse(projection),
      m_AccumulatedImageCount
   };

   // All the rendering instructions are in pre-recorded command buffer (which gets submitted to the GPU in EndFrame()).  All we have to do here is update the uniform buffer.
   BeginFrame();
   m_UniformBuffers[m_CurrentImage].CopyFromHost(0, sizeof(UniformBufferObject), &ubo);
   EndFrame();
}


void RayTracer::OnWindowResized() {
   __super::OnWindowResized();
   DestroyDescriptorSets();
   CreateStorageImages();
   CreateDescriptorSets();
   RecordCommandBuffers();
   m_AccumulatedImageCount = 0;
}
