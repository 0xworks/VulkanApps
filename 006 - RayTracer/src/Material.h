#pragma once

#include "Core.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using vec4 = glm::vec4;
using uint = uint32_t;
#include "Material.glsl"
#include "Texture.h"


inline
Material Lambertian(const Texture& texture) {
   return Material{MATERIAL_LAMBERTIAN, 0.0f, 0.0f, texture.id, texture.param1, texture.param2};
}


inline
Material Metallic(const Texture& texture, float roughness) {
   return Material{MATERIAL_METALLIC, roughness, 0.0f, texture.id, texture.param1, texture.param2};
}


inline
Material Dielectric(const float refractiveIndex) {
   return Material{MATERIAL_DIELECTRIC, refractiveIndex};
}


inline
Material DiffuseLight(const Texture& texture) {
   return Material{MATERIAL_DIFFUSELIGHT, 0.0f, 0.0f, texture.id, texture.param1, texture.param2};
}

// Smoke material requires custom intersection shader
// and so can only work with models where IsProcedural() returns true.
inline
Material Smoke(const float density, const Texture& texture) {
   return Material{MATERIAL_SMOKE, -1.0f / std::max(density, 0.0000001f), 0.0f, texture.id, texture.param1, texture.param2};
}
