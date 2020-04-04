#pragma once

#include "Instance.h"
#include "Model.h"

#include <filesystem>

class Sphere : public Model {
public:
   Sphere();

   bool IsProcedural() const override;

   std::array<glm::vec3, 2> GetBoundingBox() const override;

public:
   static void SetShaderHitGroupIndex(const uint32_t shaderHitGroupIndex);

private:
   static uint32_t sm_ShaderHitGroupIndex;
};


class SphereInstance : public Instance {
public:
   SphereInstance(glm::vec3 centre, float radius, Material material);

public:
   static void SetModelIndex(uint32_t modelIndex);

private:
   static uint32_t sm_ModelIndex;
};
