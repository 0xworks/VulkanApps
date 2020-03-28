#include "RasterSpheres.h"

#include "Instance.h"
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
   return std::make_unique<RasterSpheres>(argc, argv);
}

RasterSpheres::RasterSpheres(int argc, const char* argv[])
: Vulkan::Application(
   {
      "Example 1",
      VK_MAKE_VERSION(1,0,0),
   },
#ifdef _DEBUG
   /*enableValidation=*/true
#else
   /*enableValidation=*/false
#endif
)
, m_bindir(argv[0])
{
   m_bindir.remove_filename();
   Init();
}


RasterSpheres::~RasterSpheres() {
   DestroyDescriptorSets();
   DestroyDescriptorPool();
   DestroyPipeline();
   DestroyPipelineLayout();
   DestroyDescriptorSetLayout();
   DestroyUniformBuffers();
   DestroyIndexBuffer();
   DestroyInstanceBuffer();
   DestroyVertexBuffer();
}



vk::PhysicalDeviceFeatures RasterSpheres::GetRequiredPhysicalDeviceFeatures(vk::PhysicalDeviceFeatures availableFeatures) {
   vk::PhysicalDeviceFeatures features;
   if (availableFeatures.samplerAnisotropy) {
      features.setSamplerAnisotropy(true);
   }
   return features;
}


void RasterSpheres::Init() {
   Vulkan::Application::Init();
   
   m_Eye = {8.0f, 2.0f, 2.0f};
   m_Direction = glm::normalize(glm::vec3 {-8.0f, -2.0f, -2.0f});
   m_Up = glm::normalize(glm::vec3 {0.0f, 1.0f, 0.0f});
   LoadModel();
   CreateVertexBuffer();
   CreateInstanceBuffer();
   CreateIndexBuffer();
   CreateUniformBuffers();
   CreateDescriptorSetLayout();
   CreatePipelineLayout();
   CreatePipeline();
   CreateDescriptorPool();
   CreateDescriptorSets();
   RecordCommandBuffers();
}


void RasterSpheres::LoadModel() {
   tinyobj::attrib_t attrib;
   std::vector<tinyobj::shape_t> shapes;
   std::vector<tinyobj::material_t> materials;
   std::string warn, err;

   if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "Assets/Models/sphere.obj")) {
      throw std::runtime_error(warn + err);
   }


   std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

   for (const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
         Vertex vertex = {
            {
               attrib.vertices[3 * index.vertex_index + 0],
               attrib.vertices[3 * index.vertex_index + 1],
               attrib.vertices[3 * index.vertex_index + 2]
            } /*Pos*/,
            {
               attrib.normals[3 * index.normal_index + 0],
               attrib.normals[3 * index.normal_index + 1],
               attrib.normals[3 * index.normal_index + 2],
            } /*Normal*/,
         };

         if (uniqueVertices.count(vertex) == 0) {
            uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
            m_Vertices.push_back(vertex);
         }
         m_Indices.push_back(uniqueVertices[vertex]);
      }
   }
}


void RasterSpheres::CreateVertexBuffer() {
   // A note on memory management in Vulkan in general:
   // This is a very complex topic and while it's fine for an example application to allocate small individual memory allocations
   // that is not what should be done a real-world application, where you should allocate large chunks of memory at once instead.

   vk::DeviceSize size = m_Vertices.size() * sizeof(Vertex);

   Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
   stagingBuffer.CopyFromHost(0, size, m_Vertices.data());

   m_VertexBuffer = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
   CopyBuffer(stagingBuffer.m_Buffer, m_VertexBuffer->m_Buffer, 0, 0, size);
}


void RasterSpheres::DestroyVertexBuffer() {
   m_VertexBuffer.reset(nullptr);
}


