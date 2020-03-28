//
// Shared by C++ application code and glsl shader code.
//
// WARNING: offsets and size of Vertex datamembers is hard-coded in the .rchit shader (to avoid glsl padding wasting space)
//
struct Vertex {
   vec3 pos;
   vec3 normal;
   vec2 uv;
};
