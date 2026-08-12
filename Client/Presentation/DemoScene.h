#pragma once

#include "../Rendering/RenderItem.h"
#include "../Rendering/Camera.h"

#include <DirectXMath.h>
#include <array>
#include <span>

namespace DungeonSync::Presentation
{
    class DemoScene final
    {
    public:
        DemoScene();

        void Update(
            float totalSeconds,
            float deltaSeconds,
            float moveX,
            float moveY);
        [[nodiscard]]
        const Rendering::Camera& GetCamera() const noexcept;

        [[nodiscard]]
        std::span<const Rendering::RenderItem>
            RenderItems() const noexcept;

    private:
        DirectX::XMFLOAT2 playerPosition_{};
        std::array<Rendering::RenderItem, 2> renderItems_{};
        Rendering::Camera camera_{};
    };
}