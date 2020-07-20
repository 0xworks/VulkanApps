#include "Triangle.h"
#include "Utility.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

std::unique_ptr<Vulkan::Application> CreateApplication(int argc, const char* argv[]) {
   return std::make_unique<Triangle>(argc, argv);
}

Triangle::Triangle(int argc, const char* argv[])
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


Triangle::~Triangle() {
   DestroyDescriptorSets();
   DestroyDescriptorPool();
   DestroyPipeline();
   DestroyPipelineLayout();
   DestroyDescriptorSetLayout();
   DestroyUniformBuffers();
   DestroyIndexBuffer();
   DestroyVertexBuffer();
}


void Triangle::Init() {
   Vulkan::Application::Init();
   CreateVertexBuffer();
   CreateIndexBuffer();
   CreateUniformBuffers();
   CreateDescriptorSetLayout();
   CreatePipelineLayout();
   CreatePipeline();
   CreateDescriptorPool();
   CreateDescriptorSets();
   RecordCommandBuffers();
}


void Triangle::CreateVertexBuffer() {
   // A note on memory management in Vulkan in general:
   // This is a very complex topic and while it's fine for an example application to allocate small individual memory allocations
   // that is not what should be done a real-world application, where you should allocate large chunks of memory at once instead.

   // Setup vertices
   std::vector<Vertex> vertices = {
      { {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
      { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
      { {  0.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
   };

   vk::DeviceSize size = vertices.size() * sizeof(Vertex);

   Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
   stagingBuffer.CopyFromHost(0, size, vertices.data());

   m_VertexBuffer = std::make_unique<Vulkan::Buffer>(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
   CopyBuffer(stagingBuffer.m_Buffer, m_VertexBuffer->m_Buffer, 0, 0, size);
}


void Triangle::DestroyVertexBuffer() {
   m_VertexBuffer.reset(nullptr);
}


void Triangle::CreateIndexBuffer() {
   std::vector<uint32_t> indices = {0, 1, 2};

   uint32_t count = static_cast<uint32_t>(indices.size());
   vk::DeviceSize size = count * sizeof(uint32_t);

   Vulkan::Buffer stagingBuffer(m_Device, m_PhysicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
   stagingBuffer.CopyFromHost(0, size, indices.data());

   m_IndexBuffer = std::make_unique<Vulkan::IndexBuffer>(m_Device, m_PhysicalDevice, size, count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
   CopyBuffer(stagingBuffer.m_Buffer, m_IndexBuffer->m_Buffer, 0, 0, size);
}


void Triangle::DestroyIndexBuffer() {
   m_IndexBuffer.reset(nullptr);
}


void Triangle::CreateUniformBuffers() {
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


void Triangle::DestroyUniformBuffers() {
   m_UniformBuffers.clear();
}


void Triangle::CreateDescriptorSetLayout() {
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

   std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {uboLayoutBinding};

   m_DescriptorSetLayout = m_Device.createDescriptorSetLayout({
      {}                                           /*flags*/,
      static_cast<uint32_t>(layoutBindings.size()) /*bindingCount*/,
      layoutBindings.data()                        /*pBindings*/
   });
}


void Triangle::DestroyDescriptorSetLayout() {
   if (m_Device && m_DescriptorSetLayout) {
      m_Device.destroy(m_DescriptorSetLayout);
      m_DescriptorSetLayout = nullptr;
   }
}


void Triangle::CreatePipelineLayout() {
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


void Triangle::DestroyPipelineLayout() {
   if (m_Device && m_PipelineLayout) {
      m_Device.destroy(m_PipelineLayout);
      m_PipelineLayout = nullptr;
   }
}

void Triangle::CreatePipeline() {
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
      {vk::CullModeFlagBits::eNone}    /*cullMode*/,
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
   auto bindingDescription = Vertex::GetBindingDescription();
   auto attributeDescriptions = Vertex::GetAttributeDescriptions();

   // Vertex input state used for pipeline creation
   vk::PipelineVertexInputStateCreateInfo vertexInputState = {
      {}                                                    /*flags*/,
      1                                                     /*vertexBindingDescriptionCount*/,
      &bindingDescription                                   /*pVertexBindingDescriptions*/,
      static_cast<uint32_t>(attributeDescriptions.size())   /*vertexAttributeDescriptionCount*/,
      attributeDescriptions.data()                          /*pVertexAttributeDescriptions*/
   };
   pipelineCI.pVertexInputState = &vertexInputState;

   // Shaders
   auto vertShaderCode = Vulkan::ReadFile((m_bindir / "Assets/Shaders/Triangle.vert.spv").string());
   auto fragShaderCode = Vulkan::ReadFile((m_bindir / "Assets/Shaders/Triangle.frag.spv").string());

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

   // .value works around bug in Vulkan.hpp (refer https://github.com/KhronosGroup/Vulkan-Hpp/issues/659)
   m_Pipeline = m_Device.createGraphicsPipeline(m_PipelineCache, pipelineCI).value;

   // Shader modules are no longer needed once the graphics pipeline has been created
   DestroyShaderModule(shaderStages[0].module);
   DestroyShaderModule(shaderStages[1].module);
}


void Triangle::DestroyPipeline() {
   if (m_Device && m_Pipeline) {
      m_Device.destroy(m_Pipeline);
   }
}


void Triangle::CreateDescriptorPool() {
   std::array<vk::DescriptorPoolSize, 1> typeCounts = {
      vk::DescriptorPoolSize {
         vk::DescriptorType::eUniformBuffer,
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


void Triangle::DestroyDescriptorPool() {
   if (m_Device && m_DescriptorPool) {
      m_Device.destroy(m_DescriptorPool);
   }
}


void Triangle::CreateDescriptorSets() {

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
      vk::WriteDescriptorSet writeDescriptorSet = {
         m_DescriptorSets[i]                             /*dstSet*/,
         0                                               /*dstBinding*/,
         0                                               /*dstArrayElement*/,
         1                                               /*descriptorCount*/,
         vk::DescriptorType::eUniformBuffer              /*descriptorType*/,
         nullptr                                         /*pImageInfo*/,
         &m_UniformBuffers[i].m_Descriptor               /*pBufferInfo*/,
         nullptr                                         /*pTexelBufferView*/
      };
      m_Device.updateDescriptorSets(writeDescriptorSet, nullptr);
   }
}


void Triangle::DestroyDescriptorSets() {
   // don't need to do anything.  Destroying the pool will free them.
   //if (m_Device) {
   //   m_Device.freeDescriptorSets(m_DescriptorPool, m_DescriptorSets);
   //   m_DescriptorSets.clear();
   //}
}


void Triangle::RecordCommandBuffers() {
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
      commandBuffer.bindIndexBuffer(m_IndexBuffer->m_Buffer, 0, vk::IndexType::eUint32);
      commandBuffer.drawIndexed(m_IndexBuffer->m_Count, 1, 0, 0, 0);

      commandBuffer.endRenderPass();
      // Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to 
      // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

      commandBuffer.end();
   }

}


void Triangle::RenderFrame() {
   BeginFrame();

   glm::mat4 model = glm::rotate(glm::identity<glm::mat4>(), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
   glm::mat4 view = glm::lookAt(m_Eye, m_Eye + m_Direction, m_Up);
   glm::mat4 proj = glm::perspective(m_FoVRadians, static_cast<float>(m_Extent.width) / static_cast<float>(m_Extent.height), 0.01f, 100.0f);
   proj[1][1] *= -1;

   UniformBufferObject ubo {
      proj * view * model
   };

   m_UniformBuffers[m_CurrentImage].CopyFromHost(0, sizeof(ubo), &ubo);

   EndFrame();
}


void Triangle::OnWindowResized() {
   __super::OnWindowResized();
   RecordCommandBuffers();
}
