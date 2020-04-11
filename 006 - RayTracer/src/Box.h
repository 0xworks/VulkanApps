#pragma once

#include "Model.h"
#include "Instance.h"

class Box : public Model {
public:

   // Procedural box is much slower than using model triangles
   // Only reason to use it is if you want smoke material.
   Box(const bool useProcedural);

   bool IsProcedural() const override;

   std::array<glm::vec3, 2> GetBoundingBox() const override;

public:
   static void SetDefaultShaderHitGroupIndex(const uint32_t shaderHitGroupIndex);

private:
   static uint32_t sm_ShaderHitGroupIndex;

   bool m_IsProcedural;
};


class BoxInstance : public Instance {
public:
   BoxInstance(const glm::vec3& centre, const glm::vec3& size, const glm::vec3& rotationRadians, const Material& material);

public:
   static void SetModelIndex(uint32_t modelIndex);

private:
   static uint32_t sm_ModelIndex;
};


class ProceduralBoxInstance : public Instance {
public:
   ProceduralBoxInstance(const glm::vec3& centre, const glm::vec3& size, const glm::vec3& rotationRadians, const Material& material);

public:
   static void SetModelIndex(uint32_t modelIndex);

private:
   static uint32_t sm_ModelIndex;
};
