#pragma once

#include <glm/glm.hpp>

namespace Vulkan {

// An instance with the layout as expected by VK_NV_ray_tracing
struct GeometryInstance {
   glm::mat3x4 Transform;                  // Transform matrix, containing only the top 3 rows
   uint32_t InstanceCustomIndex : 24;      // Application defined instance index. This ends up in gl_InstanceCustomId in the shaders
   uint32_t Mask : 8;                      // Visibility mask
   uint32_t Offset : 24;                   // Index of the hit group which will be invoked when a ray hits the instance
   uint32_t Flags : 8;                     // Instance flags, such as culling
   uint64_t AccelerationStructureHandle; 

   GeometryInstance(
      glm::mat3x4 _Transform,
      uint32_t _InstanceCustomIndex,
      uint32_t _Mask,
      uint32_t _Offset,
      uint32_t _Flags,
      uint64_t _AccelerationStructureHandle
   )
      : Transform(_Transform)
      , InstanceCustomIndex(_InstanceCustomIndex)
      , Mask(_Mask)
      , Offset(_Offset)
      , Flags(_Flags)
      , AccelerationStructureHandle(_AccelerationStructureHandle)
   {}

};

}
