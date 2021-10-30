#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "Libraries/stb_image.h"

#include "stb_image_library.h"

Pixels* StbImage::loadRgba( const std::string& fileName, int32_t& textureWidth, int32_t& textureHeight, int32_t& textureChannels )
{
    return stbi_load( fileName.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha );
}

void StbImage::unloadRbga( Pixels* pixels )
{
    if( pixels == nullptr )
    {
        return;
    }

    stbi_image_free( reinterpret_cast<stbi_uc*>( pixels ) );
}
