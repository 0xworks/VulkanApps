//
// Shared by C++ application code and glsl shader code.
//

#define MATERIAL_LAMBERTIAN   0
#define MATERIAL_PHONG        1
#define MATERIAL_METALLIC     2
#define MATERIAL_DIELECTRIC   3
#define MATERIAL_LIGHT        4
#define MATERIAL_SMOKE        5

// Be careful with alignment...
struct Material {
   uint type;
   float materialParameter1;
   float materialParameter2;
   float materialParameter3;

   int materialParameter4;  // these are just padding, really
   int materialParameter5;  //
   int diffuseTextureType;
   int specularTextureType;

   vec4 diffuseTextureParam1;
   vec4 diffuseTextureParam2;
   vec4 specularTextureParam1;
   vec4 specularTextureParam2;
};