#include <iostream>
#include <string>

#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "Libraries/tiny_obj_loader.h"

#include "main.h"
#include "tiny_obj_loader_library.h"

bool TinyObjLoader::loadModel( const std::string& fileName, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices )
{
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      warn, err;

    if( !tinyobj::LoadObj( &attrib, &shapes, &materials, &warn, &err, fileName.c_str() ) )
    {
        std::cerr << warn << " " << err << std::endl;
        return false;
    }

    for( const auto& shape : shapes )
    {
        for( const auto& index : shape.mesh.indices )
        {
            Vertex vertex = {};

            vertex.position = { attrib.vertices[3 * index.vertex_index + 0],
                                attrib.vertices[3 * index.vertex_index + 1],
                                attrib.vertices[3 * index.vertex_index + 2] };

            vertex.textureCoordinate = { attrib.texcoords[2 * index.texcoord_index + 0],
                                         1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };

            vertex.color = { 1.0f,
                             1.0f,
                             1.0f };

            vertices.push_back( vertex );
            indices.push_back( static_cast<uint32_t>( indices.size() ) );
        }
    }

    return true;
}
