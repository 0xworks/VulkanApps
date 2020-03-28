#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.hpp>

#include <array>

struct Vertex {
   glm::vec3 pos;
   glm::vec3 normal;

   Vertex(glm::vec3 p, glm::vec3 n) : pos(p), normal(n) {}

   bool operator==(const Vertex& other) const {
      return
         (pos == other.pos) &&
         (normal == other.normal)
      ;
   }

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
            offsetof(Vertex, pos)         /*offset*/
         },
         {
            1                             /*location*/,
            0                             /*binding*/,
            vk::Format::eR32G32B32Sfloat  /*format*/,
            offsetof(Vertex, normal)      /*offset*/
         }
      };
      return attributeDescriptions;
   }
};

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
