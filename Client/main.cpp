#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <cstdint>
#include <cstdlib>
#include <iterator>

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr wchar_t WindowClassName[] = L"DungeonSyncWindowClass";
    constexpr wchar_t WindowTitle[] = L"DungeonSync - DirectX 11";
    constexpr std::uint32_t ClientWidth = 1280;
    constexpr std::uint32_t ClientHeight = 720;

    struct GraphicsContext
    {
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> deviceContext;
        ComPtr<IDXGISwapChain> swapChain;
        ComPtr<ID3D11RenderTargetView> renderTargetView;
    };

    LRESULT CALLBACK WindowProcedure(
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
            return DefWindowProcW(window, message, wParam, lParam);
        }
    }

    bool CreateApplicationWindow(
        HINSTANCE instance,
        HWND& window)
    {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProcedure;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = WindowClassName;

        if (RegisterClassExW(&windowClass) == 0)
        {
            return false;
        }

        RECT windowRectangle{
            0,
            0,
            static_cast<LONG>(ClientWidth),
            static_cast<LONG>(ClientHeight)
        };

        constexpr DWORD windowStyle =
            WS_OVERLAPPED |
            WS_CAPTION |
            WS_SYSMENU |
            WS_MINIMIZEBOX;

        AdjustWindowRect(&windowRectangle, windowStyle, FALSE);

        const int windowWidth =
            windowRectangle.right - windowRectangle.left;
        const int windowHeight =
            windowRectangle.bottom - windowRectangle.top;

        window = CreateWindowExW(
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
            instance,
            nullptr);

        return window != nullptr;
    }

    bool CreateGraphicsContext(
        HWND window,
        GraphicsContext& graphics)
    {
        DXGI_SWAP_CHAIN_DESC swapChainDescription{};
        swapChainDescription.BufferDesc.Width = ClientWidth;
        swapChainDescription.BufferDesc.Height = ClientHeight;
        swapChainDescription.BufferDesc.Format =
            DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDescription.SampleDesc.Count = 1;
        swapChainDescription.BufferUsage =
            DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDescription.BufferCount = 2;
        swapChainDescription.OutputWindow = window;
        swapChainDescription.Windowed = TRUE;
        swapChainDescription.SwapEffect =
            DXGI_SWAP_EFFECT_DISCARD;

        UINT creationFlags = 0;

#ifdef _DEBUG
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        const D3D_FEATURE_LEVEL requestedFeatureLevels[]{
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        D3D_FEATURE_LEVEL selectedFeatureLevel{};

        HRESULT result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            requestedFeatureLevels,
            static_cast<UINT>(std::size(requestedFeatureLevels)),
            D3D11_SDK_VERSION,
            &swapChainDescription,
            graphics.swapChain.GetAddressOf(),
            graphics.device.GetAddressOf(),
            &selectedFeatureLevel,
            graphics.deviceContext.GetAddressOf());

#ifdef _DEBUG
        if (result == DXGI_ERROR_SDK_COMPONENT_MISSING)
        {
            creationFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

            result = D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                creationFlags,
                requestedFeatureLevels,
                static_cast<UINT>(std::size(requestedFeatureLevels)),
                D3D11_SDK_VERSION,
                &swapChainDescription,
                graphics.swapChain.GetAddressOf(),
                graphics.device.GetAddressOf(),
                &selectedFeatureLevel,
                graphics.deviceContext.GetAddressOf());
        }
#endif

        if (FAILED(result))
        {
            return false;
        }

        ComPtr<ID3D11Texture2D> backBuffer;

        result = graphics.swapChain->GetBuffer(
            0,
            IID_PPV_ARGS(backBuffer.GetAddressOf()));

        if (FAILED(result))
        {
            return false;
        }

        result = graphics.device->CreateRenderTargetView(
            backBuffer.Get(),
            nullptr,
            graphics.renderTargetView.GetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        const D3D11_VIEWPORT viewport{
            0.0F,
            0.0F,
            static_cast<float>(ClientWidth),
            static_cast<float>(ClientHeight),
            0.0F,
            1.0F
        };

        graphics.deviceContext->RSSetViewports(1, &viewport);

        return true;
    }

    void Render(GraphicsContext& graphics)
    {
        constexpr float backgroundColor[]{
            0.03F,
            0.06F,
            0.12F,
            1.0F
        };

        ID3D11RenderTargetView* renderTargets[]{
            graphics.renderTargetView.Get()
        };

        graphics.deviceContext->OMSetRenderTargets(
            1,
            renderTargets,
            nullptr);

        graphics.deviceContext->ClearRenderTargetView(
            graphics.renderTargetView.Get(),
            backgroundColor);

        graphics.swapChain->Present(1, 0);
    }
}

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand)
{
    HWND window{};

    if (!CreateApplicationWindow(instance, window))
    {
        MessageBoxW(
            nullptr,
            L"Win32 창 생성에 실패했습니다.",
            L"DungeonSync Error",
            MB_OK | MB_ICONERROR);

        return EXIT_FAILURE;
    }

    GraphicsContext graphics{};

    if (!CreateGraphicsContext(window, graphics))
    {
        MessageBoxW(
            window,
            L"DirectX 11 초기화에 실패했습니다.",
            L"DungeonSync Error",
            MB_OK | MB_ICONERROR);

        return EXIT_FAILURE;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};

    while (message.message != WM_QUIT)
    {
        if (PeekMessageW(
                &message,
                nullptr,
                0,
                0,
                PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        else
        {
            Render(graphics);
        }
    }

    return static_cast<int>(message.wParam);
}