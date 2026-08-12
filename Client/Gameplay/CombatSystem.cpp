#include "CombatSystem.h"

#include <algorithm>

namespace DungeonSync::Gameplay
{
    std::size_t CombatSystem::ApplyAreaAttack(
        const DirectX::XMFLOAT2& origin,
        float range,
        float damage,
        std::span<Monster> monsters) const
    {
        if (range <= 0.0F || damage <= 0.0F)
        {
            return 0;
        }

        const float rangeSquared =
            range * range;

        std::size_t hitCount = 0;

        for (Monster& monster : monsters)
        {
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

            ++hitCount;
        }

        return hitCount;
    }
}