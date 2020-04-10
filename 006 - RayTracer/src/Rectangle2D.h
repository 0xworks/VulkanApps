#pragma once

#include "Model.h"
#include "Instance.h"

class Rectangle2D : public Model {
public:
   Rectangle2D();
};


class Rectangle2DInstance : public Instance {
public:
   Rectangle2DInstance(const glm::vec3& centre, const glm::vec2& size, const glm::vec3& rotationRadians, const Material& material);

public:
   static void SetModelIndex(const uint32_t modelIndex);

private:
   static uint32_t sm_ModelIndex;
};
