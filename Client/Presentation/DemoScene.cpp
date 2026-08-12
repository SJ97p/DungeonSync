#include "DemoScene.h"

#include <DirectXMath.h>
#include <algorithm>
#include <cstdio>
#include <Windows.h>
#include <vector>

namespace DungeonSync::Presentation
{
    DemoScene::DemoScene() : spatialGrid_(0.5F)
    {
        renderItems_[0].tintColor =
            DirectX::XMFLOAT4{
                1.0F,
                0.35F,
                0.35F,
                1.0F
        };

        constexpr std::size_t ColumnCount = 10;
        constexpr float Spacing = 0.32F;
        constexpr float StartX = -1.44F;
        constexpr float StartY = -1.44F;

        for (std::size_t index = 0;
            index < MonsterCount;
            ++index)
        {
            const std::size_t column =
                index % ColumnCount;

            const std::size_t row =
                index / ColumnCount;

            monsters_[index].position =
                DirectX::XMFLOAT2{
                    StartX +
                        static_cast<float>(column) * Spacing,
                    StartY +
                        static_cast<float>(row) * Spacing
            };
        }

        spatialGrid_.Rebuild(monsters_);

        for (std::size_t index = 0;
            index < MonsterCount;
            ++index)
        {
            renderItems_[index + 1].tintColor =
                DirectX::XMFLOAT4{
                    0.25F,
                    0.45F,
                    1.0F,
                    1.0F
            };
        }
    }

    void DemoScene::Update(
        float deltaSeconds,
        float moveX,
        float moveY,
        bool attackPressed)
    {
        constexpr float PlayerMoveSpeed = 1.5F;

        playerPosition_.x +=
            moveX * PlayerMoveSpeed * deltaSeconds;

        playerPosition_.y +=
            moveY * PlayerMoveSpeed * deltaSeconds;

        constexpr float MovementLimitX = 1.5F;
        constexpr float MovementLimitY = 0.8F;

        playerPosition_.x = std::clamp(
            playerPosition_.x,
            -MovementLimitX,
            MovementLimitX);

        playerPosition_.y = std::clamp(
            playerPosition_.y,
            -MovementLimitY,
            MovementLimitY);

        if (attackPressed)
        {
            constexpr float AttackRange = 0.45F;
            constexpr float AttackDamage = 50.0F;

            const std::vector<std::size_t> candidates =
                spatialGrid_.Query(
                    playerPosition_,
                    AttackRange);

            lastAttackResult_ =
                combatSystem_.ApplyAreaAttackToCandidates(
                    playerPosition_,
                    AttackRange,
                    AttackDamage,
                    monsters_,
                    candidates);

            char debugMessage[256]{};

            std::snprintf(
                debugMessage,
                sizeof(debugMessage),
                "Grid attack - candidates: %zu, examined: %zu, hits: %zu, elapsed: %lld ns\n",
                candidates.size(),
                lastAttackResult_.examinedCount,
                lastAttackResult_.hitCount,
                static_cast<long long>(
                    lastAttackResult_.elapsedNanoseconds));

OutputDebugStringA(debugMessage);
        }

        camera_.position = DirectX::XMFLOAT3{
    playerPosition_.x,
    playerPosition_.y,
    -3.0F
        };

        camera_.target = DirectX::XMFLOAT3{
            playerPosition_.x,
            playerPosition_.y,
            0.0F
        };

        const DirectX::XMMATRIX playerWorld =
            DirectX::XMMatrixScaling(
                0.18F,
                0.18F,
                0.18F) *
            DirectX::XMMatrixTranslation(
                playerPosition_.x,
                playerPosition_.y,
                0.0F);

        DirectX::XMStoreFloat4x4(
            &renderItems_[0].world,
            playerWorld);

        for (std::size_t index = 0;
            index < MonsterCount;
            ++index)
        {
            const Gameplay::Monster& monster =
                monsters_[index];

            Rendering::RenderItem& renderItem =
                renderItems_[index + 1];

            if (!monster.alive)
            {
                const DirectX::XMMATRIX hiddenWorld =
                    DirectX::XMMatrixScaling(
                        0.0F,
                        0.0F,
                        0.0F);

                DirectX::XMStoreFloat4x4(
                    &renderItem.world,
                    hiddenWorld);

                renderItem.tintColor =
                    DirectX::XMFLOAT4{
                        0.0F,
                        0.0F,
                        0.0F,
                        0.0F
                };

                continue;
            }

            const DirectX::XMFLOAT2& monsterPosition =
                monster.position;

            const DirectX::XMMATRIX monsterWorld =
                DirectX::XMMatrixScaling(
                    0.12F,
                    0.12F,
                    0.12F) *
                DirectX::XMMatrixTranslation(
                    monsterPosition.x,
                    monsterPosition.y,
                    0.0F);

            DirectX::XMStoreFloat4x4(
                &renderItem.world,
                monsterWorld);

            if (monster.health < 100.0F)
            {
                renderItem.tintColor =
                    DirectX::XMFLOAT4{
                        1.0F,
                        0.8F,
                        0.2F,
                        1.0F
                };
            }
            else
            {
                renderItem.tintColor =
                    DirectX::XMFLOAT4{
                        0.25F,
                        0.45F,
                        1.0F,
                        1.0F
                };
            }
        }
    }

    const Rendering::Camera&
        DemoScene::GetCamera() const noexcept
    {
        return camera_;
    }

    std::span<const Rendering::RenderItem>
        DemoScene::RenderItems() const noexcept
    {
        return renderItems_;
    }
}