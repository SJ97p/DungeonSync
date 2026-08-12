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

        renderItems_[1].tintColor =
            DirectX::XMFLOAT4{
                0.35F,
                0.35F,
                1.0F,
                1.0F
        };
    }

    void DemoScene::Update(
        float totalSeconds,
        float deltaSeconds,
        float moveX,
        float moveY)
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

        const DirectX::XMMATRIX nearCubeWorld =
            DirectX::XMMatrixRotationX(
                totalSeconds * 0.7F) *
            DirectX::XMMatrixRotationY(
                totalSeconds) *
            DirectX::XMMatrixTranslation(
                playerPosition_.x,
                playerPosition_.y,
                0.0F);

        const DirectX::XMMATRIX farCubeWorld =
            DirectX::XMMatrixScaling(
                1.25F,
                1.25F,
                1.25F) *
            DirectX::XMMatrixRotationX(
                -totalSeconds * 0.4F) *
            DirectX::XMMatrixRotationY(
                -totalSeconds * 0.8F) *
            DirectX::XMMatrixTranslation(
                0.0F,
                0.0F,
                0.7F);

        DirectX::XMStoreFloat4x4(
            &renderItems_[0].world,
            nearCubeWorld);

        DirectX::XMStoreFloat4x4(
            &renderItems_[1].world,
            farCubeWorld);
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