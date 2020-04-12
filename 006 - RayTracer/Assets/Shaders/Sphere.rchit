#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.glsl"
#include "Scatter.glsl"

hitAttributeNV uint unused; // you must declare a hitAttributeNV otherwise the shader does not work properly!

rayPayloadInNV RayPayload ray;


void main() {
   const vec3 hitPoint = gl_ObjectRayOriginNV + gl_HitTNV * gl_ObjectRayDirectionNV;
   const vec3 normal = normalize(hitPoint); // note. centre is 0, radius is 1, and hitPoint is not necessarily on surface of sphere (e.g. if material is smoke)

   const float phi = atan(hitPoint.x, hitPoint.z);
   const float theta = asin(hitPoint.y);
   const float pi = 3.1415926535897932384626433832795;

   const vec2 texCoord = vec2((phi + pi) / (2.0 * pi), 1 - (theta + pi / 2.0) / pi);

   vec3 hitPointW = gl_ObjectToWorldNV * vec4(hitPoint, 1.0);
   vec3 normalW = normalize(gl_ObjectToWorldNV * vec4(normal, 0.0));
   // texCoords dont need transforming

   ray = Scatter(hitPointW, normalW, texCoord, gl_InstanceCustomIndexNV, ray.randomSeed);
}
