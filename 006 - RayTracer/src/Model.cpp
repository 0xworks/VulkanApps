#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


Model::Model(const char* filename) {
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
            } /*Pos*/,
            {
               attrib.normals[3 * index.normal_index + 0],
               attrib.normals[3 * index.normal_index + 1],
               attrib.normals[3 * index.normal_index + 2],
            } /*Normal*/,
            {
               attrib.texcoords[2 * index.texcoord_index + 0],
               attrib.texcoords[2 * index.texcoord_index + 1],
            } /*uv*/
         };

         if (uniqueVertices.count(vertex) == 0) {
            uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
            m_Vertices.push_back(vertex);
         }
         m_Indices.push_back(uniqueVertices[vertex]);
      }
   }
}


const std::vector<Vertex>& Model::Vertices() const {
   return m_Vertices;
}


const std::vector<uint32_t>& Model::Indices() const {
   return m_Indices;
}
