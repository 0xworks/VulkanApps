#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

using vec3 = glm::vec3;
using vec2 = glm::vec2;
#include "Vertex.glsl"


inline
bool operator==(const Vertex& a, const Vertex& b) {
   return
      (a.pos == b.pos) &&
      (a.normal == b.normal) &&
      (a.uv == b.uv)
   ;
}

namespace std {

template<> struct hash<Vertex> {
   size_t operator()(Vertex const& vertex) const {
      size_t seed = 0;
      hash<glm::vec3> hasherV3;
      hash<glm::vec2> hasherV2;
      glm::detail::hash_combine(seed, hasherV3(vertex.pos));
      glm::detail::hash_combine(seed, hasherV3(vertex.normal));
      glm::detail::hash_combine(seed, hasherV2(vertex.uv));
      return seed;
   }
};

}
