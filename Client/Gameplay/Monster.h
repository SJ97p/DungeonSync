#pragma once

#include <DirectXMath.h>

namespace DungeonSync::Gameplay
{
    struct Monster
    {
        DirectX::XMFLOAT2 position{};
        float health{ 100.0F };
        bool alive{ true };
    };
}