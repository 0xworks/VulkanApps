#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "Bindings.glsl"
#include "Scatter.glsl"

hitAttributeEXT uint unused; // you must declare a hitAttributeEXT otherwise the shader does not work properly!

rayPayloadInEXT RayPayload ray;


void main() {
   const vec4 normals[6] = {vec4(0.0, 0.0, -1.0, 0.0), vec4(0.0, 0.0, 1.0, 0.0), vec4(0.0, -1.0, 0.0, 0.0), vec4(0.0, 1.0, 0.0, 0.0), vec4(-1.0, 0.0, 0.0, 0.0), vec4(1.0, 0.0, 0.0, 0.0)};

   const vec3 hitPoint = gl_ObjectRayOriginEXT + gl_HitTEXT * gl_ObjectRayDirectionEXT;
   const vec4 normal = normals[gl_HitKindEXT];

   vec2 texCoord = vec2(0.0);
   switch(gl_HitKindEXT) {
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

   vec3 hitPointW = gl_ObjectToWorldEXT * vec4(hitPoint, 1);
   vec3 normalW = normalize(gl_ObjectToWorldEXT * normal);
   // texCoords dont need transforming

   ray = Scatter(hitPointW, normalW, texCoord, gl_InstanceCustomIndexEXT, ray.randomSeed);
}
