#pragma once

#include "../Rendering/RenderItem.h"

#include <cstddef>
#include <span>
#include <vector>

namespace DungeonSync::Diagnostics
{
    class RenderingStressScene final
    {
    public:
        static constexpr std::size_t
            MaximumInstanceCount = 10000;

        RenderingStressScene();

        void SetInstanceCount(
            std::size_t instanceCount);

        void Disable() noexcept;

        [[nodiscard]]
        bool IsActive() const noexcept;

        [[nodiscard]]
        std::size_t InstanceCount() const noexcept;

        [[nodiscard]]
        std::span<const Rendering::RenderItem>
            RenderItems() const noexcept;

    private:
        std::vector<Rendering::RenderItem>
            renderItems_;

        bool active_{};
    };
}