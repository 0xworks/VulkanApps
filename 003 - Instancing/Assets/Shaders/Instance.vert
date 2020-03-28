#version 450

// Vertex attributes
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inUV;

// Instanced attributes
layout (location = 4) in vec3 instancePos;
layout (location = 5) in vec3 instanceRotation;
layout (location = 6) in float instanceScale;
layout (location = 7) in int instanceTexIndex;

layout (binding = 0) uniform UBO 
{
   mat4 projection;
   mat4 modelview;
   vec4 lightPos;
   float locRotation;
   float globalRotation;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

void main() 
{
   outColor = inColor;
   outUV = inUV; //vec3(inUV, instanceTexIndex);

   mat3 mx, my, mz;

   // rotate around x
   float s = sin(instanceRotation.x + ubo.locRotation);
   float c = cos(instanceRotation.x + ubo.locRotation);
   
   mx[0] = vec3(c, s, 0.0);
   mx[1] = vec3(-s, c, 0.0);
   mx[2] = vec3(0.0, 0.0, 1.0);

   // rotate around y
   s = sin(instanceRotation.y + ubo.locRotation);
   c = cos(instanceRotation.y + ubo.locRotation);
   
   my[0] = vec3(c, 0.0, s);
   my[1] = vec3(0.0, 1.0, 0.0);
   my[2] = vec3(-s, 0.0, c);
   
   // rot around z
   s = sin(instanceRotation.z + ubo.locRotation);
   c = cos(instanceRotation.z + ubo.locRotation);	

   mz[0] = vec3(1.0, 0.0, 0.0);
   mz[1] = vec3(0.0, c, s);
   mz[2] = vec3(0.0, -s, c);

   mat3 rotMat = mz * my * mx;

   mat4 gRotMat;
   s = sin(instanceRotation.y + ubo.globalRotation);
   c = cos(instanceRotation.y + ubo.globalRotation);
   gRotMat[0] = vec4(c, 0.0, s, 0.0);
   gRotMat[1] = vec4(0.0, 1.0, 0.0, 0.0);
   gRotMat[2] = vec4(-s, 0.0, c, 0.0);
   gRotMat[3] = vec4(0.0, 0.0, 0.0, 1.0);	

   vec4 locPos = vec4(inPos.xyz * rotMat, 1.0);
   vec4 pos = vec4((locPos.xyz * instanceScale) + instancePos, 1.0);

   gl_Position = ubo.projection * ubo.modelview * gRotMat * pos;

   outNormal = mat3(ubo.modelview * gRotMat) * inverse(rotMat) * inNormal;

   pos = ubo.modelview * vec4(inPos.xyz + instancePos, 1.0);
   vec3 lPos = mat3(ubo.modelview) * ubo.lightPos.xyz;
   outLightVec = lPos - pos.xyz;
   outViewVec = -pos.xyz;
}
