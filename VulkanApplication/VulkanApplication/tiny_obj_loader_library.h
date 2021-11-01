#pragma once

using Pixels = void;

class TinyObjLoader
{
public:
    static bool loadModel( const std::string& fileName, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices );
};
