#pragma once

using Pixels = void;

class StbImage
{
public:
    static Pixels* loadRgba( const std::string& fileName, int32_t& textureWidth, int32_t& textureHeight, int32_t& textureChannels );
    static void    unloadRbga( Pixels* pixels );
};
