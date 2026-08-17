#include "Application.h"
#include "../Shared/Network/Packet.h"
#include "Diagnostics/ProcessMemorySampler.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cwchar>
#include <iterator>
#include <thread>
#include <array>
#include <filesystem>
#include <string>
#include <utility>

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
        std::uint64_t
            lastRecordedGpuTimingSampleSerial = 0;

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

        if (!textureResourceManager_.Start(
            renderer_))
        {
            OutputDebugStringA(
                "Failed to start texture resource manager.\n");

            return EXIT_FAILURE;
        }

        const std::array<std::filesystem::path, 7>
            textureLoadTestPaths{
                L"Assets/Textures/Environment/floor_stone_a.png",
                L"Assets/Textures/Environment/floor_stone_b.png",
                L"Assets/Textures/Environment/aqueduct_far_background.png",
                L"Assets/Textures/Sprites/dungeon_sprite_atlas.png",
                L"Assets/Textures/Sprites/monster_golem_boss.png",
                L"Assets/Textures/Sprites/monster_imp.png",
                L"Assets/Textures/Sprites/vfx_masks.png"
        };

        constexpr std::size_t GroundTextureTestIndex = 1;

        const std::filesystem::path& asyncTextureTestPath =
            textureLoadTestPaths[GroundTextureTestIndex];

        bool streamedTextureReadyWasLogged = false;

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
        bool benchmarkStartWasDown = false;
        bool benchmarkCancelWasDown = false;
        bool synchronousGroundLoadWasDown = false;
        bool asyncGroundLoadWasDown = false;
        bool groundResetWasDown = false;
        bool diagnosticsOverlayWasDown = false;
        bool diagnosticsOverlayEnabled = false;

        std::wstring diagnosticsOverlayText{
            L"DUNGEONSYNC ENGINE DIAGNOSTICS\nCollecting samples..."
        };

        std::array<bool, 5>
            stressKeyWasDown{};

        bool submissionModeToggleWasDown = false;

        Rendering::SpriteSubmissionMode
            stressSubmissionMode =
            Rendering::SpriteSubmissionMode::
            InstancedBatch;

#ifndef NDEBUG
        bool testKeyWasDown = false;
        bool hitchTestKeyWasDown = false;
