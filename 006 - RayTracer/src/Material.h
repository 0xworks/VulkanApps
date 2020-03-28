#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using vec4 = glm::vec4;
using uint = uint32_t;
#include "Material.glsl"


inline
Material Lambertian(glm::vec3 albedo) {
   return Material {glm::vec4{albedo, 1 }, MATERIAL_LAMBERTIAN};
}


inline
Material Metallic(glm::vec3 albedo, float roughness) {
   return Material {glm::vec4{albedo, 1 }, MATERIAL_METALLIC, roughness};
}


inline
Material Dielectric(float refractiveIndex) {
   return Material {glm::one<vec4>(), MATERIAL_DIELECTRIC, 0.0, refractiveIndex};
}