void RasterSpheres::CreateInstanceBuffer() {
   static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
   static std::mt19937 generator;
   static std::function<float()> random_float = std::bind(distribution, generator);

   std::vector<Instance> instances;

   int n = 500;
   instances.emplace_back(glm::vec3(0.0f, -1000.0f, 0.0f), 1000.0f, glm::vec3(0.5f, 1.0f, 0.5f)); // std::make_shared<Lambertian>(glm::vec3(0.5f, 0.5f, 0.5f))));
   int i = 1;
   for (int a = -11; a < 11; ++a) {
      for (int b = -11; b < 11; ++b) {
         float choose_mat = random_float();
         glm::vec3 centre(a + (0.9f * random_float()), 0.2f, b + (0.9f * random_float()));
         if (glm::length(centre - glm::vec3(4.0f, 0.2f, 0.0f)) > 0.9f) {
            instances.emplace_back(centre, 0.2f, glm::vec3(random_float(), random_float(), random_float()));
         }
      }
   }

   instances.emplace_back(glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f)); // , std::make_shared<Dielectric>(1.5f)));
   instances.emplace_back(glm::vec3(-4.0f, 1.0f, 0.0f), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f)); // , std::make_shared<Lambertian>(glm::vec3(0.4f, 0.2f, 0.1f))));
   instances.emplace_back(glm::vec3(4.0f, 1.0f, 0.0f), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));// , std::make_shared<Metal>(glm::vec3(0.7f, 0.6f, 0.5f), 0.0f)));

   m_InstanceCount = static_cast<uint32_t>(instances.size());

   vk::DeviceSize size = instances.size() * sizeof(Instance);
   Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
   stagingBuffer.CopyFromHost(0, size, instances.data());

   m_InstanceBuffer = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
   CopyBuffer(stagingBuffer.m_Buffer, m_InstanceBuffer->m_Buffer, 0, 0, size);
}


void RasterSpheres::DestroyInstanceBuffer() {
   m_VertexBuffer.reset(nullptr);
}


