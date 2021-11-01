#include <iostream>
#include <string>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#pragma warning( push )
#pragma warning( disable : 4201 )
#include <glm/gtx/hash.hpp>
#pragma warning( pop )

#define TINYOBJLOADER_IMPLEMENTATION
#include "Libraries/tiny_obj_loader.h"

#include "main.h"
#include "tiny_obj_loader_library.h"

template <>
struct std::hash<Vertex>
{
    std::size_t operator()( Vertex const& vertex ) const
    {
        const std::size_t hash1 = std::hash<glm::vec3>()( vertex.position );
        const std::size_t hash2 = std::hash<glm::vec3>()( vertex.color ) << 1;
        const std::size_t hash3 = std::hash<glm::vec2>()( vertex.textureCoordinate ) << 1;

        return ( ( hash1 ^ hash2 ) >> 1 ) ^ hash3;
    }
};

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

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

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

            if( uniqueVertices.count( vertex ) == 0 )
            {
                uniqueVertices[vertex] = static_cast<uint32_t>( vertices.size() );
                vertices.push_back( vertex );
            }

            indices.push_back( uniqueVertices[vertex] );
        }
    }

    return true;
}
