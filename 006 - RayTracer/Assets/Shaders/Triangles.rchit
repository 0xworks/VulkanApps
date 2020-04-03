#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.glsl"
#include "Offset.glsl"
#include "Scatter.glsl"
#include "UniformBufferObject.glsl"
#include "Vertex.glsl"

layout(set = 0, binding = BINDING_VERTEXBUFFER) readonly buffer VertexArray { float vertices[]; };  // not { Vertex Vertices[]; } because glsl structure padding makes it a bit tricky
layout(set = 0, binding = BINDING_INDEXBUFFER) readonly buffer IndexArray { uint indices[]; };
layout(set = 0, binding = BINDING_OFFSETBUFFER) readonly buffer OffsetArray { Offset offsets[]; };

hitAttributeNV vec2 hit;
rayPayloadInNV RayPayload ray;

Vertex UnpackVertex(uint index)
{
   const uint vertexSize = 8;
   const uint offset = index * vertexSize;

   Vertex v;
   v.pos = vec3(vertices[offset + 0], vertices[offset + 1], vertices[offset + 2]);
   v.normal = vec3(vertices[offset + 3], vertices[offset + 4], vertices[offset + 5]);

   return v;
}


void main() {
   ivec3 triangle = ivec3(gl_PrimitiveID * 3 + 0, gl_PrimitiveID * 3 + 1, gl_PrimitiveID * 3 + 2);
   Offset offset = offsets[gl_InstanceCustomIndexNV];
   const Vertex v0 = UnpackVertex(offset.vertexOffset + indices[offset.indexOffset + triangle.x]);
   const Vertex v1 = UnpackVertex(offset.vertexOffset + indices[offset.indexOffset + triangle.y]);
   const Vertex v2 = UnpackVertex(offset.vertexOffset + indices[offset.indexOffset + triangle.z]);

   const vec3 barycentricCoords = vec3(1.0f - hit.x - hit.y, hit.x, hit.y);
   const vec3 hitPoint = v0.pos * barycentricCoords.x + v1.pos * barycentricCoords.y + v2.pos * barycentricCoords.z;
   const vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);

   ray = Scatter(gl_WorldRayDirectionNV, hitPoint, normal, gl_InstanceCustomIndexNV, ray.randomSeed);
}
