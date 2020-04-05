#pragma once

#include "Vertex.h"

#include <array>

class Model {
public:
   Model(const char* filename, uint32_t shaderHitGroupIndex = sm_ShaderHitGroupIndex);

   // For now all "Models" have vertices and indices, even though procedural geometries
   // do not strictly need these.
   // It's convenient, however, for debugging to be able to switch procedural geometries
   // back to their triangles for comparing result.
   // Also note that you can (if you want), just leave the vertex and index collections
   // empty for procedural geometries.

   const std::vector<Vertex>& GetVertices() const;

   const std::vector<uint32_t>& GetIndices() const;

   // If true, then model intersections will be determined via AABBs + procedural shader
   virtual bool IsProcedural() const;

   // Shader hit group to use for model intersections.
   uint32_t GetShaderHitGroupIndex() const;

   // only returns sensible result if IsProcedural() = true
   virtual std::array<glm::vec3, 2> GetBoundingBox() const;

public:
   static void SetShaderHitGroupIndex(const uint32_t shaderHitGroupIndex);

private:
   std::vector<Vertex> m_Vertices;
   std::vector<uint32_t> m_Indices;
   uint32_t m_ShaderHitGroupIndex;

   static uint32_t sm_ShaderHitGroupIndex;
};
