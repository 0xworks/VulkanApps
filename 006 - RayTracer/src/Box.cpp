#include "Box.h"
#include "Core.h"

uint32_t Box::sm_ShaderHitGroupIndex = ~0;
uint32_t BoxInstance::sm_ModelIndex = ~0;
uint32_t ProceduralBoxInstance::sm_ModelIndex = ~0;

Box::Box(const bool useProcedural)
: Model {"Assets/Models/Box.obj", useProcedural ? Box::sm_ShaderHitGroupIndex : Model::GetDefaultShaderHitGroupIndex()}
, m_IsProcedural{useProcedural}
{}


bool Box::IsProcedural() const {
   return m_IsProcedural;
}


std::array<glm::vec3, 2> Box::GetBoundingBox() const {
   return {glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec3{0.5f, 0.5f, 0.5f}};
}


void Box::SetDefaultShaderHitGroupIndex(const uint32_t shaderHitGroupIndex) {
   sm_ShaderHitGroupIndex = shaderHitGroupIndex;
}


BoxInstance::BoxInstance(const glm::vec3& centre, const glm::vec3& size, const glm::vec3& rotateRadians, const Material& material)
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
         size
      )
   ),
   material
}
{
   ASSERT(sm_ModelIndex != ~0, "ERROR: Box model index has not been set.  You must set the model index (via SetModelIndex()) before instantiating a Box.");
}


void BoxInstance::SetModelIndex(uint32_t modelIndex) {
   sm_ModelIndex = modelIndex;
}

ProceduralBoxInstance::ProceduralBoxInstance(const glm::vec3& centre, const glm::vec3& size, const glm::vec3& rotateRadians, const Material& material)
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
         size
      )
   ),
   material
} {
   ASSERT(sm_ModelIndex != ~0, "ERROR: ProceduralBox model index has not been set.  You must set the model index (via SetModelIndex()) before instantiating a ProceduralBox.");
}


void ProceduralBoxInstance::SetModelIndex(uint32_t modelIndex) {
   sm_ModelIndex = modelIndex;
}
