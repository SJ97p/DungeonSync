#pragma once

#include <DirectXMath.h>

namespace DungeonSync::Rendering
{
    struct InstanceData
    {
        DirectX::XMFLOAT4 worldRow0;
        DirectX::XMFLOAT4 worldRow1;
        DirectX::XMFLOAT4 worldRow2;
        DirectX::XMFLOAT4 worldRow3;
        DirectX::XMFLOAT4 tintColor;
    };

    static_assert(sizeof(InstanceData) == 80);
}