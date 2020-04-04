#include "Cube.h"
#include "Core.h"

uint32_t CubeInstance::sm_ModelIndex = ~0;

Cube::Cube()
   : Model("Assets/Models/cube.obj") {
}


CubeInstance::CubeInstance(glm::vec3 centre, float sideLength, float rotateXRadians, float rotateYRadians, float rotateZRadians, Material material)
: Instance {
      sm_ModelIndex,
      glm::mat3x4{glm::translate(glm::scale(glm::rotate(glm::rotate(glm::rotate(glm::identity<glm::mat4x4>(), rotateXRadians, {1.0f, 0.0f, 0.0f}), rotateYRadians, {0.0f, 1.0f, 0.0f}), rotateZRadians, {0.0f, 0.0f, 1.0f}), {sideLength, sideLength, sideLength}), centre)},
      0,
      std::move(material)
   }
{
   ASSERT(sm_ModelIndex != ~0, "ERROR: Cube model instance has not been set.  You must set the cube model index (via SetModelIndex()) before instantiating a cube.");
}


void CubeInstance::SetModelIndex(uint32_t modelIndex) {
   sm_ModelIndex = modelIndex;
}
