#include "Instance.h"

Instance::Instance(uint32_t modelIndex, glm::mat3x4 transform, Material material)
: m_ModelIndex(modelIndex)
, m_Transform(std::move(transform))
, m_Material(std::move(material))
{}


uint32_t Instance::GetModelIndex() const {
   return m_ModelIndex;
}


const glm::mat3x4& Instance::GetTransform() const {
   return m_Transform;
}


const Material& Instance::GetMaterial() const {
   return m_Material;
}
