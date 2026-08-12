#include "Application.h"
#include "../Shared/Network/Packet.h"

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstdint>

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
        if (!winsock_.IsInitialized())
        {
            MessageBoxW(
                nullptr,
                L"Failed to initialize Winsock.",
                L"DungeonSync Network Error",
                MB_OK | MB_ICONERROR);

            return EXIT_FAILURE;
        }

        constexpr std::uint16_t ServerPort = 27015;

        if (!tcpClient_.Connect(
            "127.0.0.1",
            ServerPort))
        {
            char message[128]{};

            std::snprintf(
                message,
                sizeof(message),
                "Failed to connect to server. Error: %d\n",
                tcpClient_.ErrorCode());

            OutputDebugStringA(message);

            MessageBoxW(
                nullptr,
                L"Failed to connect to DungeonSync Server.",
                L"DungeonSync Network Error",
                MB_OK | MB_ICONERROR);

            return EXIT_FAILURE;
        }

        OutputDebugStringA(
            "Connected to DungeonSync Server.\n");


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

        auto previousTime =
            std::chrono::steady_clock::now();

        float statisticsElapsedSeconds = 0.0F;
        std::size_t statisticsFrameCount = 0;

        constexpr float NetworkSendIntervalSeconds = 0.05F;

        float networkSendElapsedSeconds =
            NetworkSendIntervalSeconds;

        std::uint32_t moveSequence = 0;

        bool spaceWasDown = false;

        while (window_.ProcessMessages())
        {
            const auto currentTime =
                std::chrono::steady_clock::now();

            const std::chrono::duration<float> frameElapsed =
                currentTime - previousTime;

            previousTime = currentTime;

            float moveX = 0.0F;
            float moveY = 0.0F;

            if ((GetAsyncKeyState('A') & 0x8000) != 0)
            {
                moveX -= 1.0F;
            }

            if ((GetAsyncKeyState('D') & 0x8000) != 0)
            {
                moveX += 1.0F;
            }

            if ((GetAsyncKeyState('S') & 0x8000) != 0)
            {
                moveY -= 1.0F;
            }

            if ((GetAsyncKeyState('W') & 0x8000) != 0)
            {
                moveY += 1.0F;
            }

            const float moveLengthSquared =
                moveX * moveX + moveY * moveY;

            if (moveLengthSquared > 1.0F)
            {
                constexpr float DiagonalNormalizer =
                    0.70710678F;

                moveX *= DiagonalNormalizer;
                moveY *= DiagonalNormalizer;
            }

            const bool spaceIsDown =
                (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

            const bool attackPressed =
                spaceIsDown && !spaceWasDown;

            spaceWasDown = spaceIsDown;

            demoScene_.Update(
                frameElapsed.count(),
                moveX,
                moveY,
                attackPressed);

            networkSendElapsedSeconds +=
                frameElapsed.count();

            if (networkSendElapsedSeconds >=
                NetworkSendIntervalSeconds)
            {
                const DirectX::XMFLOAT2& playerPosition =
                    demoScene_.PlayerPosition();

                const Network::PlayerMovePacket movePacket =
                    Network::MakePlayerMovePacket(
                        ++moveSequence,
                        playerPosition.x,
                        playerPosition.y);

                if (!tcpClient_.Send(
                    &movePacket,
                    sizeof(movePacket)))
                {
                    char message[128]{};

                    std::snprintf(
                        message,
                        sizeof(message),
                        "Failed to send move packet. Error: %d\n",
                        tcpClient_.ErrorCode());

                    OutputDebugStringA(message);

                    return EXIT_FAILURE;
                }

                networkSendElapsedSeconds -=
                    NetworkSendIntervalSeconds;
            }

            renderer_.Render(
                demoScene_.GetCamera(),
                demoScene_.RenderItems());

            statisticsElapsedSeconds +=
                frameElapsed.count();

            ++statisticsFrameCount;

            if (statisticsElapsedSeconds >= 1.0F)
            {
                const float framesPerSecond =
                    static_cast<float>(
                        statisticsFrameCount) /
                    statisticsElapsedSeconds;

                const Rendering::RenderStatistics& statistics =
                    renderer_.Statistics();

                char message[256]{};

                std::snprintf(
                    message,
                    sizeof(message),
                    "Render - FPS: %.1f, Draw Calls: %zu, Instances: %zu\n",
                    framesPerSecond,
                    statistics.drawCallCount,
                    statistics.instanceCount);

                OutputDebugStringA(message);

                statisticsElapsedSeconds = 0.0F;
                statisticsFrameCount = 0;
            }

        }

        return EXIT_SUCCESS;
    }
}