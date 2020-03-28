#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.hpp>

#include <array>

struct Vertex {
   glm::vec3 pos;
   glm::vec3 color;
   glm::vec2 textureCoordinate;

   Vertex(glm::vec3 p, glm::vec3 c, glm::vec2 t) : pos(p), color(c), textureCoordinate(t) {}

   bool operator==(const Vertex& other) const {
      return (pos == other.pos) && (color == other.color) && (textureCoordinate == other.textureCoordinate);
   }

   static auto GetBindingDescription() {
      static vk::VertexInputBindingDescription bindingDescription = {
         0,
         sizeof(Vertex)
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
            offsetof(Vertex, color)       /*offset*/
         },
         {
            2                                    /*location*/,
            0                                    /*binding*/,
            vk::Format::eR32G32Sfloat            /*format*/,
            offsetof(Vertex, textureCoordinate)  /*offset*/
         }
      };
      return attributeDescriptions;
   }
};

namespace std {

template<> struct hash<Vertex> {
   size_t operator()(Vertex const& vertex) const {
      return ((hash<glm::vec3>()(vertex.pos) ^
         (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
         (hash<glm::vec2>()(vertex.textureCoordinate) << 1);
   }
};

}
