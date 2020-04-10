#include "Rectangle2D.h"
#include "Core.h"

uint32_t Rectangle2DInstance::sm_ModelIndex = ~0;

Rectangle2D::Rectangle2D() : Model("Assets/Models/Rectangle2D.obj") {}


Rectangle2DInstance::Rectangle2DInstance(const glm::vec3& centre, const glm::vec2& size, const glm::vec3& rotateRadians, const Material& material)
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
               -rotateRadians.y,  // y axis flipped for vulkan
               {0.0f, 1.0f, 0.0f}
            ),
            rotateRadians.z,
            {0.0f, 0.0f, 1.0f}
         ),
         glm::vec3 {size, 1.0}
      )
   ),
   material
} {
   ASSERT(sm_ModelIndex != ~0, "ERROR: Rectangle2D model instance has not been set.  You must set the Rectangle2D model index (via SetModelIndex()) before instantiating a Rectangle2D.");
}


void Rectangle2DInstance::SetModelIndex(uint32_t modelIndex) {
   sm_ModelIndex = modelIndex;
}
