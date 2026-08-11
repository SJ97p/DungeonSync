#include "DemoScene.h"

#include <DirectXMath.h>

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

    void DemoScene::Update(float totalSeconds)
    {
        const DirectX::XMMATRIX nearCubeWorld =
            DirectX::XMMatrixRotationX(
                totalSeconds * 0.7F) *
            DirectX::XMMatrixRotationY(
                totalSeconds);

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

    std::span<const Rendering::RenderItem>
        DemoScene::RenderItems() const noexcept
    {
        return renderItems_;
    }
}