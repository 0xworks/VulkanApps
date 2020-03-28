#pragma once

#include <glm/glm.hpp>

struct Instance {
   glm::vec3 pos;
   float scale;
   glm::vec3 color;

   Instance(glm::vec3 p, float s, glm::vec3 c) : pos(p), scale(s), color(c) {}

   static auto GetBindingDescription() {
      static vk::VertexInputBindingDescription bindingDescription = {
         0,
         sizeof(Instance),
         vk::VertexInputRate::eInstance
      };
      return bindingDescription;
   }

   static auto GetAttributeDescriptions() {
      static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions = {
         {
            0                             /*location*/,
            0                             /*binding*/,
            vk::Format::eR32G32B32Sfloat  /*format*/,
            offsetof(Instance, pos)       /*offset*/
         },
         {
            1                             /*location*/,
            0                             /*binding*/,
            vk::Format::eR32Sfloat        /*format*/,
            offsetof(Instance, scale)     /*offset*/
         },
         {
            2                             /*location*/,
            0                             /*binding*/,
            vk::Format::eR32G32B32Sfloat  /*format*/,
            offsetof(Instance, color)     /*offset*/
         }
      };
      return attributeDescriptions;
   }


};
