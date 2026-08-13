#include "CombatSystem.h"

#include <algorithm>
#include <chrono>
#include <cmath>

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

    AreaAttackResult
        CombatSystem::ApplyAreaAttackToCandidates(
            const DirectX::XMFLOAT2& origin,
            float range,
            float damage,
            std::span<Monster> monsters,
            std::span<const std::size_t> candidateIndices) const
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

        for (const std::size_t monsterIndex :
        candidateIndices)
        {
            // Grid 데이터가 잘못되더라도 배열 범위를 넘지 않도록 방어
            if (monsterIndex >= monsters.size())
            {
                continue;
            }

            ++attackResult.examinedCount;

            Monster& monster = monsters[monsterIndex];

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


    AreaAttackResult
        CombatSystem::ApplyConeAttackToCandidates(
            const DirectX::XMFLOAT2& origin,
            const DirectX::XMFLOAT2& direction,
            float range,
            float halfAngleRadians,
            float damage,
            std::span<Monster> monsters,
            std::span<const std::size_t>
            candidateIndices) const
    {
        if (range <= 0.0F ||
            damage <= 0.0F)
        {
            return {};
        }

        const auto startTime =
            std::chrono::steady_clock::now();

        const float rangeSquared =
            range * range;

        const float minimumDot =
            std::cos(halfAngleRadians);

        AreaAttackResult attackResult{};

        for (const std::size_t monsterIndex :
        candidateIndices)
        {
            if (monsterIndex >= monsters.size())
            {
                continue;
            }

            ++attackResult.examinedCount;

            Monster& monster =
                monsters[monsterIndex];

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

            if (distanceSquared >
                rangeSquared)
            {
                continue;
            }

            if (distanceSquared > 0.0F)
            {
                const float inverseDistance =
                    1.0F /
                    std::sqrt(distanceSquared);

                const float normalizedX =
                    differenceX * inverseDistance;

                const float normalizedY =
                    differenceY * inverseDistance;

                const float directionDot =
                    normalizedX * direction.x +
                    normalizedY * direction.y;

                if (directionDot < minimumDot)
                {
                    continue;
                }
            }

            monster.health =
                (std::max)(
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