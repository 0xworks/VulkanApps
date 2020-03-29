#pragma once

#include "Instance.h"
#include "Model.h"

#include <vector>

class Scene {
public:
   uint32_t AddModel(Model model);
   uint32_t AddInstance(Instance instance);

   const std::vector<Model>& Models() const;
   const std::vector<Instance>& Instances() const;

private:
   std::vector<Model> m_Models;                 // unique models
   std::vector<Instance> m_Instances;           // instances of models (i.e. tuples of model, transform, texture, material)
};
