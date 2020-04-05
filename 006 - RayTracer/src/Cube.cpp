#include "Cube.h"
#include "Core.h"

uint32_t CubeInstance::sm_ModelIndex = ~0;

Cube::Cube() : Model("Assets/Models/cube.obj") {}


CubeInstance::CubeInstance(glm::vec3 centre, float sideLength, glm::vec3 rotateRadians, Material material)
: Instance {
      sm_ModelIndex,
      glm::transpose(
         glm::scale(
            glm::rotate(
               glm::rotate(
                  glm::rotate(
                     glm::translate(glm::identity<glm::mat4x4>(), centre),
                     rotateRadians.x,
                     {1.0f, 0.0f, 0.0f}
                  ),
                  rotateRadians.y,
                  {0.0f, 1.0f, 0.0f}
               ),
               rotateRadians.z,
               {0.0f, 0.0f, 1.0f}
            ),
            glm::vec3 {sideLength}
         )
      ),
      std::move(material)
   }
{
   ASSERT(sm_ModelIndex != ~0, "ERROR: Cube model instance has not been set.  You must set the cube model index (via SetModelIndex()) before instantiating a cube.");
}


void CubeInstance::SetModelIndex(uint32_t modelIndex) {
   sm_ModelIndex = modelIndex;
}
