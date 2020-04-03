#include "Material.glsl"
#include "Random.glsl"
#include "RayPayload.glsl"

layout(set = 0, binding = BINDING_MATERIALBUFFER) readonly buffer MaterialArray { Material materials[]; };

float Schlick(float cosine, float refractiveIndex) {
    float r0 = (1 - refractiveIndex) / (1 + refractiveIndex);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}


RayPayload Scatter(const vec3 direction, const vec3 hitPoint, const vec3 normal, const uint materialIndex, uint randomSeed) {
   Material material = materials[materialIndex];

   // debugging: for checking normals, can return normal as color...and then look at resulting image
   //vec3 color = 0.5 * (vec3(1) + normal);
   //return RayPayload(vec4(color, gl_HitTNV), vec4(0), randomSeed);

   switch(material.type) {
      case MATERIAL_LAMBERTIAN: {
         const vec4 colorAndDistance = vec4(material.albedo.rgb, gl_HitTNV);
         const vec3 target = hitPoint + normal + RandomInUnitSphere(randomSeed);
         const vec4 scatterDirection = vec4(target - hitPoint, 1);
         return RayPayload(colorAndDistance, scatterDirection, randomSeed);
      }
      case MATERIAL_METALLIC: {
         const vec4 colorAndDistance = vec4(material.albedo.rgb, gl_HitTNV);
         const vec4 scatterDirection = vec4(reflect(direction, normal) + material.roughness * RandomInUnitSphere(randomSeed), 1);
         return RayPayload(colorAndDistance, scatterDirection, randomSeed);
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

