#pragma once

#include "Model.h"
#include "Instance.h"

class Cube : public Model {
public:
   Cube();
};


class CubeInstance : public Instance {
public:
   CubeInstance(glm::vec3 centre, float sideLength, float rotateXRadians, float rotateYRadians, float rotateZRadians, Material material);

public:
   static void SetModelIndex(uint32_t modelIndex);

private:
   static uint32_t sm_ModelIndex;
};