#endif

        const auto resetBenchmarkProfilers =
            [this,
            &statisticsElapsedSeconds,
            &statisticsFrameCount]()
            {
                frameTimeProfiler_.Reset();
                cpuSubmissionProfiler_.Reset();
                presentProfiler_.Reset();
                gpuProfiler_.Reset();

                renderer_.BeginGpuTimingGeneration();

                statisticsElapsedSeconds = 0.0F;
                statisticsFrameCount = 0;
            };

        const auto applyBenchmarkScenario =
            [this, &stressSubmissionMode]()
            {
                const Diagnostics::BenchmarkScenario&
                    scenario =
                    benchmarkSession_.CurrentScenario();

                renderingStressScene_.SetInstanceCount(
                    scenario.instanceCount);

                if (scenario.submissionMode ==
                    Diagnostics::BenchmarkSubmissionMode::
                    InstancedBatch)
                {
                    stressSubmissionMode =
                        Rendering::SpriteSubmissionMode::
                        InstancedBatch;
                }
                else
                {
                    stressSubmissionMode =
                        Rendering::SpriteSubmissionMode::
                        PerInstance;
                }

                renderer_.BeginGpuTimingGeneration();

                char message[256]{};

                std::snprintf(
                    message,
                    sizeof(message),
                    "Benchmark scenario %zu/%zu"
                    " | instances: %zu"
                    " | mode: %s"
                    " | warmup: %.1f seconds\n",
                    benchmarkSession_.CurrentScenarioNumber(),
                    Diagnostics::BenchmarkSession::ScenarioCount,
                    scenario.instanceCount,
                    scenario.submissionMode ==
                    Diagnostics::BenchmarkSubmissionMode::
                    InstancedBatch
                    ? "instanced"
                    : "per-instance",
                    Diagnostics::BenchmarkSession::
                    WarmupSeconds);

                OutputDebugStringA(message);
            };

        while (window_.ProcessMessages())
        {
            const auto currentTime =
                std::chrono::steady_clock::now();

            const std::chrono::duration<float> frameElapsed =
                currentTime - previousTime;

            previousTime = currentTime;
            frameTimeProfiler_.RecordFrame(
                frameElapsed.count());

            textureResourceManager_.Update();

            const bool streamedTextureSetIsReady =
                std::ranges::all_of(
                    textureLoadTestPaths,
                    [this](const std::filesystem::path& path)
                    {
                        return textureResourceManager_.State(path) ==
                            Rendering::TextureLoadState::Ready;
                    });

            if (!streamedTextureReadyWasLogged &&
                streamedTextureSetIsReady)
            {
                const Rendering::LoadedTexture*
                    streamedTexture =
                    textureResourceManager_.Find(
                        asyncTextureTestPath);

                const Rendering::
                    TextureResourceManagerStatistics
                    resourceStatistics =
                    textureResourceManager_.Statistics();

                if (streamedTexture != nullptr)
                {
                    const bool groundTextureSwitched =
                        renderer_.SetGroundTextureOverride(
                            streamedTexture);

                    char readyMessage[448]{};

                    std::snprintf(
                        readyMessage,
                        sizeof(readyMessage),
                        "Async texture set ready"
                        " | ground switched: %s"
                        " | textures: %zu"
                        " | width: %u"
                        " | height: %u"
                        " | resources: %zu"
                        " | ready: %zu"
                        " | GPU bytes: %llu"
                        " | decode: %.3f ms"
                        " | upload: %.3f ms"
                        " | request-to-ready: %.3f ms"
                        " | peak decoded bytes: %zu\n",
                        groundTextureSwitched
                        ? "true"
                        : "false",
                        textureLoadTestPaths.size(),
                        streamedTexture->width,
                        streamedTexture->height,
                        resourceStatistics.resourceCount,
                        resourceStatistics.readyResourceCount,
                        static_cast<unsigned long long>(
                            resourceStatistics
                            .estimatedGpuBytes),
                        resourceStatistics
                        .lastDecodeMilliseconds,
                        resourceStatistics
                        .lastUploadMilliseconds,
                        resourceStatistics
                        .lastRequestToReadyMilliseconds,
                        resourceStatistics
                        .peakDecodedBytes);

                    OutputDebugStringA(readyMessage);

                    streamedTextureReadyWasLogged = true;
                }
            }

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

            constexpr std::array<int, 5>
                StressKeys{
                    '1',
                    '2',
                    '3',
                    '4',
                    '5'
            };

            constexpr std::array<std::size_t, 5>
                StressInstanceCounts{
                    100,
                    1000,
                    3000,
                    10000,
                    0
            };

            for (std::size_t index = 0;
                index < StressKeys.size();
                ++index)
            {
                const bool keyIsDown =
                    (GetAsyncKeyState(
                        StressKeys[index]) &
                        0x8000) != 0;

                const bool keyPressed =
                    keyIsDown &&
                    !stressKeyWasDown[index];

                if (benchmarkSession_.IsActive())
                {
                    continue;
                }

                stressKeyWasDown[index] =
                    keyIsDown;

                if (!keyPressed)
                {
                    continue;
                }

                const std::size_t requestedCount =
                    StressInstanceCounts[index];

                if (requestedCount == 0)
                {
                    renderingStressScene_.Disable();

                    frameTimeProfiler_.Reset();
                    cpuSubmissionProfiler_.Reset();
                    presentProfiler_.Reset();
                    gpuProfiler_.Reset();
                    renderer_.BeginGpuTimingGeneration();
                    statisticsElapsedSeconds = 0.0F;
                    statisticsFrameCount = 0;

                    OutputDebugStringA(
                        "Rendering stress scene disabled.\n");

                    continue;
                }

                renderingStressScene_.SetInstanceCount(
                    requestedCount);

                frameTimeProfiler_.Reset();
                cpuSubmissionProfiler_.Reset();
                presentProfiler_.Reset();
                gpuProfiler_.Reset();
                renderer_.BeginGpuTimingGeneration();
                statisticsElapsedSeconds = 0.0F;
                statisticsFrameCount = 0;

                char stressMessage[128]{};

                std::snprintf(
                    stressMessage,
                    sizeof(stressMessage),
                    "Rendering stress scene enabled"
                    " | instances: %zu\n",
                    requestedCount);

                OutputDebugStringA(stressMessage);
            }

            const bool submissionModeToggleIsDown =
                (GetAsyncKeyState('6') &
                    0x8000) != 0;

            const bool submissionModeTogglePressed =
                submissionModeToggleIsDown &&
                !submissionModeToggleWasDown;

            submissionModeToggleWasDown =
                submissionModeToggleIsDown;

            if (submissionModeTogglePressed &&
                !benchmarkSession_.IsActive())
            {
                if (!renderingStressScene_.IsActive())
                {
                    OutputDebugStringA(
                        "6 ignored."
                        " Enable a stress scene first.\n");
                }
                else if (stressSubmissionMode ==
                    Rendering::SpriteSubmissionMode::
                    InstancedBatch)
                {
                    stressSubmissionMode =
                        Rendering::SpriteSubmissionMode::
                        PerInstance;

                    frameTimeProfiler_.Reset();
                    cpuSubmissionProfiler_.Reset();
                    presentProfiler_.Reset();
                    gpuProfiler_.Reset();
                    renderer_.BeginGpuTimingGeneration();
                    statisticsElapsedSeconds = 0.0F;
                    statisticsFrameCount = 0;

                    OutputDebugStringA(
                        "Sprite submission mode"
                        " | per-instance draws\n");
                }
                else
                {
                    stressSubmissionMode =
                        Rendering::SpriteSubmissionMode::
                        InstancedBatch;

                    frameTimeProfiler_.Reset();
                    cpuSubmissionProfiler_.Reset();
                    presentProfiler_.Reset();
                    gpuProfiler_.Reset();
                    renderer_.BeginGpuTimingGeneration();
                    statisticsElapsedSeconds = 0.0F;
                    statisticsFrameCount = 0;

                    OutputDebugStringA(
                        "Sprite submission mode"
                        " | instanced batch\n");
                }
            }

            const bool benchmarkStartIsDown =
                (GetAsyncKeyState('7') &
                    0x8000) != 0;

            const bool benchmarkStartPressed =
                benchmarkStartIsDown &&
                !benchmarkStartWasDown;

            benchmarkStartWasDown =
                benchmarkStartIsDown;

            const bool benchmarkCancelIsDown =
                (GetAsyncKeyState('8') &
                    0x8000) != 0;

            const bool benchmarkCancelPressed =
                benchmarkCancelIsDown &&
                !benchmarkCancelWasDown;

            benchmarkCancelWasDown =
                benchmarkCancelIsDown;

            const bool synchronousGroundLoadIsDown =
                (GetAsyncKeyState('9') &
                    0x8000) != 0;

            const bool synchronousGroundLoadPressed =
                synchronousGroundLoadIsDown &&
                !synchronousGroundLoadWasDown;

            synchronousGroundLoadWasDown =
                synchronousGroundLoadIsDown;

            const bool asyncGroundLoadIsDown =
                (GetAsyncKeyState('0') &
                    0x8000) != 0;

            const bool asyncGroundLoadPressed =
                asyncGroundLoadIsDown &&
                !asyncGroundLoadWasDown;

            asyncGroundLoadWasDown =
                asyncGroundLoadIsDown;

            const bool groundResetIsDown =
                (GetAsyncKeyState(VK_OEM_PLUS) &
                    0x8000) != 0;

            const bool groundResetPressed =
                groundResetIsDown &&
                !groundResetWasDown;

            groundResetWasDown =
                groundResetIsDown;

            const bool diagnosticsOverlayIsDown =
                (GetAsyncKeyState(VK_OEM_MINUS) &
                    0x8000) != 0;

            const bool diagnosticsOverlayPressed =
                diagnosticsOverlayIsDown &&
                !diagnosticsOverlayWasDown;

            diagnosticsOverlayWasDown =
                diagnosticsOverlayIsDown;

            if (diagnosticsOverlayPressed)
            {
                diagnosticsOverlayEnabled =
                    !diagnosticsOverlayEnabled;

                OutputDebugStringA(
                    diagnosticsOverlayEnabled
                    ? "Diagnostics overlay enabled.\n"
                    : "Diagnostics overlay disabled.\n");
            }

            if (synchronousGroundLoadPressed)
            {
                if (benchmarkSession_.IsActive())
                {
                    OutputDebugStringA(
                        "9 ignored during benchmark.\n");
                }
                else
                {
                    const auto loadStartTime =
                        std::chrono::steady_clock::now();

                    std::vector<Rendering::LoadedTexture>
                        loadedTextures;

                    loadedTextures.reserve(
                        textureLoadTestPaths.size());

                    bool loadSucceeded = true;
                    std::uint64_t estimatedGpuBytes = 0;

                    for (const std::filesystem::path& path :
                        textureLoadTestPaths)
                    {
                        Rendering::LoadedTexture loadedTexture{};

                        if (!renderer_.LoadTextureSynchronously(
                            path,
                            loadedTexture))
                        {
                            loadSucceeded = false;
                            break;
                        }

                        estimatedGpuBytes +=
                            static_cast<std::uint64_t>(
                                loadedTexture.width) *
                            loadedTexture.height *
                            4ULL;

                        loadedTextures.push_back(
                            std::move(loadedTexture));
                    }

                    const auto loadEndTime =
                        std::chrono::steady_clock::now();

                    const float loadMilliseconds =
                        std::chrono::duration<float, std::milli>(
                            loadEndTime - loadStartTime).count();

                    const std::size_t loadedTextureCount =
                        loadedTextures.size();

                    bool switched = false;

                    if (loadSucceeded)
                    {
                        synchronousTextureSet_ =
                            std::move(loadedTextures);

                        switched =
                            renderer_.SetGroundTextureOverride(
                                &synchronousTextureSet_[
                                    GroundTextureTestIndex]);
                    }

                    char message[320]{};

                    std::snprintf(
                        message,
                        sizeof(message),
                        "9 synchronous texture set completed"
                        " | success: %s"
                        " | ground switched: %s"
                        " | textures: %zu"
                        " | estimated GPU bytes: %llu"
                        " | main-thread stall: %.3f ms\n",
                        loadSucceeded ? "true" : "false",
                        switched ? "true" : "false",
                        loadedTextureCount,
                        static_cast<unsigned long long>(
                            estimatedGpuBytes),
                        loadMilliseconds);

                    OutputDebugStringA(message);
                }
            }

            if (asyncGroundLoadPressed)
            {
                if (benchmarkSession_.IsActive())
                {
                    OutputDebugStringA(
                        "0 ignored during benchmark.\n");
                }
                else
                {
                    if (streamedTextureSetIsReady)
                    {
                        const Rendering::LoadedTexture*
                            cachedTexture =
                            textureResourceManager_.Find(
                                asyncTextureTestPath);

                        const bool switched =
                            renderer_.SetGroundTextureOverride(
                                cachedTexture);

                        streamedTextureReadyWasLogged = true;

                        OutputDebugStringA(
                            switched
                            ? "0 texture set cache hit"
                              " | floor_stone_b applied.\n"
                            : "0 texture set cache hit"
                              " | failed to apply texture.\n");
                    }
                    else
                    {
                        std::size_t queuedCount = 0;
                        std::size_t existingCount = 0;

                        for (const std::filesystem::path& path :
                            textureLoadTestPaths)
                        {
                            const Rendering::TextureLoadState state =
                                textureResourceManager_.State(path);

                            if (state ==
                                Rendering::TextureLoadState::Unloaded ||
                                state ==
                                Rendering::TextureLoadState::Failed)
                            {
                                if (textureResourceManager_.RequestAsync(
                                    path))
                                {
                                    ++queuedCount;
                                }
                            }
                            else
                            {
                                ++existingCount;
                            }
                        }

                        streamedTextureReadyWasLogged = false;

                        char queuedMessage[192]{};

                        std::snprintf(
                            queuedMessage,
                            sizeof(queuedMessage),
                            "0 async texture set requested"
                            " | queued: %zu"
                            " | existing: %zu"
                            " | total: %zu\n",
                            queuedCount,
                            existingCount,
                            textureLoadTestPaths.size());

                        OutputDebugStringA(queuedMessage);
                    }
                }
            }

            if (groundResetPressed)
            {
                if (benchmarkSession_.IsActive())
                {
                    OutputDebugStringA(
                        "= ignored during benchmark.\n");
                }
                else
                {
                    (void)renderer_.SetGroundTextureOverride(
                        nullptr);

                    streamedTextureReadyWasLogged = true;

                    OutputDebugStringA(
                        "= ground texture reset"
                        " | floor_stone_a applied.\n");
                }
            }

            if (benchmarkStartPressed &&
                !benchmarkSession_.IsActive())
            {
                renderer_.SetVSyncEnabled(false);
                benchmarkSession_.Start();
                applyBenchmarkScenario();

                OutputDebugStringA(
                    "Automated benchmark started"
                    " | presentation: immediate"
                    " | press 8 to cancel.\n");
            }

            if (benchmarkCancelPressed &&
                benchmarkSession_.IsActive())
            {
                benchmarkSession_.Stop();
                renderingStressScene_.Disable();
                renderer_.SetVSyncEnabled(true);

                stressSubmissionMode =
                    Rendering::SpriteSubmissionMode::
                    InstancedBatch;

                resetBenchmarkProfilers();

                OutputDebugStringA(
                    "Automated benchmark cancelled"
                    " | VSync restored.\n");
            }

            std::size_t collectedBenchmarkSamples =
                frameTimeProfiler_.SampleCount();

            collectedBenchmarkSamples =
                (std::min)(
                    collectedBenchmarkSamples,
                    cpuSubmissionProfiler_.SampleCount());

            collectedBenchmarkSamples =
                (std::min)(
                    collectedBenchmarkSamples,
                    presentProfiler_.SampleCount());

            collectedBenchmarkSamples =
                (std::min)(
                    collectedBenchmarkSamples,
                    gpuProfiler_.SampleCount());

            const Diagnostics::BenchmarkSessionEvent
                benchmarkEvent =
                benchmarkSession_.Update(
                    frameElapsed.count(),
                    collectedBenchmarkSamples);

            if (benchmarkEvent ==
                Diagnostics::BenchmarkSessionEvent::
                MeasurementStarted)
            {
                resetBenchmarkProfilers();

                char measurementMessage[160]{};

                std::snprintf(
                    measurementMessage,
                    sizeof(measurementMessage),
                    "Benchmark measurement started"
                    " | minimum duration: %.1f seconds"
                    " | target samples: %zu\n",
                    Diagnostics::BenchmarkSession::
                    MinimumMeasurementSeconds,
                    Diagnostics::BenchmarkSession::
                    TargetSampleCount);

                OutputDebugStringA(measurementMessage);
            }
            else if (benchmarkEvent ==
                Diagnostics::BenchmarkSessionEvent::
                MeasurementFinished)
            {
                const Diagnostics::BenchmarkScenario&
                    scenario =
                    benchmarkSession_.CurrentScenario();

                const Rendering::RenderStatistics&
                    benchmarkRenderStatistics =
                    renderer_.Statistics();

                Diagnostics::BenchmarkResult result{};

                result.scenarioName =
                    "sprite_submission";

                result.submissionMode =
                    scenario.submissionMode ==
                    Diagnostics::BenchmarkSubmissionMode::
                    InstancedBatch
                    ? "instanced"
                    : "per_instance";

                result.presentationMode =
                    renderer_.IsVSyncEnabled()
                    ? "vsync"
                    : "immediate";

#ifdef NDEBUG
                result.buildConfiguration = "Release";
#else
                result.buildConfiguration = "Debug";
#endif

                result.instanceCount =
                    benchmarkRenderStatistics.instanceCount;

                result.drawCallCount =
                    benchmarkRenderStatistics.drawCallCount;

                result.droppedInstanceCount =
                    benchmarkRenderStatistics
                    .droppedInstanceCount;

                result.instanceBufferCapacity =
                    benchmarkRenderStatistics
                    .instanceBufferCapacity;

                result.frameTime =
                    frameTimeProfiler_.CaptureSnapshot();

                result.cpuSubmissionTime =
                    cpuSubmissionProfiler_
                    .CaptureSnapshot();

                result.gpuTime =
                    gpuProfiler_.CaptureSnapshot();

                result.presentTime =
                    presentProfiler_.CaptureSnapshot();

                const bool saved =
                    Diagnostics::BenchmarkCsvWriter::Append(
                        "Benchmarks/results.csv",
                        result);

                if (saved)
                {
                    char savedMessage[256]{};

                    std::snprintf(
                        savedMessage,
                        sizeof(savedMessage),
                        "Benchmark result saved"
                        " | scenario: %zu/%zu"
                        " | instances: %zu"
                        " | mode: %s\n",
                        benchmarkSession_
                        .CurrentScenarioNumber(),
                        Diagnostics::BenchmarkSession::
                        ScenarioCount,
                        scenario.instanceCount,
                        result.submissionMode.c_str());

                    OutputDebugStringA(savedMessage);
                }
                else
                {
                    OutputDebugStringA(
                        "Failed to save benchmark result.\n");
                }

                if (benchmarkSession_.AdvanceAfterResult())
                {
                    applyBenchmarkScenario();
                }
                else
                {
                    renderingStressScene_.Disable();
                    renderer_.SetVSyncEnabled(true);

                    stressSubmissionMode =
                        Rendering::SpriteSubmissionMode::
                        InstancedBatch;

                    OutputDebugStringA(
                        "Automated benchmark completed."
                        " VSync restored."
                        " Results: Benchmarks/results.csv\n");
                }
            }

#ifndef NDEBUG
            const bool testKeyIsDown =
                (GetAsyncKeyState('T') & 0x8000) != 0;

            const bool invalidMoveTestPressed =
                testKeyIsDown &&
                !testKeyWasDown;

            testKeyWasDown = testKeyIsDown;

            const bool hitchTestKeyIsDown =
                (GetAsyncKeyState('H') & 0x8000) != 0;

            const bool hitchTestPressed =
                hitchTestKeyIsDown &&
                !hitchTestKeyWasDown;

            hitchTestKeyWasDown =
                hitchTestKeyIsDown;

            if (hitchTestPressed)
            {
                OutputDebugStringA(
                    "Simulating a 50 ms frame hitch.\n");

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(50));
            }
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


            const std::wstring_view diagnosticsText =
                diagnosticsOverlayEnabled
                ? std::wstring_view(diagnosticsOverlayText)
                : std::wstring_view{};

            if (renderingStressScene_.IsActive())
            {
                renderer_.Render(
                    demoScene_.GetCamera(),
                    renderingStressScene_.RenderItems(),
                    stressSubmissionMode,
                    diagnosticsText);
            }
            else
            {
                renderer_.Render(
                    demoScene_.GetCamera(),
                    demoScene_.RenderItems(),
                    Rendering::SpriteSubmissionMode::InstancedBatch,
                    diagnosticsText);
            }

            const Rendering::RenderStatistics&
                currentRenderStatistics =
                renderer_.Statistics();

            cpuSubmissionProfiler_.RecordMilliseconds(
                currentRenderStatistics
                .cpuSubmissionMilliseconds);

            presentProfiler_.RecordMilliseconds(
                currentRenderStatistics
                .presentMilliseconds);

            if (currentRenderStatistics.gpuTimingValid &&
                currentRenderStatistics
                .gpuTimingSampleSerial !=
                lastRecordedGpuTimingSampleSerial)
            {
                gpuProfiler_.RecordMilliseconds(
                    currentRenderStatistics
                    .gpuMilliseconds);

                lastRecordedGpuTimingSampleSerial =
                    currentRenderStatistics
                    .gpuTimingSampleSerial;
            }

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

                const Diagnostics::FrameTimeSnapshot
                    frameTimeSnapshot =
                        frameTimeProfiler_.CaptureSnapshot();

                const Diagnostics::FrameTimeSnapshot
                    cpuSubmissionSnapshot =
                        cpuSubmissionProfiler_
                        .CaptureSnapshot();

                const Diagnostics::FrameTimeSnapshot
                    presentSnapshot =
                        presentProfiler_
                        .CaptureSnapshot();

                const Diagnostics::FrameTimeSnapshot
                    gpuSnapshot =
                    gpuProfiler_.CaptureSnapshot();

                const Rendering::
                    TextureResourceManagerStatistics
                    textureStatistics =
                    textureResourceManager_.Statistics();

                const Diagnostics::ProcessMemorySnapshot
                    processMemory =
                    Diagnostics::ProcessMemorySampler::Capture();

                constexpr double BytesPerMebibyte =
                    1024.0 * 1024.0;

                const std::uint64_t instanceBufferBytes =
                    static_cast<std::uint64_t>(
                        statistics.instanceBufferCapacity) *
                    sizeof(Rendering::InstanceData);

                const std::uint64_t trackedGpuBytes =
                    textureStatistics.estimatedGpuBytes +
                    instanceBufferBytes;

                wchar_t overlayBuffer[1024]{};

                std::swprintf(
                    overlayBuffer,
                    std::size(overlayBuffer),
                    L"DUNGEONSYNC ENGINE DIAGNOSTICS [-]\n"
                    L"Frame  FPS %6.1f | Avg %6.3f ms | P99 %6.3f ms | Max %7.3f ms\n"
                    L"Hitch  >16.67ms %zu/%zu | >33.33ms %zu/%zu\n"
                    L"Render Draw %zu | Submitted %zu | Visible %zu | Dropped %zu\n"
                    L"Timing CPU P99 %6.3f ms | GPU P99 %6.3f ms | Present P99 %6.3f ms\n"
                    L"Buffer Instance capacity %zu | %.2f KiB\n"
                    L"Texture Ready %zu/%zu | GPU %.2f MiB | Decoded pending %.2f MiB\n"
                    L"Stream  Queued %zu | Completed %zu | Decoding %s | Peak decoded %.2f MiB\n"
                    L"Process Working %.2f MiB | Peak %.2f MiB | Private %.2f MiB\n"
                    L"Tracked GPU %.2f MiB (textures + instance buffer)",
                    framesPerSecond,
                    frameTimeSnapshot.averageMilliseconds,
                    frameTimeSnapshot.percentile99Milliseconds,
                    frameTimeSnapshot.maximumMilliseconds,
                    frameTimeSnapshot.framesOver16Milliseconds,
                    frameTimeSnapshot.sampleCount,
                    frameTimeSnapshot.framesOver33Milliseconds,
                    frameTimeSnapshot.sampleCount,
                    statistics.drawCallCount,
                    statistics.submittedInstanceCount,
                    statistics.instanceCount,
                    statistics.droppedInstanceCount,
                    cpuSubmissionSnapshot.percentile99Milliseconds,
                    gpuSnapshot.percentile99Milliseconds,
                    presentSnapshot.percentile99Milliseconds,
                    statistics.instanceBufferCapacity,
                    static_cast<double>(
                        statistics.instanceBufferCapacity *
                        sizeof(Rendering::InstanceData)) / 1024.0,
                    textureStatistics.readyResourceCount,
                    textureStatistics.resourceCount,
                    static_cast<double>(
                        textureStatistics.estimatedGpuBytes) /
                        (1024.0 * 1024.0),
                    static_cast<double>(
                        textureStatistics.decodedBytesAwaitingUpload) /
                        (1024.0 * 1024.0),
                    textureStatistics.queuedRequestCount,
                    textureStatistics.completedResultCount,
                    textureStatistics.decoding ? L"YES" : L"NO",
                    static_cast<double>(
                        textureStatistics.peakDecodedBytes) /
                        BytesPerMebibyte,
                    static_cast<double>(
                        processMemory.workingSetBytes) /
                        BytesPerMebibyte,
                    static_cast<double>(
                        processMemory.peakWorkingSetBytes) /
                        BytesPerMebibyte,
                    static_cast<double>(
                        processMemory.privateBytes) /
                        BytesPerMebibyte,
                    static_cast<double>(trackedGpuBytes) /
                        BytesPerMebibyte);

                diagnosticsOverlayText = overlayBuffer;

                char message[640]{};

                std::snprintf(
                    message,
                    sizeof(message),
                    "Performance"
                    " | FPS: %.1f"
                    " | Avg: %.3f ms"
                    " | P95: %.3f ms"
                    " | P99: %.3f ms"
                    " | Max: %.3f ms"
                    " | >16.67 ms: %zu/%zu"
                    " | >33.33 ms: %zu/%zu"
                    " | Draw Calls: %zu"
                    " | Submitted: %zu"
                    " | Rendered: %zu"
                    " | Dropped: %zu"
                    " | Capacity: %zu"
                    " | CPU Avg/P95/P99:"
                    " %.3f/%.3f/%.3f ms"
                    " | Present Avg/P95/P99:"
                    " %.3f/%.3f/%.3f ms"
                    " | GPU Avg/P95/P99:"
                    " %.3f/%.3f/%.3f ms"
                    " | GPU Samples: %zu"
                    " | Working Set: %.2f MiB"
                    " | Peak Working Set: %.2f MiB"
                    " | Private Bytes: %.2f MiB"
                    " | Tracked GPU: %.2f MiB\n",
                    framesPerSecond,
                    frameTimeSnapshot.averageMilliseconds,
                    frameTimeSnapshot.percentile95Milliseconds,
                    frameTimeSnapshot.percentile99Milliseconds,
                    frameTimeSnapshot.maximumMilliseconds,
                    frameTimeSnapshot.framesOver16Milliseconds,
                    frameTimeSnapshot.sampleCount,
                    frameTimeSnapshot.framesOver33Milliseconds,
                    frameTimeSnapshot.sampleCount,
                    statistics.drawCallCount,
                    statistics.submittedInstanceCount,
                    statistics.instanceCount,
                    statistics.droppedInstanceCount,
                    statistics.instanceBufferCapacity,
                    cpuSubmissionSnapshot.averageMilliseconds,
                    cpuSubmissionSnapshot.percentile95Milliseconds,
                    cpuSubmissionSnapshot.percentile99Milliseconds,
                    presentSnapshot.averageMilliseconds,
                    presentSnapshot.percentile95Milliseconds,
                    presentSnapshot.percentile99Milliseconds,
                    gpuSnapshot.averageMilliseconds,
                    gpuSnapshot.percentile95Milliseconds,
                    gpuSnapshot.percentile99Milliseconds,
                    gpuSnapshot.sampleCount,
                    static_cast<double>(
                        processMemory.workingSetBytes) /
                        BytesPerMebibyte,
                    static_cast<double>(
                        processMemory.peakWorkingSetBytes) /
                        BytesPerMebibyte,
                    static_cast<double>(
                        processMemory.privateBytes) /
                        BytesPerMebibyte,
                    static_cast<double>(trackedGpuBytes) /
                        BytesPerMebibyte);

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
        (void)renderer_.SetGroundTextureOverride(
            nullptr);

        textureResourceManager_.Stop();
        serverStateReceiver_.Stop();

        return EXIT_SUCCESS;
    }
}
