#pragma once

#include <DirectXMath.h>

#include <array>
#include <cstddef>
#include <span>

namespace DungeonSync::Presentation
{
    struct SpriteEffectSpawn
    {
        DirectX::XMFLOAT2 position{};
        float worldHeight{};
        float facingX{ 1.0F };
        float scale{ 1.0F };
        float lifetimeSeconds{ 0.1F };

        DirectX::XMFLOAT4 tintColor{
            1.0F,
            1.0F,
            1.0F,
            1.0F
        };

        DirectX::XMFLOAT4 uvRectangle{};
    };

    struct SpriteEffect
    {
        DirectX::XMFLOAT2 position{};
        float worldHeight{};
        float facingX{ 1.0F };
        float scale{ 1.0F };
        float remainingSeconds{};

        DirectX::XMFLOAT4 tintColor{};
        DirectX::XMFLOAT4 uvRectangle{};

        bool active{};
    };

    class SpriteEffectPool final
    {
    public:
        static constexpr std::size_t Capacity = 64;

        bool Spawn(
            const SpriteEffectSpawn& spawn) noexcept;

        void Update(float deltaSeconds) noexcept;
        void Clear() noexcept;

        [[nodiscard]]
        std::span<const SpriteEffect>
            Effects() const noexcept;

        [[nodiscard]]
        std::size_t ActiveCount() const noexcept;

    private:
        std::array<SpriteEffect, Capacity>
            effects_{};

        std::size_t activeCount_{};
    };
}