#pragma once

#include "Model.h"
#include "Instance.h"

class Box : public Model {
public:
   Box();

   bool IsProcedural() const override;

   std::array<glm::vec3, 2> GetBoundingBox() const override;

public:
   static void SetShaderHitGroupIndex(const uint32_t shaderHitGroupIndex);

private:
   static uint32_t sm_ShaderHitGroupIndex;
};


class BoxInstance : public Instance {
public:
   BoxInstance(const glm::vec3& centre, const glm::vec3& size, const glm::vec3& rotationRadians, const Material& material);

public:
   static void SetModelIndex(uint32_t modelIndex);

private:
   static uint32_t sm_ModelIndex;
};
