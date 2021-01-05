#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.glsl"
#include "Offset.glsl"
#include "Scatter.glsl"
#include "UniformBufferObject.glsl"
#include "Vertex.glsl"

layout(binding = BINDING_VERTEXBUFFER) readonly buffer VertexArray { float vertices[]; };  // not { Vertex Vertices[]; } because glsl structure padding makes it a bit tricky
layout(binding = BINDING_INDEXBUFFER) readonly buffer IndexArray { uint indices[]; };
layout(binding = BINDING_OFFSETBUFFER) readonly buffer OffsetArray { Offset offsets[]; };

hitAttributeNV vec2 hit;
rayPayloadInNV RayPayload ray;

Vertex UnpackVertex(uint index) {
   const uint vertexSize = 8;
   const uint offset = index * vertexSize;

   Vertex v;
   v.pos = vec3(vertices[offset + 0], vertices[offset + 1], vertices[offset + 2]);
   v.normal = vec3(vertices[offset + 3], vertices[offset + 4], vertices[offset + 5]);
   v.uv = vec2(vertices[offset + 6], vertices[offset + 7]);

   return v;
}


void main() {
   ivec3 triangle = ivec3(gl_PrimitiveID * 3 + 0, gl_PrimitiveID * 3 + 1, gl_PrimitiveID * 3 + 2);
   Offset offset = offsets[gl_InstanceCustomIndexNV];
   const Vertex v0 = UnpackVertex(offset.vertexOffset + indices[offset.indexOffset + triangle.x]);
   const Vertex v1 = UnpackVertex(offset.vertexOffset + indices[offset.indexOffset + triangle.y]);
   const Vertex v2 = UnpackVertex(offset.vertexOffset + indices[offset.indexOffset + triangle.z]);

   const vec3 barycentric = vec3(1.0f - hit.x - hit.y, hit.x, hit.y);

   const vec3 hitPoint = v0.pos * barycentric.x + v1.pos * barycentric.y + v2.pos * barycentric.z;
   const vec3 normal = normalize(v0.normal * barycentric.x + v1.normal * barycentric.y + v2.normal * barycentric.z);
   const vec2 texCoord = v0.uv * barycentric.x + v1.uv * barycentric.y + v2.uv * barycentric.z;

   // Transform hitPoint and normal to world coords for "scatter" function (texCoords should not be transformed)
   vec3 hitPointW = gl_ObjectToWorldNV * vec4(hitPoint, 1);
   vec3 normalW = normalize(gl_ObjectToWorldNV * vec4(normal, 0));

   ray = Scatter(hitPointW, normalW, texCoord, gl_InstanceCustomIndexNV, ray.randomSeed);
}
