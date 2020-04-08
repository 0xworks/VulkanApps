#include "Scene.h"

glm::vec3 Scene::GetHorizonColor() const {
   return m_HorizonColor;
}


void Scene::SetHorizonColor(const glm::vec3& color) {
   m_HorizonColor = color;
}


glm::vec3 Scene::GetZenithColor() const {
   return m_ZenithColor;
}


void Scene::SetZenithColor(const glm::vec3& color) {
   m_ZenithColor = color;
}


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


const std::vector<std::unique_ptr<Model>>& Scene::GetModels() const {
   return m_Models;
}


const std::vector<std::unique_ptr<Instance>>& Scene::GetInstances() const {
   return m_Instances;
}
