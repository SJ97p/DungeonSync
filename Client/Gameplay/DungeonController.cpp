#include "DungeonController.h"

#include <algorithm>

namespace DungeonSync::Gameplay
{
    DungeonController::DungeonController(
        std::size_t firstMonsterIndex,
        std::size_t monsterCount,
        const DirectX::XMFLOAT2& minimumBounds,
        const DirectX::XMFLOAT2& maximumBounds) noexcept
        : room_{
            firstMonsterIndex,
            monsterCount,
            minimumBounds,
            maximumBounds,
            RoomState::Ready
        }
    {
    }

    void DungeonController::Update(
        const DirectX::XMFLOAT2& playerPosition,
        std::span<const Monster> monsters) noexcept
    {
        if (room_.state == RoomState::Ready)
        {
            if (ContainsPlayer(playerPosition))
            {
                room_.state = RoomState::Combat;
            }

            return;
        }

        if (room_.state != RoomState::Combat)
        {
            return;
        }

        if (AliveMonsterCount(monsters) == 0)
        {
            room_.state = RoomState::Cleared;
        }
    }

    const DungeonRoom&
        DungeonController::CurrentRoom() const noexcept
    {
        return room_;
    }

    std::size_t DungeonController::AliveMonsterCount(
        std::span<const Monster> monsters) const noexcept
    {
        if (room_.firstMonsterIndex >= monsters.size())
        {
            return 0;
        }

        const std::size_t availableCount =
            monsters.size() -
            room_.firstMonsterIndex;

        const std::size_t count =
            (std::min)(
                room_.monsterCount,
                availableCount);

        std::size_t aliveCount = 0;

        for (std::size_t offset = 0;
            offset < count;
            ++offset)
        {
            const std::size_t monsterIndex =
                room_.firstMonsterIndex + offset;

            if (monsters[monsterIndex].alive)
            {
                ++aliveCount;
            }
        }

        return aliveCount;
    }

    bool DungeonController::ContainsPlayer(
        const DirectX::XMFLOAT2&
        playerPosition) const noexcept
    {
        return
            playerPosition.x >=
            room_.minimumBounds.x &&
            playerPosition.x <=
            room_.maximumBounds.x &&
            playerPosition.y >=
            room_.minimumBounds.y &&
            playerPosition.y <=
            room_.maximumBounds.y;
    }
}