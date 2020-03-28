#pragma once

#include "Instance.h"

class Sphere : public Instance {
public:
   Sphere(uint32_t modelIndex, glm::vec3 centre, float radius, Material material);
};
