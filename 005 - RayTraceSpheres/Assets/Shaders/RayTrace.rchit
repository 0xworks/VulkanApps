#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.h"
#include "Material.h"
#include "Random.glsl"
#include "RayPayload.glsl"
#include "UniformBufferObject.h"
#include "Vertex.h"

layout(set = 0, binding = BINDING_VERTEXBUFFER) readonly buffer VertexArray { float vertices[]; };  // not { Vertex Vertices[]; } because glsl structure padding makes it a bit tricky
layout(set = 0, binding = BINDING_INDEXBUFFER) readonly buffer IndexArray { uint indices[]; };
layout(set = 0, binding = BINDING_MATERIALBUFFER) readonly buffer MaterialArray { Material materials[]; };

hitAttributeNV vec2 hit;
rayPayloadInNV RayPayload ray;

Vertex UnpackVertex(uint index)
{
   const uint vertexSize = 6;
   const uint offset = index * vertexSize;

   Vertex v;
   v.pos = vec3(vertices[offset + 0], vertices[offset + 1], vertices[offset + 2]);
   v.normal = vec3(vertices[offset + 3], vertices[offset + 4], vertices[offset + 5]);

   return v;
}


float Schlick(float cosine, float refractiveIndex) {
    float r0 = (1 - refractiveIndex) / (1 + refractiveIndex);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}


RayPayload Scatter(const vec3 direction, const vec3 hitPoint, const vec3 normal, const uint materialIndex, uint randomSeed) {
   switch(materials[materialIndex].type) {
      case MATERIAL_LAMBERTIAN: {
         const vec4 colorAndDistance = vec4(materials[materialIndex].albedo.rgb, gl_RayTmaxNV);
         const vec3 target = hitPoint + normal + RandomInUnitSphere(randomSeed);
         const vec4 scatterDirection = vec4(target - hitPoint, 1);
         return RayPayload(colorAndDistance, scatterDirection, randomSeed);
      }
      case MATERIAL_METALLIC: {
         const vec4 colorAndDistance = vec4(materials[materialIndex].albedo.rgb, gl_RayTmaxNV);
         const vec4 scatterDirection = vec4(reflect(direction, normal) + materials[materialIndex].roughness * RandomInUnitSphere(randomSeed), 1);
         return RayPayload(colorAndDistance, scatterDirection, randomSeed);
      }
      case MATERIAL_DIELECTRIC: {
         vec3 outward_normal;
         float ni_over_nt;
         float reflect_prob;
         float cosine;
         if (dot(direction, normal) > 0) {
            outward_normal = -normal;
            ni_over_nt = materials[materialIndex].refractiveIndex;
            cosine = ni_over_nt * dot(direction, normal) / length(direction);
         } else {
            outward_normal = normal;
            ni_over_nt = 1.0 / materials[materialIndex].refractiveIndex;
            cosine = -dot(direction, normal) / length(direction);
         }
         const vec3 refracted = refract(direction, outward_normal, ni_over_nt);
         const vec4 colorAndDistance = vec4(1.0, 1.0, 1.0, gl_RayTmaxNV);
         if(dot(refracted, refracted) > 0) {
            reflect_prob = Schlick(cosine, materials[materialIndex].refractiveIndex);
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


void main() {
   ivec3 triangle = ivec3(gl_PrimitiveID * 3 + 0, gl_PrimitiveID * 3 + 1, gl_PrimitiveID * 3 + 2);
   const Vertex v0 = UnpackVertex(indices[triangle.x]);
   const Vertex v1 = UnpackVertex(indices[triangle.y]);
   const Vertex v2 = UnpackVertex(indices[triangle.z]);

   const vec3 barycentricCoords = vec3(1.0f - hit.x - hit.y, hit.x, hit.y);
   const vec3 hitPoint = v0.pos * barycentricCoords.x + v1.pos * barycentricCoords.y + v2.pos * barycentricCoords.z;
   const vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);

   ray = Scatter(gl_WorldRayDirectionNV, hitPoint, normal, gl_InstanceID, ray.randomSeed);
}
