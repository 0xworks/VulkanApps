//
// Shared by C++ application code and glsl shader code.
//

#define MATERIAL_LAMBERTIAN   0
#define MATERIAL_METALLIC     1
#define MATERIAL_DIELECTRIC   2
#define MATERIAL_DIFFUSELIGHT 3
#define MATERIAL_SMOKE        4

// Be careful with alignment...
struct Material {
   uint type;
   float materialParameter1;
   float materialParameter2;
   int textureId;
   vec4 textureParam1;
   vec4 textureParam2;
};