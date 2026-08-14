#include "SpriteEffectPool.h"

#include <algorithm>

namespace DungeonSync::Presentation
{
    bool SpriteEffectPool::Spawn(
        const SpriteEffectSpawn& spawn) noexcept
    {
        const auto iterator = std::find_if(
            effects_.begin(),
            effects_.end(),
            [](const SpriteEffect& effect)
            {
                return !effect.active;
            });

        if (iterator == effects_.end())
        {
            return false;
        }

        iterator->position = spawn.position;
        iterator->worldHeight = spawn.worldHeight;
        iterator->facingX = spawn.facingX;
        iterator->scale = spawn.scale;

        iterator->remainingSeconds =
            (std::max)(
                spawn.lifetimeSeconds,
                0.0F);

        iterator->tintColor = spawn.tintColor;
        iterator->uvRectangle = spawn.uvRectangle;
        iterator->active = true;

        ++activeCount_;

        return true;
    }

    void SpriteEffectPool::Update(
        float deltaSeconds) noexcept
    {
        const float safeDeltaSeconds =
            (std::max)(deltaSeconds, 0.0F);

        for (SpriteEffect& effect : effects_)
        {
            if (!effect.active)
            {
                continue;
            }

            effect.remainingSeconds -=
                safeDeltaSeconds;

            if (effect.remainingSeconds > 0.0F)
            {
                continue;
            }

            effect.remainingSeconds = 0.0F;
            effect.active = false;
            --activeCount_;
        }
    }

    void SpriteEffectPool::Clear() noexcept
    {
        effects_ = {};
        activeCount_ = 0;
    }

    std::span<const SpriteEffect>
        SpriteEffectPool::Effects() const noexcept
    {
        return effects_;
    }

    std::size_t
        SpriteEffectPool::ActiveCount() const noexcept
    {
        return activeCount_;
    }
}