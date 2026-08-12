#include "DemoScene.h"

#include <DirectXMath.h>
#include <algorithm>

namespace DungeonSync::Presentation
{
    DemoScene::DemoScene()
    {
        renderItems_[0].tintColor =
            DirectX::XMFLOAT4{
                1.0F,
                0.35F,
                0.35F,
                1.0F
        };

        monsters_ = {
            Gameplay::Monster{
                DirectX::XMFLOAT2{ -1.2F,  0.6F }
            },
            Gameplay::Monster{
                DirectX::XMFLOAT2{  0.0F,  0.7F }
            },
            Gameplay::Monster{
                DirectX::XMFLOAT2{  1.2F,  0.6F }
            },
            Gameplay::Monster{
                DirectX::XMFLOAT2{ -0.8F, -0.6F }
            },
            Gameplay::Monster{
                DirectX::XMFLOAT2{  0.8F, -0.6F }
            }
        };

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
            constexpr float AttackRange = 0.75F;
            constexpr float AttackRangeSquared =
                AttackRange * AttackRange;

            constexpr float AttackDamage = 50.0F;

            for (Gameplay::Monster& monster : monsters_)
            {
                if (!monster.alive)
                {
                    continue;
                }

                const float differenceX =
                    monster.position.x - playerPosition_.x;

                const float differenceY =
                    monster.position.y - playerPosition_.y;

                const float distanceSquared =
                    differenceX * differenceX +
                    differenceY * differenceY;

                if (distanceSquared >
                    AttackRangeSquared)
                {
                    continue;
                }

                monster.health = std::max(
                    0.0F,
                    monster.health - AttackDamage);

                if (monster.health <= 0.0F)
                {
                    monster.alive = false;
                }
            }
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
                0.35F,
                0.35F,
                0.35F) *
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
                    0.3F,
                    0.3F,
                    0.3F) *
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