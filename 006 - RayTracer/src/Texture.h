#pragma once

#include <glm/glm.hpp>

struct Texture {
   int id;
   glm::vec4 param1;
   glm::vec4 param2;
};


inline
Texture FlatColor(glm::vec3 color) {
   return Texture{-1, glm::vec4{color, 0.0f}};
}


inline
Texture Checker(glm::vec3 colorOdd, glm::vec3 colorEven, float size) {
   return Texture{-2, glm::vec4{colorOdd, size}, glm::vec4{colorEven, 0.0f}};
}
