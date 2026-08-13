#pragma once

#include "DungeonRoom.h"
#include "Monster.h"

#include <DirectXMath.h>

#include <cstddef>
#include <span>
#include <array>

namespace DungeonSync::Gameplay
{
    class DungeonController final
    {
    public:
        DungeonController() noexcept;

        void Update(
            const DirectX::XMFLOAT2& playerPosition,
            std::span<const Monster> monsters) noexcept;

        [[nodiscard]]
        const DungeonRoom& CurrentRoom() const noexcept;

        [[nodiscard]]
        std::size_t CurrentRoomIndex() const noexcept;

        [[nodiscard]]
        bool IsDungeonCleared() const noexcept;

        [[nodiscard]]
        bool ConsumeRoomChanged() noexcept;

        [[nodiscard]]
        bool ConsumeClearedRoom(
            std::size_t& clearedRoomIndex) noexcept;

        [[nodiscard]]
        bool ConsumeDungeonCleared() noexcept;

        void Restart() noexcept;

        [[nodiscard]]
        std::size_t AliveMonsterCount(
            std::span<const Monster> monsters) const noexcept;

    private:
        [[nodiscard]]
        bool ContainsPlayer(
            const DungeonRoom& room,
            const DirectX::XMFLOAT2&
            playerPosition) const noexcept;

        static constexpr std::size_t RoomCount = 3;

        std::array<DungeonRoom, RoomCount> rooms_{};

        std::size_t currentRoomIndex_{};
        bool dungeonCleared_{};
        bool roomChanged_{ true };

        std::size_t lastClearedRoomIndex_{};
        bool hasClearedRoomEvent_{};
        bool hasDungeonClearedEvent_{};
    };
}