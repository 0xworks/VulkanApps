//
// Shared by C++ application code and glsl shader code.
//

#define MATERIAL_LAMBERTIAN   0
#define MATERIAL_BLENDED      1
#define MATERIAL_METALLIC     2
#define MATERIAL_DIELECTRIC   3
#define MATERIAL_DIFFUSELIGHT 4
#define MATERIAL_SMOKE        5

// Be careful with alignment...
struct Material {
   uint type;
   float materialParameter1;
   float materialParameter2;
   int textureId;
   vec4 textureParam1;
   vec4 textureParam2;
};