void RasterSpheres::CreateIndexBuffer() {
   uint32_t count = static_cast<uint32_t>(m_Indices.size());
   vk::DeviceSize size = count * sizeof(uint32_t);

   Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
   stagingBuffer.CopyFromHost(0, size, m_Indices.data());

   m_IndexBuffer = std::make_unique<Vulkan::IndexBuffer>(m_Device, m_PhysicalDevice, size, count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
   CopyBuffer(stagingBuffer.m_Buffer, m_IndexBuffer->m_Buffer, 0, 0, size);
}


void RasterSpheres::DestroyIndexBuffer() {
   m_IndexBuffer.reset(nullptr);
}


void RasterSpheres::CreateUniformBuffers() {
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


void RasterSpheres::DestroyUniformBuffers() {
   m_UniformBuffers.clear();
}


void RasterSpheres::CreateDescriptorSetLayout() {
   // Setup layout of descriptors used in this example
   // Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
   // So every shader binding should map to one descriptor set layout binding

   vk::DescriptorSetLayoutBinding uboLayoutBinding = {
      0                                   /*binding*/,
      vk::DescriptorType::eUniformBuffer  /*descriptorType*/,
      1                                   /*descriptorCount*/,
      {vk::ShaderStageFlagBits::eVertex}  /*stageFlags*/,
      nullptr                             /*pImmutableSamplers*/
   };

   vk::DescriptorSetLayoutBinding samplerLayoutBinding = {
      1                                          /*binding*/,
      vk::DescriptorType::eCombinedImageSampler  /*descriptorType*/,
      1                                          /*descriptorCount*/,
      {vk::ShaderStageFlagBits::eFragment}       /*stageFlags*/,
      nullptr                                    /*pImmutableSamplers*/
   };

   std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {uboLayoutBinding, samplerLayoutBinding};

   m_DescriptorSetLayout = m_Device.createDescriptorSetLayout({
      {}                                           /*flags*/,
      static_cast<uint32_t>(layoutBindings.size()) /*bindingCount*/,
      layoutBindings.data()                        /*pBindings*/
   });
}


void RasterSpheres::DestroyDescriptorSetLayout() {
   if (m_Device && m_DescriptorSetLayout) {
      m_Device.destroy(m_DescriptorSetLayout);
      m_DescriptorSetLayout = nullptr;
   }
}


void RasterSpheres::CreatePipelineLayout() {
   // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
   // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
   m_PipelineLayout = m_Device.createPipelineLayout({
      {}                       /*flags*/,
      1                        /*setLayoutCount*/,
      &m_DescriptorSetLayout   /*pSetLayouts*/,
      0                        /*pushConstantRangeCount*/,
      nullptr                  /*pPushConstantRanges*/
   });
}


void RasterSpheres::DestroyPipelineLayout() {
   if (m_Device && m_PipelineLayout) {
      m_Device.destroy(m_PipelineLayout);
      m_PipelineLayout = nullptr;
   }
}

void RasterSpheres::CreatePipeline() {
   // Create the graphics pipeline used in this example
   // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
   // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
   // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

   vk::GraphicsPipelineCreateInfo pipelineCI;
   pipelineCI.layout = m_PipelineLayout;
   pipelineCI.renderPass = m_RenderPass;

   // Input assembly state describes how primitives are assembled
   // This pipeline will assemble vertex data as a triangle lists (though we're only use one triangle)
   vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState = {
      {}                                   /*flags*/,
      vk::PrimitiveTopology::eTriangleList /*topology*/,
      false                                /*primitiveRestartEnable*/
   };
   pipelineCI.pInputAssemblyState = &inputAssemblyState;

   // Rasterization state
   vk::PipelineRasterizationStateCreateInfo rasterizationState = {
      {}                               /*flags*/,
      false                            /*depthClampEnable*/,
      false                            /*rasterizerDiscardEnable*/,
      vk::PolygonMode::eFill           /*polygonMode*/,
      {vk::CullModeFlagBits::eBack}    /*cullMode*/,
      vk::FrontFace::eCounterClockwise /*frontFace*/,
      false                            /*depthBiasEnable*/,
      0.0f                             /*depthBiasConstantFactor*/,
      0.0f                             /*depthBiasClamp*/,
      0.0f                             /*depthBiasSlopeFactor*/,
      1.0f                             /*lineWidth*/
   };
   pipelineCI.pRasterizationState = &rasterizationState;

   // Color blend state describes how blend factors are calculated (if used)
   // We need one blend attachment state per color attachment (even if blending is not used)
   vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {
      true                                     /*blendEnable*/,
      vk::BlendFactor::eSrcAlpha               /*srcColorBlendFactor*/,
      vk::BlendFactor::eOneMinusSrcAlpha       /*dstColorBlendFactor*/,
      vk::BlendOp::eAdd                        /*colorBlendOp*/,
      vk::BlendFactor::eOne                    /*srcAlphaBlendFactor*/,
      vk::BlendFactor::eZero                   /*dstAlphaBlendFactor*/,
      vk::BlendOp::eAdd                        /*alphaBlendOp*/,
      {vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA } /*colorWriteMask*/
   };

   vk::PipelineColorBlendStateCreateInfo colorBlendState = {
      {}                         /*flags*/,
      false                      /*logicOpEnable*/,
      vk::LogicOp::eCopy         /*logicOp*/,
      1                          /*attachmentCount*/,
      &colorBlendAttachmentState /*pAttachments*/,
      {{0.0f}}                   /*blendConstants*/
   };
   pipelineCI.pColorBlendState = &colorBlendState;

   // Viewport state sets the number of viewports and scissor used in this pipeline
   // Note: This is actually overridden by the dynamic states (see below)
   vk::Viewport viewport = {
      0.0f, 0.0f,
      static_cast<float>(m_Extent.width), static_cast<float>(m_Extent.height),
      0.0f, 1.0f
   };

   vk::Rect2D scissor = {
      {0, 0},
      m_Extent
   };

   vk::PipelineViewportStateCreateInfo viewportState = {
      {}          /*flags*/,
      1           /*viewportCount*/,
      &viewport   /*pViewports*/,
      1           /*scissorCount*/,
      &scissor    /*pScissors*/
   };
   pipelineCI.pViewportState = &viewportState;

   // Enable dynamic states
   // Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
   // To be able to change these we need do specify which dynamic states will be changed using this pipeline.
   // Their actual states are set later on in the command buffer.
   // For this example we will set the viewport and scissor using dynamic states
   std::array<vk::DynamicState, 2> dynamicStates = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
   };

   vk::PipelineDynamicStateCreateInfo dynamicState = {
      {}                                           /*flags*/,
      static_cast<uint32_t>(dynamicStates.size())  /*dynamicStateCount*/,
      dynamicStates.data()                         /*pDynamicStates*/
   };
   pipelineCI.pDynamicState = &dynamicState;

   // Depth and stencil state containing depth and stencil compare and test operations
   // We only use depth tests and want depth tests and writes to be enabled and compare with less
   vk::PipelineDepthStencilStateCreateInfo depthStencilState = {
      {}                    /*flags*/,
      true                  /*depthTestEnable*/,
      true                  /*depthWriteEnable*/,
      vk::CompareOp::eLess  /*depthCompareOp*/,
      false                 /*depthBoundsTestEnable*/,
      false                 /*stencilTestEnable*/,
      {
         vk::StencilOp::eKeep    /*failOp*/,
         vk::StencilOp::eKeep    /*passOp*/,
         vk::StencilOp::eKeep    /*depthFailOp*/,
         vk::CompareOp::eNever   /*compareOp*/,
         0                       /*compareMask*/,
         0                       /*writeMask*/,
         0                       /*reference*/
      }                     /*front*/,
      {
         vk::StencilOp::eKeep    /*failOp*/,
         vk::StencilOp::eKeep    /*passOp*/,
         vk::StencilOp::eKeep    /*depthFailOp*/,
         vk::CompareOp::eNever   /*compareOp*/,
         0                       /*compareMask*/,
         0                       /*writeMask*/,
         0                       /*reference*/
      }                     /*back*/,
      0.0f                  /*minDepthBounds*/,
      1.0f                  /*maxDepthBounds*/
   };
   pipelineCI.pDepthStencilState = &depthStencilState;

   // Multi sampling state
   // This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
   vk::PipelineMultisampleStateCreateInfo multisampleState = {
      {}                          /*flags*/,
      vk::SampleCountFlagBits::e1 /*rasterizationSamples*/,
      false                       /*sampleShadingEnable*/,
      1.0f                        /*minSampleShading*/,
      nullptr                     /*pSampleMask*/,
      false                       /*alphaToCoverageEnable*/,
      false                       /*alphaToOneEnable*/
   };
   pipelineCI.pMultisampleState = &multisampleState;

   // Vertex input descriptions 
   // Specifies the vertex input parameters for a pipeline
   auto bindingDescriptionVertex = Vertex::GetBindingDescription();
   auto bindingDescriptionInstance = Instance::GetBindingDescription();
   bindingDescriptionInstance.binding = 1;
   std::array<vk::VertexInputBindingDescription, 2> bindingDescriptions = {bindingDescriptionVertex, bindingDescriptionInstance};

   auto attributeDescriptionsVertex = Vertex::GetAttributeDescriptions();
   auto attributeDescriptionsInstance = Instance::GetAttributeDescriptions();

   for (auto& attributeDescription : attributeDescriptionsInstance) {
      attributeDescription.binding = bindingDescriptionInstance.binding;
      attributeDescription.location += static_cast<uint32_t>(attributeDescriptionsVertex.size());  // TODO: this isnt technically correct... it depends on how many locations the vertex attributes are actually taking up
   }
   std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
   attributeDescriptions.reserve(attributeDescriptionsVertex.size() + attributeDescriptionsInstance.size());
   attributeDescriptions.insert(attributeDescriptions.end(), std::make_move_iterator(attributeDescriptionsVertex.begin()), std::make_move_iterator(attributeDescriptionsVertex.end()));
   attributeDescriptions.insert(attributeDescriptions.end(), std::make_move_iterator(attributeDescriptionsInstance.begin()), std::make_move_iterator(attributeDescriptionsInstance.end()));

   // Vertex input state used for pipeline creation
   vk::PipelineVertexInputStateCreateInfo vertexInputState = {
      {}                                                    /*flags*/,
      static_cast<uint32_t>(bindingDescriptions.size())     /*vertexBindingDescriptionCount*/,
      bindingDescriptions.data()                            /*pVertexBindingDescriptions*/,
      static_cast<uint32_t>(attributeDescriptions.size())   /*vertexAttributeDescriptionCount*/,
      attributeDescriptions.data()                          /*pVertexAttributeDescriptions*/
   };
   pipelineCI.pVertexInputState = &vertexInputState;

   // Shaders
   auto vertShaderCode = Vulkan::ReadFile((m_bindir / "Assets/Shaders/Instance.vert.spv").string());
   auto fragShaderCode = Vulkan::ReadFile((m_bindir / "Assets/Shaders/Instance.frag.spv").string());

   std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
      vk::PipelineShaderStageCreateInfo {
         {}                                  /*flags*/,
         vk::ShaderStageFlagBits::eVertex    /*stage*/,
         CreateShaderModule(vertShaderCode)  /*module*/,
         "main"                              /*name*/,
         nullptr                             /*pSpecializationInfo*/
      },
      {
         {}                                  /*flags*/,
         vk::ShaderStageFlagBits::eFragment  /*stage*/,
         CreateShaderModule(fragShaderCode)  /*module*/,
         "main"                              /*name*/,
         nullptr                             /*pSpecializationInfo*/
      }
   };
   pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
   pipelineCI.pStages = shaderStages.data();

   m_Pipeline = m_Device.createGraphicsPipeline(m_PipelineCache, pipelineCI);

   // Shader modules are no longer needed once the graphics pipeline has been created
   DestroyShaderModule(shaderStages[0].module);
   DestroyShaderModule(shaderStages[1].module);
}


