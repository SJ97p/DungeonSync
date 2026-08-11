#pragma once

#include "../Rendering/RenderItem.h"

#include <array>
#include <span>

namespace DungeonSync::Presentation
{
    class DemoScene final
    {
    public:
        DemoScene();

        void Update(float totalSeconds);

        [[nodiscard]]
        std::span<const Rendering::RenderItem>
            RenderItems() const noexcept;

    private:
        std::array<Rendering::RenderItem, 2> renderItems_{};
    };
}