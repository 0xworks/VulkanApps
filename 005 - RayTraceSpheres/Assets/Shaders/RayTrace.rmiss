#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "RayPayload.glsl"

layout(location = 0) rayPayloadInNV RayPayload ray;


void main() {
   // Sky color
   const float t = 0.5 * (normalize(gl_WorldRayDirectionNV).y + 1);
   const vec3 skyColor = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t);
   ray.colorAndDistance = vec4(skyColor, -1);
   ray.scatterDirection = vec4(0.0);
}
