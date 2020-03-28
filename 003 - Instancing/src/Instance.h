#pragma once

#include <glm/glm.hpp>

struct Instance {
   glm::vec3 pos;
   glm::vec3 rot;
   float scale;
   uint32_t texIndex;

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
            vk::Format::eR32G32B32Sfloat  /*format*/,
            offsetof(Instance, rot)      /*offset*/
         },
         {
            2                             /*location*/,
            0                             /*binding*/,
            vk::Format::eR32Sfloat        /*format*/,
            offsetof(Instance, scale)     /*offset*/
         },
         {
            3                             /*location*/,
            0                             /*binding*/,
            vk::Format::eR32Sint          /*format*/,
            offsetof(Instance, texIndex)  /*offset*/
         }
      };
      return attributeDescriptions;
   }


};
