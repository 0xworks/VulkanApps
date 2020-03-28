#include "Application.h"

#include "Buffer.h"

#include <filesystem>
#include <memory>

class Triangle final : public Vulkan::Application {
public:
   Triangle(int argc, const char* argv[]);
   ~Triangle();

   struct Vertex {
      glm::vec3 Pos;
      glm::vec3 Color;

      static auto GetBindingDescription() {
         static vk::VertexInputBindingDescription bindingDescription = {
            0,
            sizeof(Vertex),
            vk::VertexInputRate::eVertex
         };
         return bindingDescription;
      }

      static auto GetAttributeDescriptions() {
         static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions = {
            {
               0                             /*location*/,
               0                             /*binding*/,
               vk::Format::eR32G32B32Sfloat  /*format*/,
               offsetof(Vertex, Pos)         /*offset*/
            },
            {
               1                             /*location*/,
               0                             /*binding*/,
               vk::Format::eR32G32B32Sfloat  /*format*/,
               offsetof(Vertex, Color)       /*offset*/
            },
         };
         return attributeDescriptions;
      }

   };

   struct UniformBufferObject {
      alignas(16) glm::mat4 MVP; // Model View Projection
   };

   virtual void Init() override;

   void CreateVertexBuffer();
   void DestroyVertexBuffer();

   void CreateIndexBuffer();
   void DestroyIndexBuffer();

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

   virtual void RenderFrame() override;

   virtual void OnWindowResized() override;

private:
   std::filesystem::path m_bindir;
   std::unique_ptr<Vulkan::Buffer> m_VertexBuffer;
   std::unique_ptr<Vulkan::IndexBuffer> m_IndexBuffer;
   std::vector<Vulkan::Buffer> m_UniformBuffers;
   vk::DescriptorSetLayout m_DescriptorSetLayout;
   vk::PipelineLayout m_PipelineLayout;
   vk::Pipeline m_Pipeline;
   vk::DescriptorPool m_DescriptorPool;
   std::vector<vk::DescriptorSet> m_DescriptorSets;
};
