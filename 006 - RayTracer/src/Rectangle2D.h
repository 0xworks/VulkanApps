#pragma once

#include "Model.h"
#include "Instance.h"

class Rectangle2D : public Model {
public:
   Rectangle2D();
};


class Rectangle2DInstance : public Instance {
public:
   Rectangle2DInstance(glm::vec3 centre, glm::vec2 size, glm::vec3 rotationRadians, Material material);

public:
   static void SetModelIndex(uint32_t modelIndex);

private:
   static uint32_t sm_ModelIndex;
};
