#include "DungeonController.h"

#include <algorithm>

namespace DungeonSync::Gameplay
{
    DungeonController::DungeonController() noexcept
        : rooms_{
            DungeonRoom{
                15,
                100.0F,
                DirectX::XMFLOAT2{-1.5F, -1.5F},
                DirectX::XMFLOAT2{1.5F, 1.5F},
                RoomState::Ready
            },
            DungeonRoom{
                100,
                100.0F,
                DirectX::XMFLOAT2{-1.5F, -1.5F},
                DirectX::XMFLOAT2{1.5F, 1.5F},
                RoomState::Ready
            },
            DungeonRoom{
                12,
                250.0F,
                DirectX::XMFLOAT2{-1.5F, -1.5F},
                DirectX::XMFLOAT2{1.5F, 1.5F},
                RoomState::Ready
            }
        }
    { }

    void DungeonController::Update(
        const DirectX::XMFLOAT2& playerPosition,
        std::span<const Monster> monsters) noexcept
    {
        if (dungeonCleared_)
        {
            return;
        }

        DungeonRoom& room =
            rooms_[currentRoomIndex_];

        if (room.state == RoomState::Ready)
        {
            if (ContainsPlayer(
                room,
                playerPosition))
            {
                room.state = RoomState::Combat;
            }

            return;
        }

        if (room.state != RoomState::Combat)
        {
            return;
        }

        if (AliveMonsterCount(monsters) != 0)
        {
            return;
        }

        room.state = RoomState::Cleared;

        lastClearedRoomIndex_ =
            currentRoomIndex_;

        hasClearedRoomEvent_ = true;

        const std::size_t nextRoomIndex =
            currentRoomIndex_ + 1;

        if (nextRoomIndex >= rooms_.size())
        {
            dungeonCleared_ = true;
            hasDungeonClearedEvent_ = true;
            return;
        }

        currentRoomIndex_ = nextRoomIndex;
        roomChanged_ = true;
    }

    const DungeonRoom&
        DungeonController::CurrentRoom() const noexcept
    {
        return rooms_[currentRoomIndex_];
    }

    std::size_t
        DungeonController::CurrentRoomIndex() const noexcept
    {
        return currentRoomIndex_;
    }

    bool DungeonController::IsDungeonCleared() const noexcept
    {
        return dungeonCleared_;
    }

    bool DungeonController::ConsumeRoomChanged() noexcept
    {
        if (!roomChanged_)
        {
            return false;
        }

        roomChanged_ = false;
        return true;
    }

    bool DungeonController::ConsumeClearedRoom(
        std::size_t& clearedRoomIndex) noexcept
    {
        if (!hasClearedRoomEvent_)
        {
            return false;
        }

        clearedRoomIndex =
            lastClearedRoomIndex_;

        hasClearedRoomEvent_ = false;

        return true;
    }

    bool DungeonController::ConsumeDungeonCleared()
        noexcept
    {
        if (!hasDungeonClearedEvent_)
        {
            return false;
        }

        hasDungeonClearedEvent_ = false;

        return true;
    }

    void DungeonController::Restart() noexcept
    {
        for (DungeonRoom& room : rooms_)
        {
            room.state = RoomState::Ready;
        }

        currentRoomIndex_ = 0;
        dungeonCleared_ = false;
        roomChanged_ = true;

        lastClearedRoomIndex_ = 0;
        hasClearedRoomEvent_ = false;
        hasDungeonClearedEvent_ = false;
    }

    std::size_t DungeonController::AliveMonsterCount(
        std::span<const Monster> monsters) const noexcept
    {
        const DungeonRoom& room =
            CurrentRoom();

        const std::size_t count =
            (std::min)(
                room.monsterCount,
                monsters.size());

        std::size_t aliveCount = 0;

        for (std::size_t index = 0;
            index < count;
            ++index)
        {
            if (monsters[index].alive)
            {
                ++aliveCount;
            }
        }

        return aliveCount;
    }

    bool DungeonController::ContainsPlayer(
        const DungeonRoom& room,
        const DirectX::XMFLOAT2&
        playerPosition) const noexcept
    {
        return
            playerPosition.x >=
            room.minimumBounds.x &&
            playerPosition.x <=
            room.maximumBounds.x &&
            playerPosition.y >=
            room.minimumBounds.y &&
            playerPosition.y <=
            room.maximumBounds.y;
    }

}