#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require

#include "Bindings.glsl"
#include "Scatter.glsl"

hitAttributeNV uint unused; // you must declare a hitAttributeNV otherwise the shader does not work properly!

rayPayloadInNV RayPayload ray;


void main() {
   const vec4 normals[6] = {vec4(0.0, 0.0, -1.0, 0.0), vec4(0.0, 0.0, 1.0, 0.0), vec4(0.0, -1.0, 0.0, 0.0), vec4(0.0, 1.0, 0.0, 0.0), vec4(-1.0, 0.0, 0.0, 0.0), vec4(1.0, 0.0, 0.0, 0.0)};

   const vec3 hitPoint = gl_ObjectRayOriginNV + gl_HitTNV * gl_ObjectRayDirectionNV;
   const vec4 normal = normals[gl_HitKindNV];

   vec2 texCoord = vec2(0.0);
   switch(gl_HitKindNV) {
      case 0:
      case 1:
         texCoord = hitPoint.xy;
         break;
      case 2:
      case 3:
         texCoord = hitPoint.xz;
         break;
      case 4:
      case 5:
         texCoord = hitPoint.yz;
         break;
   }

   vec3 hitPointW = gl_ObjectToWorldNV * vec4(hitPoint, 1);
   vec3 normalW = normalize(gl_ObjectToWorldNV * normal);
   // texCoords dont need transforming

   ray = Scatter(hitPointW, normalW, texCoord, gl_InstanceCustomIndexNV, ray.randomSeed);
}
