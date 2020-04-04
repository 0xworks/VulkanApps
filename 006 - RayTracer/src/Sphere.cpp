#include "Sphere.h"
#include "Core.h"

uint32_t Sphere::sm_ShaderHitGroupIndex = ~0;
uint32_t SphereInstance::sm_ModelIndex = ~0;


Sphere::Sphere()
: Model("Assets/Models/sphere.obj")
{}


bool Sphere::IsProcedural() const {
   return false;
}


std::array<glm::vec3, 2> Sphere::GetBoundingBox() const {
   return {glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{1.0f, 1.0f, 1.0f}};
}


void Sphere::SetShaderHitGroupIndex(const uint32_t shaderHitGroupIndex) {
   sm_ShaderHitGroupIndex = shaderHitGroupIndex;
}


SphereInstance::SphereInstance(glm::vec3 centre, float radius, Material material)
: Instance {
      sm_ModelIndex,
      glm::mat3x4 {
         {radius, 0.0f, 0.0f, centre.x},
         {0.0f, radius, 0.0f, centre.y},
         {0.0f, 0.0f, radius, centre.z},
      },
      0,
      std::move(material)
   }
{
   ASSERT(sm_ModelIndex != ~0, "ERROR: Sphere model instance has not been set.  You must set the sphere model index (via SetModelIndex()) before instantiating a sphere");
}


void SphereInstance::SetModelIndex(uint32_t modelIndex) {
   sm_ModelIndex = modelIndex;
}
