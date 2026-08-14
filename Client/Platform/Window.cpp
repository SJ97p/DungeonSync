#include "Window.h"

namespace
{
    constexpr wchar_t WindowClassName[] =
        L"DungeonSyncWindowClass";

    constexpr wchar_t WindowTitle[] =
        L"DungeonSync - DirectX 11";
}

namespace DungeonSync::Platform
{
    Window::Window(
        HINSTANCE instance,
        std::uint32_t clientWidth,
        std::uint32_t clientHeight) noexcept
        : instance_(instance),
        clientWidth_(clientWidth),
        clientHeight_(clientHeight)
    {
    }

    Window::~Window()
    {
        if (handle_ != nullptr && IsWindow(handle_))
        {
            DestroyWindow(handle_);
        }

        if (classRegistered_)
        {
            UnregisterClassW(WindowClassName, instance_);
        }
    }

    bool Window::Initialize(int showCommand)
    {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProcedure;
        windowClass.hInstance = instance_;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = WindowClassName;

        if (RegisterClassExW(&windowClass) == 0)
        {
            return false;
        }

        classRegistered_ = true;

        constexpr DWORD windowStyle =
            WS_OVERLAPPED |
            WS_CAPTION |
            WS_SYSMENU |
            WS_MINIMIZEBOX;

        RECT windowRectangle{
            0,
            0,
            static_cast<LONG>(clientWidth_),
            static_cast<LONG>(clientHeight_)
        };

        if (!AdjustWindowRect(
            &windowRectangle,
            windowStyle,
            FALSE))
        {
            return false;
        }

        const int windowWidth =
            windowRectangle.right - windowRectangle.left;

        const int windowHeight =
            windowRectangle.bottom - windowRectangle.top;

        handle_ = CreateWindowExW(
            0,
            WindowClassName,
            WindowTitle,
            windowStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowWidth,
            windowHeight,
            nullptr,
            nullptr,
            instance_,
            nullptr);

        if (handle_ == nullptr)
        {
            return false;
        }

        ShowWindow(handle_, showCommand);
        UpdateWindow(handle_);

        return true;
    }

    bool Window::ProcessMessages()
    {
        MSG message{};

        while (PeekMessageW(
            &message,
            nullptr,
            0,
            0,
            PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        return true;
    }

    HWND Window::Handle() const noexcept
    {
        return handle_;
    }

    std::uint32_t Window::ClientWidth() const noexcept
    {
        return clientWidth_;
    }

    std::uint32_t Window::ClientHeight() const noexcept
    {
        return clientHeight_;
    }

    void Window::SetTitle(
        const wchar_t* title) noexcept
    {
        if (handle_ == nullptr ||
            title == nullptr)
        {
            return;
        }

        SetWindowTextW(
            handle_,
            title);
    }

    LRESULT CALLBACK Window::WindowProcedure(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
    {
        switch (message)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(
                window,
                message,
                wParam,
                lParam);
        }
    }
}