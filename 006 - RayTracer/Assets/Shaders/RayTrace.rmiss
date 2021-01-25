#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.glsl"
#include "RayPayload.glsl"
#include "UniformBufferObject.glsl"

layout(set = 0, binding = BINDING_UNIFORMBUFFER) readonly uniform UBO {
   UniformBufferObject ubo;
};

layout(set = 0, binding = BINDING_SKYBOX) uniform samplerCube skybox;

layout(location = 0) rayPayloadInNV RayPayload ray;


void main() {
   const float t = clamp(normalize(gl_WorldRayDirectionNV).y, 0.0, 1.0);
   ray.attenuationAndDistance = vec4(vec3(1.0), -1.0);
   if(ubo.useSkybox == 1) {
      ray.emission = texture(skybox, normalize(gl_WorldRayDirectionNV).xyz * vec3(1.0, -1.0, 1.0));
   } else {
      ray.emission = mix(ubo.horizonColor, ubo.zenithColor, t);
   }
   ray.scatterDirection = vec4(0.0);
}
