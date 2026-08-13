#pragma once

#include "DungeonRoom.h"
#include "Monster.h"

#include <DirectXMath.h>

#include <cstddef>
#include <span>

namespace DungeonSync::Gameplay
{
    class DungeonController final
    {
    public:
        DungeonController(
            std::size_t firstMonsterIndex,
            std::size_t monsterCount,
            const DirectX::XMFLOAT2& minimumBounds,
            const DirectX::XMFLOAT2& maximumBounds) noexcept;

        void Update(
            const DirectX::XMFLOAT2& playerPosition,
            std::span<const Monster> monsters) noexcept;

        [[nodiscard]]
        const DungeonRoom& CurrentRoom() const noexcept;

        [[nodiscard]]
        std::size_t AliveMonsterCount(
            std::span<const Monster> monsters) const noexcept;

    private:
        [[nodiscard]]
        bool ContainsPlayer(
            const DirectX::XMFLOAT2&
            playerPosition) const noexcept;

        DungeonRoom room_{};
    };
}