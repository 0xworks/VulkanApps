//
// Shared by C++ application code and glsl shader code.
//

#define MATERIAL_LAMBERTIAN 1
#define MATERIAL_METALLIC 2
#define MATERIAL_DIELECTRIC 3

struct Material {
   vec4 albedo; // note vec3 and vec4 both take up 16 bytes in glsl
   uint type;
   float roughness;
   float refractiveIndex;
   uint pad;   // need to pad rest of structure to size of largest data member (16 bytes)
};