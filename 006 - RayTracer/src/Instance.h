#pragma once

#include "Material.h"

class Instance {
public:
   Instance(const uint32_t modelIndex, const glm::mat3x4& transform, const Material& material);

   uint32_t GetModelIndex() const;

   const glm::mat3x4& GetTransform() const;

   const Material& GetMaterial() const;

private:
   uint32_t m_ModelIndex;
   glm::mat3x4 m_Transform;
   Material m_Material;
};
