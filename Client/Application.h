#pragma once

#include "Platform/Window.h"
#include "Rendering/D3D11Renderer.h"

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
        Platform::Window window_;
        Rendering::D3D11Renderer renderer_;

        int showCommand_{};
    };
}