void RasterSpheres::DestroyPipeline() {
   if (m_Device && m_Pipeline) {
      m_Device.destroy(m_Pipeline);
   }
}


void RasterSpheres::CreateDescriptorPool() {
   std::array<vk::DescriptorPoolSize, 2> typeCounts = {
      vk::DescriptorPoolSize {
         vk::DescriptorType::eUniformBuffer,
         static_cast<uint32_t>(m_SwapChainFrameBuffers.size())
      },
      vk::DescriptorPoolSize {
         vk::DescriptorType::eCombinedImageSampler,
         static_cast<uint32_t>(m_SwapChainFrameBuffers.size())
      }
   };

   vk::DescriptorPoolCreateInfo descriptorPoolCI = {
      {}                                                     /*flags*/,
      static_cast<uint32_t>(m_SwapChainFrameBuffers.size())  /*maxSets*/,       // we create one uniform buffer per swap chain framebuffer, and one descriptor set for each uniform buffer.
      static_cast<uint32_t>(typeCounts.size())               /*poolSizeCount*/,
      typeCounts.data()                                      /*pPoolSizes*/
   };
   m_DescriptorPool = m_Device.createDescriptorPool(descriptorPoolCI);
}


void RasterSpheres::DestroyDescriptorPool() {
   if (m_Device && m_DescriptorPool) {
      m_Device.destroy(m_DescriptorPool);
   }
}


