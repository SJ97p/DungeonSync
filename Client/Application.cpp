#include "Application.h"
#include "../Shared/Network/Packet.h"

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cwchar>
#include <iterator>

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
        : serverStateReceiver_(tcpClient_), 
        window_(
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

        if (!serverStateReceiver_.Start())
        {
            OutputDebugStringA(
                "Failed to start server state receiver.\n");

            return EXIT_FAILURE;
        }

        OutputDebugStringA(
            "Server state receiver started.\n");


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

        std::uint32_t lastLoggedServerSequence = 0;
        float serverLogElapsedSeconds = 0.0F;

        bool attackWasDown = false;
        bool jumpWasDown = false;
        bool coneAttackWasDown = false;
        bool restartWasDown = false;

#ifndef NDEBUG
        bool testKeyWasDown = false;
#endif

        while (window_.ProcessMessages())
        {
            const auto currentTime =
                std::chrono::steady_clock::now();

            const std::chrono::duration<float> frameElapsed =
                currentTime - previousTime;

            previousTime = currentTime;

            serverLogElapsedSeconds +=
                frameElapsed.count();

            float moveX = 0.0F;
            float moveY = 0.0F;

            if ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0)
            {
                moveX -= 1.0F;
            }

            if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0)
            {
                moveX += 1.0F;
            }

            if ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0)
            {
                moveY -= 1.0F;
            }

            if ((GetAsyncKeyState(VK_UP) & 0x8000) != 0)
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

            const bool attackIsDown =
                (GetAsyncKeyState('X') & 0x8000) != 0;

            const bool attackPressed =
                attackIsDown && !attackWasDown;

            attackWasDown = attackIsDown;

            const bool jumpIsDown =
                (GetAsyncKeyState('C') & 0x8000) != 0;

            const bool jumpPressed =
                jumpIsDown && !jumpWasDown;

            jumpWasDown = jumpIsDown;

            const bool coneAttackIsDown =
                (GetAsyncKeyState('E') & 0x8000) != 0;

            const bool coneAttackPressed =
                coneAttackIsDown &&
                !coneAttackWasDown;

            coneAttackWasDown =
                coneAttackIsDown;

            const bool restartIsDown =
                (GetAsyncKeyState('R') & 0x8000) != 0;

            const bool restartPressed =
                restartIsDown &&
                !restartWasDown;

            restartWasDown = restartIsDown;

#ifndef NDEBUG
            const bool testKeyIsDown =
                (GetAsyncKeyState('T') & 0x8000) != 0;

            const bool invalidMoveTestPressed =
                testKeyIsDown &&
                !testKeyWasDown;

            testKeyWasDown = testKeyIsDown;
#endif

            if (restartPressed)
            {
                demoScene_.RestartDungeon();
            }

            demoScene_.Update(
                frameElapsed.count(),
                moveX,
                moveY,
                jumpPressed,
                attackPressed,
                coneAttackPressed);

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
#ifndef NDEBUG
            if (invalidMoveTestPressed)
            {
                const Network::PlayerMovePacket invalidMovePacket =
                    Network::MakePlayerMovePacket(
                        ++moveSequence,
                        9999.0F,
                        9999.0F);

                if (!tcpClient_.Send(
                    &invalidMovePacket,
                    sizeof(invalidMovePacket)))
                {
                    OutputDebugStringA(
                        "Failed to send invalid move test packet.\n");

                    return EXIT_FAILURE;
                }

                OutputDebugStringA(
                    "Sent invalid move test packet.\n");
            }
#endif

            Network::PlayerStateSnapshot
                serverState{};

            if (serverStateReceiver_.TryConsumeLatest(
                serverState))
            {
                demoScene_.ReconcilePlayerPosition(
                    serverState.positionX,
                    serverState.positionY,
                    serverState.accepted,
                    frameElapsed.count());

                const bool shouldLogServerState =
                    !serverState.accepted ||
                    serverLogElapsedSeconds >= 1.0F;

                if (shouldLogServerState)
                {
                    char stateMessage[256]{};

                    std::snprintf(
                        stateMessage,
                        sizeof(stateMessage),
                        "Server state"
                        " | sequence: %u"
                        " | accepted: %s"
                        " | position: (%.3f, %.3f)\n",
                        serverState.sequence,
                        serverState.accepted
                        ? "true"
                        : "false",
                        serverState.positionX,
                        serverState.positionY);

                    OutputDebugStringA(stateMessage);

                    lastLoggedServerSequence =
                        serverState.sequence;

                    serverLogElapsedSeconds = 0.0F;
                }
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

                wchar_t windowTitle[256]{};

                if (demoScene_.IsDungeonCleared())
                {
                    std::swprintf(
                        windowTitle,
                        std::size(windowTitle),
                        L"DungeonSync"
                        L" | DUNGEON CLEARED"
                        L" | PRESS R"
                        L" | FPS %.0f"
                        L" | DRAW %zu"
                        L" | INSTANCES %zu"
                        L" | SERVER CONNECTED",
                        framesPerSecond,
                        statistics.drawCallCount,
                        statistics.instanceCount);
                }
                else
                {
                    std::swprintf(
                        windowTitle,
                        std::size(windowTitle),
                        L"DungeonSync"
                        L" | ROOM %zu/%zu"
                        L" | MONSTERS %zu"
                        L" | FPS %.0f"
                        L" | DRAW %zu"
                        L" | INSTANCES %zu"
                        L" | SERVER CONNECTED",
                        demoScene_.CurrentRoomNumber(),
                        demoScene_.RoomCount(),
                        demoScene_.AliveMonsterCount(),
                        framesPerSecond,
                        statistics.drawCallCount,
                        statistics.instanceCount);
                }

                window_.SetTitle(windowTitle);

                statisticsElapsedSeconds = 0.0F;
                statisticsFrameCount = 0;
            }

        }

        serverStateReceiver_.Stop();

        return EXIT_SUCCESS;
    }
}