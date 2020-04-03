#pragma once

#include "Instance.h"
#include "Model.h"

#include <filesystem>

class Sphere : public Model {
public:
   Sphere();

   bool IsTriangles() const override;

   std::array<glm::vec3, 2> BoundingBox() const override;

};


class SphereInstance : public Instance {
public:
   SphereInstance(glm::vec3 centre, float radius, Material material);

public:
   static void SetModelIndex(uint32_t modelIndex);

private:
   static uint32_t sm_modelIndex;
};
