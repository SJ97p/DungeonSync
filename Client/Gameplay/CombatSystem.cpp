#include "CombatSystem.h"

#include <algorithm>
#include <chrono>

namespace DungeonSync::Gameplay
{
    AreaAttackResult CombatSystem::ApplyAreaAttack(
        const DirectX::XMFLOAT2& origin,
        float range,
        float damage,
        std::span<Monster> monsters) const
    {
        if (range <= 0.0F || damage <= 0.0F)
        {
            return {};
        }

        const auto startTime =
            std::chrono::steady_clock::now();

        const float rangeSquared =
            range * range;

        AreaAttackResult attackResult{};

        for (Monster& monster : monsters)
        {
            ++attackResult.examinedCount;

            if (!monster.alive)
            {
                continue;
            }

            const float differenceX =
                monster.position.x - origin.x;

            const float differenceY =
                monster.position.y - origin.y;

            const float distanceSquared =
                differenceX * differenceX +
                differenceY * differenceY;

            if (distanceSquared > rangeSquared)
            {
                continue;
            }

            monster.health = std::max(
                0.0F,
                monster.health - damage);

            if (monster.health <= 0.0F)
            {
                monster.alive = false;
            }

            ++attackResult.hitCount;
        }

        const auto endTime =
            std::chrono::steady_clock::now();

        attackResult.elapsedNanoseconds =
            std::chrono::duration_cast<
            std::chrono::nanoseconds>(
                endTime - startTime)
            .count();

        return attackResult;
    }
}