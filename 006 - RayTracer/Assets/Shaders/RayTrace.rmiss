#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.glsl"
#include "RayPayload.glsl"
#include "UniformBufferObject.glsl"

layout(set = 0, binding = BINDING_UNIFORMBUFFER) readonly uniform UBO {
   UniformBufferObject ubo;
};

layout(location = 0) rayPayloadInNV RayPayload ray;


void main() {
   const float t = clamp(normalize(gl_WorldRayDirectionNV).y, 0.0, 1.0);
   const vec4 skyColor = mix(ubo.horizonColor, ubo.zenithColor, t);
   ray.colorAndDistance = vec4(skyColor.rgb, -1);
   ray.scatterDirection = vec4(0.0);
}
