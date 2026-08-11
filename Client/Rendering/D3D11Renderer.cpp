#include "D3D11Renderer.h"

#include <iterator>

using Microsoft::WRL::ComPtr;

namespace DungeonSync::Rendering
{
    bool D3D11Renderer::Initialize(
        HWND window,
        std::uint32_t width,
        std::uint32_t height)
    {
        if (window == nullptr || width == 0 || height == 0)
        {
            return false;
        }

        width_ = width;
        height_ = height;

        if (!CreateDeviceAndSwapChain(window, width, height))
        {
            return false;
        }

        if (!CreateRenderTarget())
        {
            return false;
        }

        const D3D11_VIEWPORT viewport{
            0.0F,
            0.0F,
            static_cast<float>(width_),
            static_cast<float>(height_),
            0.0F,
            1.0F
        };

        deviceContext_->RSSetViewports(1, &viewport);

        return true;
    }

    bool D3D11Renderer::CreateDeviceAndSwapChain(
        HWND window,
        std::uint32_t width,
        std::uint32_t height)
    {
        DXGI_SWAP_CHAIN_DESC swapChainDescription{};
        swapChainDescription.BufferDesc.Width = width;
        swapChainDescription.BufferDesc.Height = height;
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

        constexpr D3D_FEATURE_LEVEL requestedFeatureLevels[]{
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        HRESULT result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            requestedFeatureLevels,
            static_cast<UINT>(
                std::size(requestedFeatureLevels)),
            D3D11_SDK_VERSION,
            &swapChainDescription,
            swapChain_.ReleaseAndGetAddressOf(),
            device_.ReleaseAndGetAddressOf(),
            &featureLevel_,
            deviceContext_.ReleaseAndGetAddressOf());

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
                static_cast<UINT>(
                    std::size(requestedFeatureLevels)),
                D3D11_SDK_VERSION,
                &swapChainDescription,
                swapChain_.ReleaseAndGetAddressOf(),
                device_.ReleaseAndGetAddressOf(),
                &featureLevel_,
                deviceContext_.ReleaseAndGetAddressOf());
        }
#endif

        return SUCCEEDED(result);
    }

    bool D3D11Renderer::CreateRenderTarget()
    {
        ComPtr<ID3D11Texture2D> backBuffer;

        HRESULT result = swapChain_->GetBuffer(
            0,
            IID_PPV_ARGS(
                backBuffer.ReleaseAndGetAddressOf()));

        if (FAILED(result))
        {
            return false;
        }

        result = device_->CreateRenderTargetView(
            backBuffer.Get(),
            nullptr,
            renderTargetView_.ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }

    void D3D11Renderer::Render()
    {
        constexpr float backgroundColor[]{
            0.03F,
            0.06F,
            0.12F,
            1.0F
        };

        ID3D11RenderTargetView* renderTargets[]{
            renderTargetView_.Get()
        };

        deviceContext_->OMSetRenderTargets(
            1,
            renderTargets,
            nullptr);

        deviceContext_->ClearRenderTargetView(
            renderTargetView_.Get(),
            backgroundColor);

        swapChain_->Present(1, 0);
    }
}