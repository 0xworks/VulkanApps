#include "Sphere.h"
#include "Core.h"

uint32_t SphereInstance::sm_modelIndex = ~0;

Sphere::Sphere()
: Model("Assets/Models/sphere.obj")
{}


bool Sphere::IsTriangles() const {
   return false;
}


std::array<glm::vec3, 2> Sphere::BoundingBox() const {
   return {glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{1.0f, 1.0f, 1.0f}};
}


SphereInstance::SphereInstance(glm::vec3 centre, float radius, Material material)
: Instance {
      sm_modelIndex,
      glm::mat3x4 {
         {radius, 0.0f, 0.0f, centre.x},
         {0.0f, radius, 0.0f, centre.y},
         {0.0f, 0.0f, radius, centre.z},
      },
      0,
      std::move(material)
   }
{
   ASSERT(sm_modelIndex != ~0, "ERROR: Sphere model instance has not been set.  You must set the sphere model index (via SetModelIndex()) before instantiating a sphere");
}


void SphereInstance::SetModelIndex(uint32_t modelIndex) {
   sm_modelIndex = modelIndex;
}
