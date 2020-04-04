#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.glsl"
#include "Scatter.glsl"

layout(set = 0, binding = BINDING_SPHEREBUFFER) readonly buffer SphereArray { vec4 spheres[]; };

hitAttributeNV vec4 unused; // you must declare a hitAttributeNV otherwise the shader does not work properly!

rayPayloadInNV RayPayload ray;

vec2 GetSphereTexCoord(const vec3 point)
{
	const float phi = atan(point.x, point.z);
	const float theta = asin(point.y);
	const float pi = 3.1415926535897932384626433832795;

	return vec2
	(
		(phi + pi) / (2* pi),
		1 - (theta + pi /2) / pi
	);
}


void main()
{
	// Compute the ray hit point and normal.
	// These calcs are already in world coordinates, no need to transform.
	const vec4 sphere = spheres[gl_InstanceCustomIndexNV];
	const vec3 centre = sphere.xyz;
	const float radius = sphere.w;
	const vec3 hitPoint = gl_WorldRayOriginNV + gl_HitTNV * gl_WorldRayDirectionNV;
	const vec3 normal = (hitPoint - centre) / radius;

	const vec2 texCoord = GetSphereTexCoord(normal);

   ray = Scatter(gl_WorldRayDirectionNV, hitPoint, normal, texCoord, gl_InstanceCustomIndexNV, ray.randomSeed);
}
