#pragma once

#include "Material.h"

class Instance {
public:
   Instance(uint32_t modelIndex, glm::mat3x4 transform, Material material);

   uint32_t GetModelIndex() const;

   const glm::mat3x4& GetTransform() const;

   const Material& GetMaterial() const;

private:
   uint32_t m_ModelIndex;
   glm::mat3x4 m_Transform;
   Material m_Material;
};
