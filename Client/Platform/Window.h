#pragma once

#include <Windows.h>

#include <cstdint>

namespace DungeonSync::Platform
{
    class Window final
    {
    public:
        Window(
            HINSTANCE instance,
            std::uint32_t clientWidth,
            std::uint32_t clientHeight) noexcept;

        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        [[nodiscard]] bool Initialize(int showCommand);
        [[nodiscard]] bool ProcessMessages();

        [[nodiscard]] HWND Handle() const noexcept;
        [[nodiscard]] std::uint32_t ClientWidth() const noexcept;
        [[nodiscard]] std::uint32_t ClientHeight() const noexcept;

    private:
        static LRESULT CALLBACK WindowProcedure(
            HWND window,
            UINT message,
            WPARAM wParam,
            LPARAM lParam);

        HINSTANCE instance_{};
        HWND handle_{};

        std::uint32_t clientWidth_{};
        std::uint32_t clientHeight_{};

        bool classRegistered_{};
    };
}