#include "Model.h"

#include "Core.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


uint32_t Model::sm_ShaderHitGroupIndex = ~0;


Model::Model(const char* filename, const uint32_t shaderHitGroupIndex)
: m_ShaderHitGroupIndex(shaderHitGroupIndex)
{
   tinyobj::attrib_t attrib;
   std::vector<tinyobj::shape_t> shapes;
   std::vector<tinyobj::material_t> materials;
   std::string warn;
   std::string err;

   if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
      throw std::runtime_error(warn + err);
   }

   std::unordered_map<Vertex, uint32_t> uniqueVertices;

   for (const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
         Vertex vertex = {
            {
               attrib.vertices[3 * index.vertex_index + 0],
               attrib.vertices[3 * index.vertex_index + 1],
               attrib.vertices[3 * index.vertex_index + 2],
            } /*Pos*/
         };
         if (index.normal_index >= 0) {
            vertex.normal = {
               attrib.normals[3 * index.normal_index + 0],
               attrib.normals[3 * index.normal_index + 1],
               attrib.normals[3 * index.normal_index + 2],
            };
         }
         if(index.texcoord_index >= 0) {
            vertex.uv = {
               attrib.texcoords[2 * index.texcoord_index + 0],
               attrib.texcoords[2 * index.texcoord_index + 1],
            };
         }

         if (uniqueVertices.count(vertex) == 0) {
            uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
            m_Vertices.push_back(vertex);
         }
         m_Indices.push_back(uniqueVertices[vertex]);
      }
   }
}


const std::vector<Vertex>& Model::GetVertices() const {
   return m_Vertices;
}


const std::vector<uint32_t>& Model::GetIndices() const {
   return m_Indices;
}


bool Model::IsProcedural() const {
   return false;
}


uint32_t Model::GetShaderHitGroupIndex() const {
   ASSERT(m_ShaderHitGroupIndex != ~0, "ERROR: shader hit group index not set.  You must set the shader hit group index (via SetShaderHitGroupIndex()).");
   return m_ShaderHitGroupIndex;
}


std::array<glm::vec3, 2> Model::GetBoundingBox() const {
   ASSERT(false, "ERROR: GetBoundingBox() called for non-procedural model");
   return {};
};


void Model::SetDefaultShaderHitGroupIndex(const uint32_t shaderHitGroupIndex) {
   sm_ShaderHitGroupIndex = shaderHitGroupIndex;
}

uint32_t Model::GetDefaultShaderHitGroupIndex() {
   return sm_ShaderHitGroupIndex;
}
