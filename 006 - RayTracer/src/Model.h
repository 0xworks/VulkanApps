#pragma once

#include "Vertex.h"

class Model {
public:
   Model(const char* filename);

   const std::vector<Vertex>& Vertices() const;

   const std::vector<uint32_t>& Indices() const;

private:
   std::vector<Vertex> m_Vertices;
   std::vector<uint32_t> m_Indices;
};
