#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <cstdint>

namespace DungeonSync::Rendering
{
    class D3D11Renderer final
    {
    public:
        D3D11Renderer() = default;
        ~D3D11Renderer() = default;

        D3D11Renderer(const D3D11Renderer&) = delete;
        D3D11Renderer& operator=(const D3D11Renderer&) = delete;
        D3D11Renderer(D3D11Renderer&&) = delete;
        D3D11Renderer& operator=(D3D11Renderer&&) = delete;

        [[nodiscard]] bool Initialize(
            HWND window,
            std::uint32_t width,
            std::uint32_t height);

        void Render(float totalSeconds);

    private:
        [[nodiscard]] bool CreateDeviceAndSwapChain(
            HWND window,
            std::uint32_t width,
            std::uint32_t height);

        [[nodiscard]] bool CreateRenderTarget();
        [[nodiscard]] bool CreateDepthBuffer();
        [[nodiscard]] bool CreateCubeGeometryBuffers();
        [[nodiscard]] bool CreateShadersAndInputLayout();
        [[nodiscard]] bool CreateConstantBuffer();

        Microsoft::WRL::ComPtr<ID3D11Device> device_;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext_;
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBuffer_;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;

        D3D_FEATURE_LEVEL featureLevel_{};

        std::uint32_t width_{};
        std::uint32_t height_{};
    };
}