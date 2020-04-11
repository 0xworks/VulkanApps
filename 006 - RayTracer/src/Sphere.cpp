#include "Sphere.h"
#include "Core.h"

uint32_t Sphere::sm_ShaderHitGroupIndex = ~0;
uint32_t SphereInstance::sm_ModelIndex = ~0;


Sphere::Sphere() : Model("Assets/Models/Sphere.obj", Sphere::sm_ShaderHitGroupIndex) {}


bool Sphere::IsProcedural() const {
   return true;
}


std::array<glm::vec3, 2> Sphere::GetBoundingBox() const {
   return {glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{1.0f, 1.0f, 1.0f}};
}


void Sphere::SetDefaultShaderHitGroupIndex(const uint32_t shaderHitGroupIndex) {
   sm_ShaderHitGroupIndex = shaderHitGroupIndex;
}


SphereInstance::SphereInstance(const glm::vec3& centre, const float radius, const Material& material)
: Instance {
   sm_ModelIndex,
   glm::mat3x4 {
      {radius, 0.0f, 0.0f, centre.x},
      {0.0f, radius, 0.0f, centre.y},
      {0.0f, 0.0f, radius, centre.z},
   },
   material
}
{
   ASSERT(sm_ModelIndex != ~0, "ERROR: Sphere model index has not been set.  You must set the model index (via SetModelIndex()) before instantiating a Sphere");
}


void SphereInstance::SetModelIndex(uint32_t modelIndex) {
   sm_ModelIndex = modelIndex;
}
