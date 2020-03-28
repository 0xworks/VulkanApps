#include "Application.h"

#include "Buffer.h"
#include "Image.h"
#include "Scene.h"

#include <filesystem>
#include <memory>

class RayTracer final : public Vulkan::Application {
public:

   RayTracer(int argc, const char* argv[]);
   ~RayTracer();

   std::vector<const char*> GetRequiredInstanceExtensions() override;
   std::vector<const char*> GetRequiredDeviceExtensions() override;

   virtual void Init() override;

   void CreateScene();
   void DestroyScene();

   void CreateVertexBuffer();
   void DestroyVertexBuffer();

   void CreateIndexBuffer();
   void DestroyIndexBuffer();

   void CreateAccelerationStructures();
   void DestroyAccelerationStructures();

   void CreateStorageImages();
   void DestroyStorageImages();

   void CreateUniformBuffers();
   void DestroyUniformBuffers();

   void CreateDescriptorSetLayout();
   void DestroyDescriptorSetLayout();

   void CreatePipelineLayout(); // depends on descriptor set layout
   void DestroyPipelineLayout();

   void CreatePipeline();
   void DestroyPipeline();

   void CreateShaderBindingTable();
   void DestroyShaderBindingTable();

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
   Scene m_Scene;
   std::unique_ptr<Vulkan::Buffer> m_VertexBuffer;
   std::unique_ptr<Vulkan::IndexBuffer> m_IndexBuffer;
   std::unique_ptr<Vulkan::Buffer> m_MaterialBuffer;
   std::unique_ptr<Vulkan::Image> m_OutputImage;
   std::unique_ptr<Vulkan::Image> m_AccumumlationImage;
   uint32_t m_AccumulatedImageCount = 0;
   std::vector<Vulkan::Buffer> m_UniformBuffers;
   vk::PhysicalDeviceRayTracingPropertiesNV m_RayTracingProperties;
   vk::DescriptorSetLayout m_DescriptorSetLayout;
   vk::PipelineLayout m_PipelineLayout;
   vk::Pipeline m_Pipeline;
   std::unique_ptr<Vulkan::Buffer> m_ShaderBindingTable;
   vk::DescriptorPool m_DescriptorPool;
   std::vector<vk::DescriptorSet> m_DescriptorSets;

};