void RasterSpheres::CreateDescriptorSets() {

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
      std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
         {
            m_DescriptorSets[i]                /*dstSet*/,
            0                                  /*dstBinding*/,
            0                                  /*dstArrayElement*/,
            1                                  /*descriptorCount*/,
            vk::DescriptorType::eUniformBuffer /*descriptorType*/,
            nullptr                            /*pImageInfo*/,
            &m_UniformBuffers[i].m_Descriptor /*pBufferInfo*/,
            nullptr                            /*pTexelBufferView*/
         }
      };
      m_Device.updateDescriptorSets(writeDescriptorSets, nullptr);
   }
}


void RasterSpheres::DestroyDescriptorSets() {
   // don't need to do anything.  Destroying the pool will free them.
   //if (m_Device) {
   //   m_Device.freeDescriptorSets(m_DescriptorPool, m_DescriptorSets);
   //   m_DescriptorSets.clear();
   //}
}


void RasterSpheres::RecordCommandBuffers() {
   // Record the command buffers that are submitted to the graphics queue at each render.
   // We record one commend buffer per frame buffer (this allows us to pre-record the command
   // buffers, as we can bind the command buffer to its frame buffer.
   // (as opposed to having just one command buffer that gets built and bound to appropriate frame buffer
   // at render time)
   vk::CommandBufferBeginInfo commandBufferBI = {
      {}      /*flags*/,
      nullptr /*pInheritanceInfo*/
   };

   // Set clear values for all framebuffer attachments with loadOp set to clear
   // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
   std::array<vk::ClearValue, 2> clearValues = {
      vk::ClearColorValue {std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f}},
      vk::ClearDepthStencilValue {1.0f, 0}
   };

   vk::RenderPassBeginInfo renderPassBI = {
      m_RenderPass                               /*renderPass*/,
      nullptr                                    /*framebuffer*/,
      { {0,0}, m_Extent }                        /*renderArea*/,
      static_cast<uint32_t>(clearValues.size())  /*clearValueCount*/,
      clearValues.data()                         /*pClearValues*/
   };

   for (uint32_t i = 0; i < m_CommandBuffers.size(); ++i) {
      // Set target frame buffer
      renderPassBI.framebuffer = m_SwapChainFrameBuffers[i];

      vk::CommandBuffer& commandBuffer = m_CommandBuffers[i];

      commandBuffer.begin(commandBufferBI);

      // Start the first sub pass specified in the default render pass setup by the base application.
      // This will clear the color and depth attachment
      commandBuffer.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);

      // Update dynamic viewport state
      vk::Viewport viewport = {
         0.0f, 0.0f,
         static_cast<float>(m_Extent.width), static_cast<float>(m_Extent.height),
         0.0f, 1.0f
      };
      commandBuffer.setViewport(0, viewport);

      // Update dynamic scissor state
      vk::Rect2D scissor = {
         {0, 0},
         m_Extent
      };
      commandBuffer.setScissor(0, scissor);

      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineLayout, 0, m_DescriptorSets[i], nullptr);  // (i)th command buffer is bound to the (i)th descriptor set
      commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
      commandBuffer.bindVertexBuffers(0, m_VertexBuffer->m_Buffer, {0});
      commandBuffer.bindVertexBuffers(1, m_InstanceBuffer->m_Buffer, {0});
      commandBuffer.bindIndexBuffer(m_IndexBuffer->m_Buffer, 0, vk::IndexType::eUint32);
      commandBuffer.drawIndexed(m_IndexBuffer->m_Count, m_InstanceCount, 0, 0, 0);

      commandBuffer.endRenderPass();
      // Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to 
      // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

      commandBuffer.end();
   }
}


void RasterSpheres::Update(double deltaTime) {
   __super::Update(deltaTime);
   m_UniformBufferObject.projection = glm::perspective(glm::radians(45.0f), static_cast<float>(m_Extent.width) / static_cast<float>(m_Extent.height), 0.01f, 100.0f);
   m_UniformBufferObject.modelView = glm::lookAt(m_Eye, m_Eye + m_Direction, m_Up);
   m_UniformBufferObject.projection[1][1] *= -1;
}


void RasterSpheres::RenderFrame() {
   BeginFrame();
   m_UniformBuffers[m_CurrentImage].CopyFromHost(0, sizeof(UniformBufferObject), &m_UniformBufferObject);
   EndFrame();
}


void RasterSpheres::OnWindowResized() {
   __super::OnWindowResized();
   RecordCommandBuffers();
}
