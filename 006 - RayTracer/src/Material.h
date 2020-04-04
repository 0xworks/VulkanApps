#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using vec4 = glm::vec4;
using uint = uint32_t;
#include "Material.glsl"
#include "Texture.h"

inline
Material Lambertian(Texture texture) {
   return Material {MATERIAL_LAMBERTIAN, 0.0, 0.0, texture.id, texture.param1, texture.param2};
}


inline
Material Metallic(Texture texture, float roughness) {
   return Material {MATERIAL_METALLIC, roughness, 0.0, texture.id, texture.param1, texture.param2};
}


inline
Material Dielectric(float refractiveIndex) {
   return Material {MATERIAL_DIELECTRIC, 0.0, refractiveIndex};
}
