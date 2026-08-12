#pragma once

#include "../Rendering/RenderItem.h"
#include "../Rendering/Camera.h"
#include "../Gameplay/Monster.h"
#include "../Gameplay/CombatSystem.h"

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
            bool attackPressed);
        [[nodiscard]]
        const Rendering::Camera& GetCamera() const noexcept;

        [[nodiscard]]
        std::span<const Rendering::RenderItem>
            RenderItems() const noexcept;

    private:
        static constexpr std::size_t MonsterCount = 100;

        DirectX::XMFLOAT2 playerPosition_{};

        std::array<Gameplay::Monster, MonsterCount>
            monsters_{};
        Gameplay::CombatSystem combatSystem_{};
        Gameplay::AreaAttackResult
            lastAttackResult_{};

        std::array<
            Rendering::RenderItem,
            1 + MonsterCount>
            renderItems_{};

        Rendering::Camera camera_{};
    };
}