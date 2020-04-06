#include "Material.glsl"
#include "Random.glsl"
#include "RayPayload.glsl"
#include "SNoise.glsl"
#include "Texture.glsl"

layout(binding = BINDING_MATERIALBUFFER) readonly buffer MaterialArray { Material materials[]; };
//layout(set = 0, binding = BINDING_TEXTUREBUFFER) readonly buffer TextureArray { SamplerSomething textures[]; };

float Schlick(float cosine, float refractiveIndex) {
    float r0 = (1 - refractiveIndex) / (1 + refractiveIndex);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}


vec3 Color(const vec3 hitPoint, const vec3 normal, const vec2 texCoord, const Material material) {
   switch(material.textureId) {
      case TEXTURE_FLATCOLOR: {
         // flat color
         return material.textureParam1.rgb;
      }

      case TEXTURE_CHECKERBOARD: {
         // checkboard
         // WARNING: This texture doesnt work too well if you have a large axis-aligned face at some value where sin(value) is exactly 0
         //          (For example, a large cube where one of the faces is axis-aligned at z = 0)
         //          You will end up with floating point precision issues with sin(hitPoint.z) sometimes being slightly less than 0 and sometimes
         //          slightly > 0.
         const vec3 oddColor = material.textureParam1.rgb;
         const vec3 evenColor = material.textureParam2.rgb;
         const float scale = material.textureParam1.w;
         float sineProduct = (sin(hitPoint.x * scale) >= 0.0? 1 : -1) * (sin(hitPoint.y * scale) >= 0.0? 1 : -1) * (sin(hitPoint.z * scale) >= 0.0? 1 : -1);
         if (sineProduct < 0) {
            return oddColor;
         } else {
            return evenColor;
         }
      }

      case TEXTURE_SIMPLEX3D: {
         vec3 p = gl_WorldToObjectNV * vec4(hitPoint, 1);
         p *= material.textureParam2.w; // scale
         return material.textureParam1.rgb * vec3(0.5 + material.textureParam1.w * snoise(p));
      }

      case TEXTURE_NORMALS: {
         return (vec3(1.0f) + normal) / 2.0f;
      }

      case TEXTURE_UV: {
         return vec3(texCoord, 0.0f);
      }

      case TEXTURE_RED: {
         return vec3(1.0f, 0.0f, 0.0f);
      }

      default: {
         // sample from textures, indexed by textureId
         return material.textureParam1.rgb; // TODO
      }
   }
}


RayPayload Scatter(const vec3 direction, const vec3 hitPoint, const vec3 normal, const vec2 texCoord, const uint materialIndex, uint randomSeed) {
   Material material = materials[materialIndex];

   switch(material.type) {
      case MATERIAL_FLATCOLOR: {
         // for debugging.  no scattering.
         const vec4 colorAndDistance = vec4(Color(hitPoint, normal, texCoord, material), gl_HitTNV);
         return RayPayload(colorAndDistance, vec4(0), randomSeed);
      }
      case MATERIAL_LAMBERTIAN: {
         const vec4 colorAndDistance = vec4(Color(hitPoint, normal, texCoord, material), gl_HitTNV);
         const vec3 target = hitPoint + normal + RandomUnitVector(randomSeed);
         const vec4 scatterDirection = vec4(target - hitPoint, 1);
         return RayPayload(colorAndDistance, scatterDirection, randomSeed);
      }
      case MATERIAL_METALLIC: {
         const vec3 color = Color(hitPoint, normal, texCoord, material);
         const vec3 scatterDirection = reflect(direction, normal) + material.roughness * RandomInUnitSphere(randomSeed);
         return RayPayload(vec4(color, gl_HitTNV), vec4(scatterDirection, dot(scatterDirection, normal) > 0), randomSeed);
      }
      case MATERIAL_DIELECTRIC: {
         vec3 outward_normal;
         float ni_over_nt;
         float reflect_prob;
         float cosine;
         if (dot(direction, normal) > 0) {
            outward_normal = -normal;
            ni_over_nt = material.refractiveIndex;
            cosine = ni_over_nt * dot(direction, normal) / length(direction);
         } else {
            outward_normal = normal;
            ni_over_nt = 1.0 / material.refractiveIndex;
            cosine = -dot(direction, normal) / length(direction);
         }
         const vec3 refracted = refract(direction, outward_normal, ni_over_nt);
         const vec4 colorAndDistance = vec4(1.0, 1.0, 1.0, gl_HitTNV);
         if(dot(refracted, refracted) > 0) {
            reflect_prob = Schlick(cosine, material.refractiveIndex);
         } else {
            reflect_prob = 1.0;
         }
         if(RandomFloat(randomSeed) < reflect_prob) {
            const vec3 reflected = reflect(direction, normal);
            return RayPayload(colorAndDistance, vec4(reflected, 1), randomSeed);
         }
         return RayPayload(colorAndDistance, vec4(refracted, 1), randomSeed);
      }
   }
}

