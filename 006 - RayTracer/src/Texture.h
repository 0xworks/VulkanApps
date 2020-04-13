#pragma once

#include <glm/glm.hpp>

using vec3 = glm::vec3;
#include "Texture.glsl"

struct Texture {
   int type;
   glm::vec4 param1;
   glm::vec4 param2;
};


inline
Texture FlatColor(const glm::vec3& color) {
   return Texture{TEXTURE_FLATCOLOR, glm::vec4{color, 0.0f}};
}


inline
Texture CheckerBoard(const glm::vec3& colorOdd, const glm::vec3& colorEven, const float scale) {
   return Texture{TEXTURE_CHECKERBOARD, glm::vec4{colorOdd, scale}, glm::vec4{colorEven, 0.0f}};
}


inline
Texture Simplex3D(const glm::vec3& color, const float scale, const float weight) {
   return Texture {TEXTURE_SIMPLEX3D, glm::vec4{color, weight}, glm::vec4{0.0f, 0.0f, 0.0f, scale}};
}


inline
Texture Turbulence(const glm::vec3& color, const float scale, const float weight, const int depth) {
   return Texture {TEXTURE_TURBULENCE, glm::vec4{color, weight}, glm::vec4{0.0f, 0.0f, static_cast<float>(depth), scale}};
}

inline
Texture Marble(const glm::vec3& color, const float scale, const float weight, const int depth) {
   return Texture {TEXTURE_MARBLE, glm::vec4{color, weight}, glm::vec4{0.0f, 0.0f, static_cast<float>(depth), scale}};
}

inline
Texture Normals() {
   return Texture {TEXTURE_NORMALS};
}


inline
Texture TextureUV() {
   return Texture {TEXTURE_UV};
}
