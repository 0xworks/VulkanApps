#include "Application.h"

#include "Buffer.h"
#include "Image.h"
#include "Vertex.h"

#include <filesystem>
#include <memory>

class TexturedModel final : public Vulkan::Application {
public:
   TexturedModel(int argc, const char* argv[]);
   ~TexturedModel();

   struct UniformBufferObject {
      alignas(16) glm::mat4 MVP; // Model View Projection
   };

   virtual vk::PhysicalDeviceFeatures GetRequiredPhysicalDeviceFeatures(vk::PhysicalDeviceFeatures) override;

   virtual void Init() override;

   void LoadModel();

   void CreateVertexBuffer();
   void DestroyVertexBuffer();

   void CreateIndexBuffer();
   void DestroyIndexBuffer();

   void CreateTextureResources();
   void DestroyTextureResources();

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

   virtual void Update(double deltaTime);

   virtual void RenderFrame() override;

   virtual void OnWindowResized() override;

private:
   std::filesystem::path m_bindir;
   std::vector<Vertex> m_Vertices;
   std::unique_ptr<Vulkan::Buffer> m_VertexBuffer;
   std::vector<uint32_t> m_Indices;
   std::unique_ptr<Vulkan::IndexBuffer> m_IndexBuffer;
   std::unique_ptr<Vulkan::Image> m_Texture;
   vk::Sampler m_TextureSampler;
   std::vector<Vulkan::Buffer> m_UniformBuffers;
   vk::DescriptorSetLayout m_DescriptorSetLayout;
   vk::PipelineLayout m_PipelineLayout;
   vk::Pipeline m_Pipeline;
   vk::DescriptorPool m_DescriptorPool;
   std::vector<vk::DescriptorSet> m_DescriptorSets;
   float m_ModelRotation = 0.0f;
   const float m_ModelRotationSpeed = 50.0f;
};
