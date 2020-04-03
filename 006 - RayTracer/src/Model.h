#pragma once

#include "Vertex.h"

#include <array>

class Model {
public:
   Model(const char* filename);

   const std::vector<Vertex>& Vertices() const;

   const std::vector<uint32_t>& Indices() const;

   virtual bool IsTriangles() const;

   // only returns sensible result if IsTriangles() = false
   virtual std::array<glm::vec3, 2> BoundingBox() const;

private:
   std::vector<Vertex> m_Vertices;
   std::vector<uint32_t> m_Indices;
};
