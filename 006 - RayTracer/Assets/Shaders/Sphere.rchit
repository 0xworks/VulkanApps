#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.glsl"
#include "Scatter.glsl"

hitAttributeNV uint unused; // you must declare a hitAttributeNV otherwise the shader does not work properly!

rayPayloadInNV RayPayload ray;

vec2 GetSphereTexCoord(const vec3 point) {
   const float phi = atan(point.x, point.z);
   const float theta = asin(point.y);
   const float pi = 3.1415926535897932384626433832795;

   return vec2(
      (phi + pi) / (2* pi),
      1 - (theta + pi /2) / pi
   );
}


void main() {
   // Compute the ray hit point and normal.
   const vec3 centre = vec3(0.0);
   const float radius = 1.0;

   const vec3 hitPoint = gl_ObjectRayOriginNV + gl_HitTNV * gl_ObjectRayDirectionNV;
   const vec3 normal = normalize((hitPoint - centre) / radius); // note. hitPoint is not necessarily on surface of sphere (e.g. if material is smoke)

   const vec2 texCoord = GetSphereTexCoord(normal);

   vec3 hitPointW = gl_ObjectToWorldNV * vec4(hitPoint, 1);
   vec3 normalW = normalize(gl_ObjectToWorldNV * vec4(normal, 0));
   // texCoords dont need transforming

   ray = Scatter(hitPointW, normalW, texCoord, gl_InstanceCustomIndexNV, ray.randomSeed);
}
