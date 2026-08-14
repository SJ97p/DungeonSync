#pragma once

#include "../Rendering/RenderItem.h"
#include "../Rendering/Camera.h"
#include "../Gameplay/Monster.h"
#include "../Gameplay/CombatSystem.h"
#include "../Gameplay/SpatialGrid.h"
#include "../Gameplay/DungeonController.h"

#include <DirectXMath.h>
#include <array>
#include <span>
#include <cstddef>

namespace DungeonSync::Presentation
{
    class DemoScene final
    {
    public:
        DemoScene();

        void Update(
            float deltaSeconds,
            float moveX,
            float moveY,
            bool jumpPressed,
            bool attackPressed,
            bool coneAttackPressed);

        void RestartDungeon();

        [[nodiscard]]
        const DirectX::XMFLOAT2&
            PlayerPosition() const noexcept;

        void ReconcilePlayerPosition(
            float serverPositionX,
            float serverPositionY,
            bool accepted,
            float deltaSeconds) noexcept;

        [[nodiscard]]
        std::size_t CurrentRoomNumber() const noexcept;

        [[nodiscard]]
        std::size_t RoomCount() const noexcept;

        [[nodiscard]]
        std::size_t AliveMonsterCount() const noexcept;

        [[nodiscard]]
        bool IsDungeonCleared() const noexcept;

        [[nodiscard]]
        const Rendering::Camera& GetCamera() const noexcept;

        [[nodiscard]]
        std::span<const Rendering::RenderItem>
            RenderItems() const noexcept;

    private:
        void PrepareCurrentRoom();

        static constexpr std::size_t MonsterCount = 100;

        DirectX::XMFLOAT2 playerPosition_{};
        float playerJumpHeight_{};
        float playerVerticalVelocity_{};
        bool playerIsGrounded_{ true };
        float attackEffectRemainingSeconds_{};

        DirectX::XMFLOAT2 attackEffectPosition_{};

        float attackEffectFacingX_{ 1.0F };

        DirectX::XMFLOAT2 playerFacing_{
    0.0F,
    1.0F
        };

        float playerVisualFacingX_{ 1.0F };

        std::array<Gameplay::Monster, MonsterCount>
            monsters_{};
        Gameplay::CombatSystem combatSystem_{};
        Gameplay::SpatialGrid spatialGrid_;
        Gameplay::DungeonController dungeonController_;
        
        Gameplay::AreaAttackResult
            lastAttackResult_{};

        std::array<
            Rendering::RenderItem,
            1 + MonsterCount + 1>
            renderItems_{};

        std::size_t visibleRenderItemCount_{};

        Rendering::Camera camera_{};
    };
}