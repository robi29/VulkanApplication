#pragma once

////////////////////////////////////////////////////////////
/// Vertex structure.
////////////////////////////////////////////////////////////
struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 textureCoordinate;

    ////////////////////////////////////////////////////////////
    /// GetBindingDescription.
    ////////////////////////////////////////////////////////////
    static auto GetBindingDescription();

    ////////////////////////////////////////////////////////////
    /// GetAttributeDescriptions.
    ////////////////////////////////////////////////////////////
    static auto GetAttributeDescriptions();

    ////////////////////////////////////////////////////////////
    /// Overrides == operator.
    ////////////////////////////////////////////////////////////
    bool operator==( const Vertex& other ) const
    {
        return position == other.position && color == other.color && textureCoordinate == other.textureCoordinate;
    }
};
