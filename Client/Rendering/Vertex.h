#pragma once

#include <DirectXMath.h>

namespace DungeonSync::Rendering
{
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 textureCoordinate;
    };

    static_assert(sizeof(Vertex) == 20);
}