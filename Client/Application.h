#pragma once

#include "Platform/Window.h"
#include "Rendering/D3D11Renderer.h"
#include "Presentation/DemoScene.h"
#include "Network/TcpClient.h"
#include "Network/ServerStateReceiver.h"
#include "../Shared/Network/WinsockRuntime.h"
#include "Diagnostics/FrameTimeProfiler.h"
#include "Diagnostics/RenderingStressScene.h"

#include <WinSock2.h>
#include <Windows.h>

namespace DungeonSync
{
    class Application final
    {
    public:
        Application(
            HINSTANCE instance,
            int showCommand) noexcept;

        [[nodiscard]] int Run();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

    private:
        Network::WinsockRuntime winsock_;
        Network::TcpClient tcpClient_;
        Network::ServerStateReceiver
            serverStateReceiver_;

        Platform::Window window_;
        Rendering::D3D11Renderer renderer_;
        Diagnostics::FrameTimeProfiler frameTimeProfiler_;
        Diagnostics::FrameTimeProfiler
            cpuSubmissionProfiler_;
        Diagnostics::FrameTimeProfiler
            presentProfiler_;
        Diagnostics::FrameTimeProfiler
            gpuProfiler_;
        Diagnostics::RenderingStressScene renderingStressScene_;
        Presentation::DemoScene demoScene_;

        int showCommand_{};
    };
}