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


void Scene::SetSkybox(const std::string& filename) {
   m_SkyboxTextureName = filename;
}


const std::string& Scene::GetSkyboxTextureFileName() const {
   return m_SkyboxTextureName;
}


bool Scene::GetAccumulateFrames() const {
   return m_AccumulateFrames;
}


void Scene::SetAccumulateFrames(const bool b) {
   m_AccumulateFrames = b;
}


uint32_t
Scene::AddModel(std::unique_ptr<Model> model) {
   m_Models.emplace_back(std::move(model));
   return static_cast<uint32_t>(m_Models.size() - 1);
}


uint32_t Scene::AddTextureResource(std::string name, std::string fileName) {
   m_TextureNames.emplace_back(std::move(name));
   m_TextureFileNames.emplace_back(std::move(fileName));
   return static_cast<uint32_t>(m_TextureNames.size());
}


uint32_t
Scene::AddInstance(std::unique_ptr<Instance> instance) {
   m_Instances.emplace_back(std::move(instance));
   return static_cast<uint32_t>(m_Instances.size() - 1);
}


const std::vector<std::unique_ptr<Model>>& Scene::GetModels() const {
   return m_Models;
}


const std::vector<std::string>& Scene::GetTextureFileNames() const {
   return m_TextureFileNames;
}


int Scene::GetTextureId(const std::string& name) const {
   int i = 0;
   for (const auto& existingName : m_TextureNames) {
      if (existingName == name) {
         return i;
      }
      ++i;
   }
   ASSERT(false, "ERROR: Texture '{}' not found", name);
   return ~0;
}


const std::vector<std::unique_ptr<Instance>>& Scene::GetInstances() const {
   return m_Instances;
}
