#pragma once

#include "Monster.h"

#include <DirectXMath.h>
#include <cstddef>
#include <span>
#include <cstdint>

namespace DungeonSync::Gameplay
{
    struct AreaAttackResult
    {
        std::size_t examinedCount{};
        std::size_t hitCount{};
        std::int64_t elapsedNanoseconds{};
    };

    class CombatSystem final
    {
    public:
        [[nodiscard]]
        AreaAttackResult ApplyAreaAttack(
            const DirectX::XMFLOAT2& origin,
            float range,
            float damage,
            std::span<Monster> monsters) const;

        [[nodiscard]]
        AreaAttackResult ApplyAreaAttackToCandidates(
            const DirectX::XMFLOAT2& origin,
            float range,
            float damage,
            std::span<Monster> monsters,
            std::span<const std::size_t> candidateIndices) const;

        [[nodiscard]]
        AreaAttackResult ApplyConeAttackToCandidates(
            const DirectX::XMFLOAT2& origin,
            const DirectX::XMFLOAT2& direction,
            float range,
            float halfAngleRadians,
            float damage,
            std::span<Monster> monsters,
            std::span<const std::size_t>
            candidateIndices) const;
    };
}