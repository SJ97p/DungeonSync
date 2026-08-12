#pragma once

#include "Monster.h"

#include <DirectXMath.h>
#include <cstddef>
#include <span>

namespace DungeonSync::Gameplay
{
    class CombatSystem final
    {
    public:
        [[nodiscard]]
        std::size_t ApplyAreaAttack(
            const DirectX::XMFLOAT2& origin,
            float range,
            float damage,
            std::span<Monster> monsters) const;
    };
}