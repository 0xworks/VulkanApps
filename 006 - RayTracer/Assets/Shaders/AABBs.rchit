#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.glsl"
#include "Scatter.glsl"

layout(set = 0, binding = BINDING_SPHEREBUFFER) readonly buffer SphereArray { vec4 spheres[]; };

hitAttributeNV vec4 hitSphere;

rayPayloadInNV RayPayload ray;

void main()
{
	// Compute the ray hit point properties.
	const vec4 sphere = spheres[gl_InstanceCustomIndexNV];
	const vec3 centre = sphere.xyz;
	const float radius = sphere.w;
	const vec3 hitPoint = gl_WorldRayOriginNV + gl_HitTNV * gl_WorldRayDirectionNV;
	const vec3 normal = (hitPoint - centre) / radius;

   ray = Scatter(gl_WorldRayDirectionNV, hitPoint, normal, gl_InstanceCustomIndexNV, ray.randomSeed);
}
