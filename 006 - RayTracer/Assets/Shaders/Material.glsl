//
// Shared by C++ application code and glsl shader code.
//

#define MATERIAL_FLATCOLOR    0
#define MATERIAL_LAMBERTIAN   1
#define MATERIAL_METALLIC     2
#define MATERIAL_DIELECTRIC   3
#define MATERIAL_DIFFUSELIGHT 4

struct Material {
   uint type;
   float roughness;
   float refractiveIndex;
   int textureId;
   vec4 textureParam1;
   vec4 textureParam2;
      // note: make sure rest of structure is padded to size of largest data member (here, 16 bytes because textureParma1, and textureParam2 are both 16 bytes)
};