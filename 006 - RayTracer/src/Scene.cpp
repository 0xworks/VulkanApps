#include "Scene.h"

uint32_t
Scene::AddModel(Model model) {
   m_Models.emplace_back(model);
   return static_cast<uint32_t>(m_Models.size() - 1);
}


uint32_t
Scene::AddInstance(Instance instance) {
   m_Instances.emplace_back(instance);
   return static_cast<uint32_t>(m_Instances.size() - 1);
}


const std::vector<Model>& Scene::Models() const {
   return m_Models;
}


const std::vector<Instance>& Scene::Instances() const {
   return m_Instances;
}
