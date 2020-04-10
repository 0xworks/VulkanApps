#include "Instance.h"

Instance::Instance(const uint32_t modelIndex, const glm::mat3x4& transform, const Material& material)
: m_ModelIndex(modelIndex)
, m_Transform(transform)
, m_Material(material)
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
