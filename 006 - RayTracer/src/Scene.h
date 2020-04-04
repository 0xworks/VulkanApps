#pragma once

#include "Instance.h"
#include "Model.h"

#include <vector>

class Scene {
public:
   uint32_t AddModel(std::unique_ptr<Model> model);
   uint32_t AddInstance(std::unique_ptr<Instance> instance);

   const std::vector<std::unique_ptr<Model>>& GetModels() const;
   const std::vector<std::unique_ptr<Instance>>& GetInstances() const;

private:
   std::vector<std::unique_ptr<Model>> m_Models;                 // unique models
   std::vector<std::unique_ptr<Instance>> m_Instances;           // instances of models (i.e. tuples of model, transform, texture, material)
};
