#pragma once

#include "Instance.h"
#include "Model.h"

#include <vector>

class Scene {
public:
   uint32_t AddModel(Model model);
   uint32_t AddInstance(Instance instance);

   const std::vector<Model>& Models() const;
   const std::vector<uint32_t>& ModelVertexOffsets() const;
   const std::vector<uint32_t>& ModelIndexOffsets() const;

   const std::vector<Instance>& Instances() const;

private:
   std::vector<Model> m_Models;                 // unique models
   std::vector<uint32_t> m_ModelVertexOffsets;
   std::vector<Instance> m_Instances;           // instances of models (i.e. tuples of model, transform, texture, material)
   std::vector<uint32_t> m_ModelIndexOffsets;
};
