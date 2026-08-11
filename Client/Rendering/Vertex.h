#pragma once

#include <DirectXMath.h>

namespace DungeonSync::Rendering
{
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    static_assert(sizeof(Vertex) == 28);
}