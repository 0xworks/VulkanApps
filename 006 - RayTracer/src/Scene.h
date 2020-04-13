#pragma once

#include "Instance.h"
#include "Model.h"

#include <vector>

class Scene {
public:
   glm::vec3 GetHorizonColor() const;
   void SetHorizonColor(const glm::vec3& color);

   glm::vec3 GetZenithColor() const;
   void SetZenithColor(const glm::vec3& color);

   uint32_t AddModel(std::unique_ptr<Model> model);
   uint32_t AddTextureResource(std::string name, std::string fileName);
   uint32_t AddInstance(std::unique_ptr<Instance> instance);

   const std::vector<std::unique_ptr<Model>>& GetModels() const;
   const std::vector<std::string>& GetTextureFileNames() const;
   int GetTextureId(const std::string& name) const;
   const std::vector<std::unique_ptr<Instance>>& GetInstances() const;

private:
   glm::vec3 m_HorizonColor = glm::one<glm::vec3>();
   glm::vec3 m_ZenithColor = glm::one<glm::vec3>();
   std::vector<std::unique_ptr<Model>> m_Models;                 // unique models
   std::vector<std::string> m_TextureNames;
   std::vector<std::string> m_TextureFileNames;
   std::vector<std::unique_ptr<Instance>> m_Instances;           // instances of models (i.e. tuples of model, transform, texture, material)
};
