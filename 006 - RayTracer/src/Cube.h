#pragma once

#include "Instance.h"

class Cube : public Instance {
public:
   Cube(glm::vec3 centre, float sideLength, Material material);

public:
   static void SetModelIndex(uint32_t modelIndex);

private:
   static uint32_t sm_modelIndex;
};
