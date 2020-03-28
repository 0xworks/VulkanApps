#include "Application.h"

#include "Buffer.h"
#include "Image.h"
#include "Vertex.h"

#include <filesystem>
#include <memory>

class RasterSpheres final : public Vulkan::Application {
public:
   RasterSpheres(int argc, const char* argv[]);
   ~RasterSpheres();

   struct UniformBufferObject {
      alignas(16) glm::mat4 projection;
      alignas(16) glm::mat4 modelView;
      alignas(16) glm::vec4 lightPos = glm::vec4(50.0f, 50.0f, 0.0f, 1.0f);
   };

   vk::PhysicalDeviceFeatures GetRequiredPhysicalDeviceFeatures(vk::PhysicalDeviceFeatures);

   virtual void Init() override;

   void LoadModel();

   void CreateVertexBuffer();
   void DestroyVertexBuffer();

   void CreateIndexBuffer();
   void DestroyIndexBuffer();

   void CreateInstanceBuffer();
   void DestroyInstanceBuffer();

   void CreateUniformBuffers();
   void DestroyUniformBuffers();

   void CreateDescriptorSetLayout();
   void DestroyDescriptorSetLayout();

   void CreatePipelineLayout(); // depends on descriptor set layout
   void DestroyPipelineLayout();

   void CreatePipeline();
   void DestroyPipeline();

   void CreateDescriptorPool();
   void DestroyDescriptorPool();

   void CreateDescriptorSets();
   void DestroyDescriptorSets();

   void RecordCommandBuffers();

   virtual void Update(double deltaTime) override;

   virtual void RenderFrame() override;

   virtual void OnWindowResized() override;


private:
   std::filesystem::path m_bindir;
   std::vector<Vertex> m_Vertices;
   std::unique_ptr<Vulkan::Buffer> m_VertexBuffer;
   std::vector<uint32_t> m_Indices;
   std::unique_ptr<Vulkan::IndexBuffer> m_IndexBuffer;
   std::unique_ptr<Vulkan::Buffer> m_InstanceBuffer;
   UniformBufferObject m_UniformBufferObject;
   std::vector<Vulkan::Buffer> m_UniformBuffers;
   vk::DescriptorSetLayout m_DescriptorSetLayout;
   vk::PipelineLayout m_PipelineLayout;
   vk::Pipeline m_Pipeline;
   vk::DescriptorPool m_DescriptorPool;
   std::vector<vk::DescriptorSet> m_DescriptorSets;
   uint32_t m_InstanceCount = 0;

};
