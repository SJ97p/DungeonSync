#pragma once

#include <DirectXMath.h>

namespace DungeonSync::Rendering
{
    struct Camera
    {
        DirectX::XMFLOAT3 position{
            0.0F,
            0.0F,
            -3.0F
        };

        DirectX::XMFLOAT3 target{
            0.0F,
            0.0F,
            0.0F
        };

        DirectX::XMFLOAT3 up{
            0.0F,
            1.0F,
            0.0F
        };

        float fieldOfViewRadians{
            DirectX::XM_PIDIV4
        };

        float nearPlane{ 0.1F };
        float farPlane{ 100.0F };
    };
}