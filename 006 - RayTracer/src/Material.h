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
Material Lambertian(const Texture& texture) {
   return Material{MATERIAL_LAMBERTIAN, 0.0f, 0.0f, texture.id, texture.param1, texture.param2};
}


// A blend between Lambertian (blend = 0.0) and Metallic (blend = 1.0)
inline
Material Blended(const Texture& texture, const float blend, const float roughness) {
   return Material {MATERIAL_BLENDED, std::clamp(roughness, 0.0f, 0.99f), std::clamp(blend, 0.0f, 1.0f), texture.id, texture.param1, texture.param2};
}

inline
Material Metallic(const Texture& texture, const float roughness) {
   return Material{MATERIAL_METALLIC,  std::clamp(roughness, 0.0f, 0.99f), 1.0f, texture.id, texture.param1, texture.param2};
}


inline
Material Dielectric(const Texture& texture, const float refractiveIndex) {
   return Material{MATERIAL_DIELECTRIC, refractiveIndex, 0.0f, texture.id, texture.param1, texture.param2};
}


// focus controls how focused the beam of light is.
// 0  = diffuse
// 1  = diffuse in hemisphere of normal
// >1 = increasingly focused in direction of normal
inline
Material Light(const Texture& texture, const float focus) {
   return Material{MATERIAL_LIGHT, focus, 0.0f, texture.id, texture.param1, texture.param2};
}


// Smoke material requires custom intersection shader
// and so can only work with models where IsProcedural() returns true.
inline
Material Smoke(const float density, const Texture& texture) {
   return Material{MATERIAL_SMOKE, -1.0f / std::max(density, 0.0000001f), 0.0f, texture.id, texture.param1, texture.param2};
}
