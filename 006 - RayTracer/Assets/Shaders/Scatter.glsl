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


vec3 Color(const vec3 hitPoint, const vec3 normal, const vec2 texCoord, const int textureType, const vec4 textureParam1, const vec4 textureParam2) {
   switch(textureType) {
      case TEXTURE_FLATCOLOR: {
         // flat color
         return textureParam1.rgb;
      }

      case TEXTURE_CHECKERBOARD: {
         // checkboard
         // WARNING: This texture doesnt work too well if you have a large axis-aligned face at some value where sin(value) is exactly 0
         //          (For example, a large cube where one of the faces is axis-aligned at z = 0)
         //          You will end up with floating point precision issues with sin(hitPoint.z) sometimes being slightly less than 0 and sometimes
         //          slightly > 0.
         const vec3 oddColor = textureParam1.rgb;
         const vec3 evenColor = textureParam2.rgb;
         const float scale = textureParam1.w;
         float sineProduct = (sin(hitPoint.x * scale) >= 0.0? 1.0 : -1.0) * (sin(hitPoint.y * scale) >= 0.0? 1.0 : -1.0) * (sin(hitPoint.z * scale) >= 0.0? 1.0 : -1.0);
         if (sineProduct < 0) {
            return oddColor;
         } else {
            return evenColor;
         }
      }

      case TEXTURE_SIMPLEX3D: {
         vec3 p = hitPoint;
         p *= textureParam2.w; // scale
         return mix(textureParam1.rgb, vec3(snoise(p)), textureParam1.w);
      }

      case TEXTURE_TURBULENCE: {
         vec3 p = hitPoint;
         p *= textureParam2.w; // scale
         return mix(textureParam1.rgb, vec3(Turbulence(p, int(textureParam2.z))), textureParam1.w);   
      }

      case TEXTURE_MARBLE: {
         vec3 p = hitPoint;
         p *= textureParam2.w; // scale
         return mix(textureParam1.rgb, vec3(sin(p.z + 10.0 * Turbulence(p, int(textureParam2.z)))), textureParam1.w);
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
         // sample from textures, indexed by textureType
         // param1 has texture offset in xy, and texture scale in zw
         return texture(samplers[textureType], textureParam1.xy + (texCoord * textureParam1.zw)).rgb;
      }
   }
}


RayPayload ScatterLambertian(const vec3 hitPoint, const vec3 normal, const vec3 color, inout uint randomSeed) {
   //
   // sample a scatter direction.
   //
   // color = BRDF * dot(normal, scatter direction) / probability of choosing scatter direction
   //
   // So, lambertian, with uniform sampling on hemisphere:
   // color = BRDF   *  dot(normal, scatter direction)  /  probability of choosing scatter direction
   //       = kd/pi  *  dot(normal, scatter direction)  /  (1/2pi)
   //       = 2kd * dot(normal, scatter direction)
   //
   // Or, lambertian, with cosine weighted sampling on hemisphere
   // color = BRDF   *  dot(normal, scatter direction) /  probability of choosing scatter direction
   // color = kd/pi  *  dot(normal, scatter direction) /  (dot(normal, scatter direction)/pi)
   //       = kd
   //
   // Or, lambertian, with importance sampling towards an object
   // color = BRDF   *  dot(normal, scatter direction)  /  probability of choosing scatter direction
   // color = kd/pi  *  dot(normal, scatter direction)  /  probability of choosing scatter direction
   //
   // Here we are returning the color for Lambertian with cosine weighted sampling, which is just Kd, irrespective of scatter direction
   const vec3 scatterDirection = RandomOnUnitHemisphere(normal, 1.0, randomSeed);
   return RayPayload(vec4(color, gl_HitTNV), vec4(0.0), vec4(scatterDirection, 1.0), randomSeed);
}


RayPayload ScatterMetallic(const vec3 hitPoint, const vec3 normal, const vec3 color, const float roughness, inout uint randomSeed) {
   const vec3 scatterDirection = normalize(reflect(gl_WorldRayDirectionNV, normal) + roughness * RandomInUnitSphere(randomSeed));
   return RayPayload(vec4(color, gl_HitTNV), vec4(0.0), vec4(scatterDirection, 1.0), randomSeed);
}


