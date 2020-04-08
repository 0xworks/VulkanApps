//
// Shared by C++ application code and glsl shader code.
//
// Data members of UniformBufferObject must be aligned per glsl rules
// Here, means to 16 bytes (because mat4)
struct UniformBufferObject {
   mat4 viewInverse;
   mat4 projInverse;
   vec4 horizonColor;
   vec4 zenithColor;
   uint accumulatedFrameCount;
};
