#include "Scene.h"

uint32_t
Scene::AddModel(Model model) {
   m_ModelVertexOffsets.emplace_back(m_Models.size() == 0 ? 0 : m_ModelVertexOffsets.back() + static_cast<uint32_t>(m_Models.back().Vertices().size()));
   m_ModelIndexOffsets.emplace_back(m_Models.size() == 0 ? 0 : m_ModelIndexOffsets.back() + static_cast<uint32_t>(m_Models.back().Indices().size()));
   m_Models.emplace_back(model);
   return static_cast<uint32_t>(m_Models.size());
}


uint32_t
Scene::AddInstance(Instance instance) {
   m_Instances.emplace_back(instance);
   return static_cast<uint32_t>(m_Instances.size());
}


const std::vector<Model>& Scene::Models() const {
   return m_Models;
}


const std::vector<uint32_t>& Scene::ModelVertexOffsets() const {
   return m_ModelVertexOffsets;
}


const std::vector<uint32_t>& Scene::ModelIndexOffsets() const {
   return m_ModelIndexOffsets;
}


const std::vector<Instance>& Scene::Instances() const {
   return m_Instances;
}
