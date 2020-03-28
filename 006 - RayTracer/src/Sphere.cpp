#include "Sphere.h"

Sphere::Sphere(uint32_t modelIndex, glm::vec3 centre, float radius, Material material)
: Instance {
      modelIndex,
      glm::mat3x4 {
         {radius, 0.0f, 0.0f, centre.x},
         {0.0f, radius, 0.0f, centre.y},
         {0.0f, 0.0f, radius, centre.z},
      },
      0,
      std::move(material)
   }
{}
