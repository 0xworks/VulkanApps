#include "Scene.h"

uint32_t
Scene::AddModel(std::unique_ptr<Model> model) {
   m_Models.emplace_back(std::move(model));
   return static_cast<uint32_t>(m_Models.size() - 1);
}


uint32_t
Scene::AddInstance(std::unique_ptr<Instance> instance) {
   m_Instances.emplace_back(std::move(instance));
   return static_cast<uint32_t>(m_Instances.size() - 1);
}


const std::vector<std::unique_ptr<Model>>& Scene::Models() const {
   return m_Models;
}


const std::vector<std::unique_ptr<Instance>>& Scene::Instances() const {
   return m_Instances;
}
