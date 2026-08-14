#include "RenderingStressScene.h"

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>

namespace DungeonSync::Diagnostics
{
    RenderingStressScene::RenderingStressScene()
    {
        renderItems_.reserve(
            MaximumInstanceCount);
    }

    void RenderingStressScene::SetInstanceCount(
        std::size_t instanceCount)
    {
        instanceCount =
            (std::min)(
                instanceCount,
                MaximumInstanceCount);

        if (instanceCount == 0)
        {
            Disable();
            return;
        }

        active_ = true;
        renderItems_.resize(instanceCount);

        const std::size_t columnCount =
            static_cast<std::size_t>(
                std::ceil(
                    std::sqrt(
                        static_cast<float>(
                            instanceCount))));

        const std::size_t rowCount =
            (instanceCount +
                columnCount - 1) /
            columnCount;

        constexpr float FieldWidth = 6.4F;
        constexpr float FieldDepth = 2.8F;

        const float spacingX =
            FieldWidth /
            static_cast<float>(columnCount);

        const float spacingDepth =
            FieldDepth /
            static_cast<float>(rowCount);

        const float spriteScale =
            (std::min)(
                0.32F,
                (std::min)(
                    spacingX * 0.75F,
                    spacingDepth * 0.75F));

        for (std::size_t index = 0;
            index < instanceCount;
            ++index)
        {
            const std::size_t column =
                index % columnCount;

            const std::size_t row =
                index / columnCount;

            const float positionX =
                -FieldWidth * 0.5F +
                (static_cast<float>(column) +
                    0.5F) *
                spacingX;

            const float positionDepth =
                -FieldDepth * 0.5F +
                (static_cast<float>(row) +
                    0.5F) *
                spacingDepth;

            Rendering::RenderItem& item =
                renderItems_[index];

            const DirectX::XMMATRIX world =
                DirectX::XMMatrixScaling(
                    spriteScale,
                    spriteScale,
                    1.0F) *
                DirectX::XMMatrixTranslation(
                    positionX,
                    0.0F,
                    positionDepth);

            DirectX::XMStoreFloat4x4(
                &item.world,
                world);

            const float colorVariation =
                static_cast<float>(
                    index % 7) /
                20.0F;

            item.tintColor =
                DirectX::XMFLOAT4{
                    0.70F + colorVariation,
                    0.85F,
                    1.0F - colorVariation,
                    1.0F
            };

            item.uvRectangle =
                DirectX::XMFLOAT4{
                    0.5F,
                    0.0F,
                    0.5F,
                    0.5F
            };
        }
    }

    void RenderingStressScene::Disable() noexcept
    {
        active_ = false;
        renderItems_.clear();
    }

    bool RenderingStressScene::IsActive()
        const noexcept
    {
        return active_;
    }

    std::size_t
        RenderingStressScene::InstanceCount()
        const noexcept
    {
        return renderItems_.size();
    }

    std::span<const Rendering::RenderItem>
        RenderingStressScene::RenderItems()
        const noexcept
    {
        return renderItems_;
    }
}