RayPayload Scatter(const vec3 hitPoint, const vec3 normal, const vec2 texCoord, const uint materialIndex, inout uint randomSeed) {
   Material material = materials[materialIndex];

   switch(material.type) {

      case MATERIAL_LAMBERTIAN: {
         return ScatterLambertian(hitPoint, normal, Color(hitPoint, normal, texCoord, material.diffuseTextureType, material.diffuseTextureParam1, material.diffuseTextureParam2), randomSeed);
      }

      case MATERIAL_PHONG: {
         const vec3 specular = Color(hitPoint, normal, texCoord, material.specularTextureType, material.specularTextureParam1, material.specularTextureParam2);
         const vec3 diffuse = min(1.0 - specular, Color(hitPoint, normal, texCoord, material.diffuseTextureType, material.diffuseTextureParam1, material.diffuseTextureParam2));

         float specularChance = dot(specular, vec3(1.0 / 3.0));
         float diffuseChance = dot(diffuse, vec3(1.0 / 3.0));
         float sum = specularChance + diffuseChance;
         if(sum > 0.00001) {
            diffuseChance /= sum;
            specularChance /= sum;
         } else {
            diffuseChance = 1.0f;
            specularChance = 0.0f;
         }

         const float select = RandomFloat(randomSeed);
         if (select < specularChance) {
            const float alpha = pow(10000.0f, material.materialParameter1 * material.materialParameter1);
            const vec3 scatterDirection = RandomOnUnitHemisphere(reflect(gl_WorldRayDirectionNV, normal), alpha, randomSeed);
            const float f = (alpha + 2.0) / (alpha + 1.0);
            // note: cannot get here if specularChance is zero, so there is no division by zero.
            return RayPayload(vec4(specular / specularChance * clamp(dot(normal, scatterDirection), 0.0, 1.0) * f, gl_HitTNV), vec4(0.0), vec4(scatterDirection, 1.0), randomSeed);
         } else {
            // note: cannot get here if diffuseChance is zero, so there is no division by zero.
            return ScatterLambertian(hitPoint, normal, diffuse / diffuseChance, randomSeed);
         }
      }

      case MATERIAL_METALLIC: {
         return ScatterMetallic(hitPoint, normal, Color(hitPoint, normal, texCoord, material.specularTextureType, material.specularTextureParam1, material.specularTextureParam2), material.materialParameter1, randomSeed);
      }

      case MATERIAL_DIELECTRIC: {
         vec3 outward_normal;
         float ni_over_nt;
         float reflectProbability;
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
         const vec4 attenuationAndDistance = vec4(Color(hitPoint, normal, texCoord, material.diffuseTextureType, material.diffuseTextureParam1, material.diffuseTextureParam2), gl_HitTNV);

         if(dot(refracted, refracted) > 0.0) {
            reflectProbability = Schlick(cosine, material.materialParameter1);
         } else {
            reflectProbability = 1.0;
         }
         if(RandomFloat(randomSeed) < reflectProbability) {
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
         const vec3 color = Color(hitPoint, normal, texCoord, material.diffuseTextureType, material.diffuseTextureParam1, material.diffuseTextureParam2);
         return RayPayload(vec4(0.0, 0.0, 0.0, gl_HitTNV), emit * vec4(color, 0.0), vec4(0.0), randomSeed);
      }

      case MATERIAL_SMOKE: {
         const vec4 attenuationAndDistance = vec4(Color(hitPoint, normal, texCoord, material.diffuseTextureType, material.diffuseTextureParam1, material.diffuseTextureParam2), gl_HitTNV);
         const vec3 scatterDirection = RandomUnitVector(randomSeed);
         return RayPayload(attenuationAndDistance, vec4(0.0), vec4(scatterDirection, 1.0), randomSeed);
      }
   }
}
