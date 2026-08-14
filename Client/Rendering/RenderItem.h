#pragma once

#include <DirectXMath.h>

namespace DungeonSync::Rendering
{
    struct RenderItem
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4 tintColor;
        DirectX::XMFLOAT4 uvRectangle;
    };
}