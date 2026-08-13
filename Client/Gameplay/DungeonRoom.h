#pragma once

#include <DirectXMath.h>

#include <cstddef>

namespace DungeonSync::Gameplay
{
    enum class RoomState
    {
        Ready,
        Combat,
        Cleared
    };

    struct DungeonRoom
    {
        std::size_t monsterCount{};
        float monsterHealth{ 100.0F };

        DirectX::XMFLOAT2 minimumBounds{};
        DirectX::XMFLOAT2 maximumBounds{};

        RoomState state{ RoomState::Ready };
    };
}