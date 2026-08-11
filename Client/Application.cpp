#include "Application.h"
#include "Rendering/RenderItem.h"

#include <chrono>
#include <cstdlib>
#include <DirectXMath.h>
#include <array>

namespace
{
    constexpr std::uint32_t ClientWidth = 1280;
    constexpr std::uint32_t ClientHeight = 720;
}

namespace DungeonSync
{
    Application::Application(
        HINSTANCE instance,
        int showCommand) noexcept
        : window_(
            instance,
            ClientWidth,
            ClientHeight),
        showCommand_(showCommand)
    {
    }

    int Application::Run()
    {
        if (!window_.Initialize(showCommand_))
        {
            MessageBoxW(
                nullptr,
                L"Failed to create the application window.",
                L"DungeonSync Error",
                MB_OK | MB_ICONERROR);

            return EXIT_FAILURE;
        }

        if (!renderer_.Initialize(
            window_.Handle(),
            window_.ClientWidth(),
            window_.ClientHeight()))
        {
            MessageBoxW(
                window_.Handle(),
                L"Failed to initialize DirectX 11.",
                L"DungeonSync Error",
                MB_OK | MB_ICONERROR);

            return EXIT_FAILURE;
        }
        const auto startTime =
            std::chrono::steady_clock::now();

        while (window_.ProcessMessages())
        {
            const auto currentTime =
                std::chrono::steady_clock::now();

            const std::chrono::duration<float> elapsed =
                currentTime - startTime;

            const float totalSeconds = elapsed.count();

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

            std::array<Rendering::RenderItem, 2> renderItems{};

            DirectX::XMStoreFloat4x4(
                &renderItems[0].world,
                nearCubeWorld);

            renderItems[0].tintColor =
                DirectX::XMFLOAT4{
                    1.0F,
                    0.35F,
                    0.35F,
                    1.0F
            };

            DirectX::XMStoreFloat4x4(
                &renderItems[1].world,
                farCubeWorld);

            renderItems[1].tintColor =
                DirectX::XMFLOAT4{
                    0.35F,
                    0.35F,
                    1.0F,
                    1.0F
            };

            renderer_.Render(renderItems);
        }

        return EXIT_SUCCESS;
    }
}