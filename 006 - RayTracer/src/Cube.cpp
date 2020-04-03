#include "Cube.h"
#include "Core.h"

uint32_t CubeInstance::sm_modelIndex = ~0;

Cube::Cube()
   : Model("Assets/Models/cube.obj") {
}


CubeInstance::CubeInstance(glm::vec3 centre, float sideLength, Material material)
: Instance {
      sm_modelIndex,
      glm::mat3x4 {
         {sideLength, 0.0f, 0.0f, centre.x},
         {0.0f, sideLength, 0.0f, centre.y},
         {0.0f, 0.0f, sideLength, centre.z},
      },
      0,
      std::move(material)
   }
{
   ASSERT(sm_modelIndex != ~0, "ERROR: Cube model instance has not been set.  You must set the cube model index (via SetModelIndex()) before instantiating a cube");
}


void CubeInstance::SetModelIndex(uint32_t modelIndex) {
   sm_modelIndex = modelIndex;
}
