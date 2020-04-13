#pragma once

#include "Core.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using vec4 = glm::vec4;
using uint = uint32_t;
#include "Material.glsl"
#include "Texture.h"

#include <algorithm>

inline
Material Lambertian(const Texture& albedo) {
   return Material {
      MATERIAL_LAMBERTIAN,
      0.0f,
      0.0f,
      0.0f,

      0,
      0,
      albedo.type,
      0,

      albedo.param1,
      albedo.param2,
      {},
      {}
   };
}


inline
Material Layered(const Texture& albedo, const Texture& diffuse, const float roughness) {
   return Material {
      MATERIAL_LAYERED,
      std::clamp(roughness, 0.0f, 0.99f),
      0.0f,
      0.0f,

      0,
      0,
      albedo.type,
      diffuse.type,

      albedo.param1,
      albedo.param2,
      diffuse.param1,
      diffuse.param2
   };
}


inline
Material Metallic(const Texture& albedo, const float roughness) {
   return Material{
      MATERIAL_METALLIC,
      std::clamp(roughness, 0.0f, 0.99f),
      0.0f,
      0.0f,

      0,
      0,
      albedo.type,
      0,

      albedo.param1,
      albedo.param2,
      {},
      {}
   };
}


inline
Material Dielectric(const Texture& transmittance, const float refractiveIndex) {
   return Material{
      MATERIAL_DIELECTRIC,
      refractiveIndex,
      0.0f,
      0.0f,

      0,
      0,
      transmittance.type,
      0,

      transmittance.param1,
      transmittance.param2,
      {},
      {}
   };
}


// focus controls how focused the beam of light is.
// 0  = diffuse
// 1  = diffuse in hemisphere of normal
// >1 = increasingly focused in direction of normal
inline
Material Light(const Texture& emittance, const float focus) {
   return Material{
      MATERIAL_LIGHT,
      focus,
      0.0f,
      0.0f,

      0,
      0,
      emittance.type,
      0,

      emittance.param1,
      emittance.param2,
      {},
      {}
   };
}


// Smoke material requires custom intersection shader
// and so can only work with models where IsProcedural() returns true.
inline
Material Smoke(const Texture& albedo, const float density) {
   return Material {
      MATERIAL_SMOKE,
      -1.0f / std::max(density, 0.0000001f),
      0.0f,
      0.0f,

      0,
      0,
      albedo.type,
      0,

      albedo .param1,
      albedo.param2,
      {},
      {}
   };
}
