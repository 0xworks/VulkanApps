#pragma once

#include "Model.h"
#include "Instance.h"

class Box : public Model {
public:
   Box();
};


class BoxInstance : public Instance {
public:
   BoxInstance(glm::vec3 centre, glm::vec3 size, glm::vec3 rotationRadians, Material material);

public:
   static void SetModelIndex(uint32_t modelIndex);

private:
   static uint32_t sm_ModelIndex;
};
