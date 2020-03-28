#include "Application.h"

using mat4 = glm::mat4;
using vec4 = glm::vec4;
using vec3 = glm::vec3;
using uint = uint32_t;

#include "Buffer.h"
#include "Constants.h"
#include "Image.h"
#include "UniformBufferObject.h"
#include "Vertex.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <filesystem>
#include <memory>

class RayTraceSpheres final : public Vulkan::Application {
public:

   RayTraceSpheres(int argc, const char* argv[]);
   ~RayTraceSpheres();

   std::vector<const char*> GetRequiredInstanceExtensions() override;
   std::vector<const char*> GetRequiredDeviceExtensions() override;

   virtual void Init() override;

   void CreateScene();
   void DestroyScene();

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
   Constants m_Constants {
      8 /*maxRayBounces*/,
      2 /*lensAperture*/,
      1 /*lensFocalLength*/
   };
   std::unique_ptr<Vulkan::Buffer> m_VertexBuffer;
   std::unique_ptr<Vulkan::IndexBuffer> m_IndexBuffer;
   std::unique_ptr<Vulkan::Buffer> m_TransformBuffer;
   std::unique_ptr<Vulkan::Buffer> m_MaterialBuffer;
   std::unique_ptr<Vulkan::Image> m_OutputImage;
   std::unique_ptr<Vulkan::Image> m_AccumumlationImage;
   UniformBufferObject m_UniformBufferObject;
   std::vector<Vulkan::Buffer> m_UniformBuffers;
   vk::PhysicalDeviceRayTracingPropertiesNV m_RayTracingProperties;
   vk::DescriptorSetLayout m_DescriptorSetLayout;
   vk::PipelineLayout m_PipelineLayout;
   vk::Pipeline m_Pipeline;
   std::unique_ptr<Vulkan::Buffer> m_ShaderBindingTable;
   vk::DescriptorPool m_DescriptorPool;
   std::vector<vk::DescriptorSet> m_DescriptorSets;

};


#pragma once

bool operator==(const Vertex& a, const Vertex& b) {
   return
      (a.pos == b.pos) &&
      (a.normal == b.normal)
   ;
}

namespace std {

template<> struct hash<Vertex> {
   size_t operator()(Vertex const& vertex) const {
      size_t seed = 0;
      hash<glm::vec3> hasherV3;
      glm::detail::hash_combine(seed, hasherV3(vertex.pos));
      glm::detail::hash_combine(seed, hasherV3(vertex.normal));
      return seed;
   }
};

}

