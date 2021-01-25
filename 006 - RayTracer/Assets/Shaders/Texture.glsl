// Shared by C++ application code and glsl shader code.
// Keeps index numbers in synch

// positive numbers represent indexes into a texture array that is uploaded to the GPU
// negative numbers (such as those defined here) are procedural textures
// the number defines which procedure (in Scatter.glsl) is used

#define TEXTURE_FLATCOLOR        -1
#define TEXTURE_CHECKERBOARD     -2
#define TEXTURE_SIMPLEX3D        -3
#define TEXTURE_TURBULENCE       -4
#define TEXTURE_MARBLE           -5
#define TEXTURE_NORMALS          -100
#define TEXTURE_UV               -101
#define TEXTURE_RED              -102
