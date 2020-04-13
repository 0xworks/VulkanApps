#extension GL_EXT_nonuniform_qualifier : require

#include "Material.glsl"
#include "Random.glsl"
#include "RayPayload.glsl"
#include "SNoise.glsl"
#include "Texture.glsl"

layout(set = 0, binding = BINDING_TLAS) uniform accelerationStructureNV world;
layout(set = 0, binding = BINDING_MATERIALBUFFER) readonly buffer MaterialArray { Material materials[]; };
layout(set = 0, binding = BINDING_TEXTURESAMPLERS) uniform sampler2D[] samplers;

layout(location = 1) rayPayloadNV RayPayload ray1;

float Schlick(float cosine, float refractiveIndex) {
    float r0 = (1 - refractiveIndex) / (1 + refractiveIndex);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5.0);
}


float Turbulence(vec3 p, int depth) {
   float accum = 0.0;
   vec3 temp_p = p;
   float weight = 1.0;

   for (int i = 0; i < depth; ++i) {
      accum += weight * snoise(temp_p);
      weight *= 0.5;
      temp_p *= 2.0;
   }

   return abs(accum);
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
         float sineProduct = (sin(hitPoint.x * scale) >= 0.0? 1.0 : -1.0) * (sin(hitPoint.y * scale) >= 0.0? 1.0 : -1.0) * (sin(hitPoint.z * scale) >= 0.0? 1.0 : -1.0);
         if (sineProduct < 0) {
            return oddColor;
         } else {
            return evenColor;
         }
      }

      case TEXTURE_SIMPLEX3D: {
         vec3 p = hitPoint;
         p *= material.textureParam2.w; // scale
         return mix(material.textureParam1.rgb, vec3(snoise(p)), material.textureParam1.w);
      }

      case TEXTURE_TURBULENCE: {
         vec3 p = hitPoint;
         p *= material.textureParam2.w; // scale
         return mix(material.textureParam1.rgb, vec3(Turbulence(p, int(material.textureParam2.z))), material.textureParam1.w);   
      }

      case TEXTURE_MARBLE: {
         vec3 p = hitPoint;
         p *= material.textureParam2.w; // scale
         return mix(material.textureParam1.rgb, vec3(sin(p.z + 10.0 * Turbulence(p, int(material.textureParam2.z)))), material.textureParam1.w);
      }

      case TEXTURE_NORMALS: {
         return (vec3(1.0) + normal) / 2.0;
      }

      case TEXTURE_UV: {
         return vec3(texCoord, 0.0);
      }

      case TEXTURE_RED: {
         return vec3(1.0, 0.0, 0.0);
      }

      default: {
         // sample from textures, indexed by textureId
         return texture(samplers[material.textureId], texCoord).rgb;
      }
   }
}


RayPayload ScatterLambertian(const vec3 hitPoint, const vec3 normal, const vec3 color, inout uint randomSeed) {
   const vec3 target = hitPoint + normal + 0.99 * RandomUnitVector(randomSeed);
   return RayPayload(vec4(color, gl_HitTNV), vec4(0.0), vec4(normalize(target - hitPoint), 1.0), randomSeed);
}


RayPayload ScatterMetallic(const vec3 hitPoint, const vec3 normal, const vec3 color, const float roughness, inout uint randomSeed) {
   const vec3 scatterDirection = normalize(reflect(gl_WorldRayDirectionNV, normal) + roughness * RandomInUnitSphere(randomSeed));
   return RayPayload(vec4(color, gl_HitTNV), vec4(0.0), vec4(scatterDirection, 1.0), randomSeed);
}


RayPayload Scatter(const vec3 hitPoint, const vec3 normal, const vec2 texCoord, const uint materialIndex, inout uint randomSeed) {
   Material material = materials[materialIndex];

   switch(material.type) {

      case MATERIAL_LAMBERTIAN: {
         return ScatterLambertian(hitPoint, normal, Color(hitPoint, normal, texCoord, material), randomSeed);
      }

      case MATERIAL_BLENDED: {
         const vec3 color = Color(hitPoint, normal, texCoord, material);
         const vec3 colorSpecular = color * material.materialParameter2;
         const vec3 colorDiffuse = color * (1.0 - material.materialParameter2);

         float specularChance = dot(colorSpecular, vec3(1.0/3.0));
         float diffuseChance = dot(colorDiffuse, vec3(1.0/3.0));
         float sum = specularChance + diffuseChance;
         if(sum > 0.0001) {
            diffuseChance /= sum;
            specularChance /= sum;
         } else {
            diffuseChance = 1.0f;
            specularChance = 0.0f;
         }

         const float select = RandomFloat(randomSeed);
         if (select < specularChance) {
             return ScatterMetallic(hitPoint, normal, colorSpecular, material.materialParameter1, randomSeed);
         } else {
            return ScatterLambertian(hitPoint, normal, colorDiffuse, randomSeed);
         }
      }

      case MATERIAL_METALLIC: {
         return ScatterMetallic(hitPoint, normal, Color(hitPoint, normal, texCoord, material), material.materialParameter1, randomSeed);
      }

      case MATERIAL_DIELECTRIC: {
         vec3 outward_normal;
         float ni_over_nt;
         float reflect_prob;
         float cosine;
         if (dot(gl_WorldRayDirectionNV, normal) > 0.0) {
            outward_normal = -normal;
            ni_over_nt = material.materialParameter1;
            cosine = ni_over_nt * dot(gl_WorldRayDirectionNV, normal);
         } else {
            outward_normal = normal;
            ni_over_nt = 1.0 / material.materialParameter1;
            cosine = -dot(gl_WorldRayDirectionNV, normal);
         }
         const vec3 refracted = refract(gl_WorldRayDirectionNV.xyz, outward_normal, ni_over_nt);

         // fake colored glass.. I dont think it really behaves like this (e.g. shouldn't attenuation be proportional to how much
         // of the material the ray passes through)?
         const vec4 attenuationAndDistance = vec4(Color(hitPoint, normal, texCoord, material), gl_HitTNV);

         if(dot(refracted, refracted) > 0.0) {
            reflect_prob = Schlick(cosine, material.materialParameter1);
         } else {
            reflect_prob = 1.0;
         }
         if(RandomFloat(randomSeed) < reflect_prob) {
            const vec3 reflected = reflect(gl_WorldRayDirectionNV, normal);
            return RayPayload(attenuationAndDistance, vec4(0.0), vec4(reflected, 1), randomSeed);
         }
         return RayPayload(attenuationAndDistance, vec4(0.0), vec4(refracted, 1), randomSeed);
      } 

      case MATERIAL_LIGHT: {
         float emit = 1.0;
         if(material.materialParameter1 > 0.0) {
            emit = pow(max(0.0, -dot(gl_WorldRayDirectionNV, normal)), material.materialParameter1);
         }
         const vec3 color = Color(hitPoint, normal, texCoord, material.albedoTextureType, material.albedoTextureParam1, material.albedoTextureParam2);
         return RayPayload(vec4(0.0, 0.0, 0.0, gl_HitTNV), emit * vec4(color, 0.0), vec4(0.0), randomSeed);
      }

      case MATERIAL_SMOKE: {
         const vec4 attenuationAndDistance = vec4(Color(hitPoint, normal, texCoord, material), gl_HitTNV);
         const vec3 scatterDirection = RandomUnitVector(randomSeed);
         return RayPayload(attenuationAndDistance, vec4(0.0), vec4(scatterDirection, 1.0), randomSeed);
      }
   }
}
