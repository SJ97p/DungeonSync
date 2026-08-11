#include "D3D11Renderer.h"
#include <d3dcompiler.h>
#include "Vertex.h"
#include <iterator>
#include <cstddef>

using Microsoft::WRL::ComPtr;


namespace
{
    const DungeonSync::Rendering::Vertex TriangleVertices[]{
        {
            { 0.0F, 0.5F, 0.0F },
            { 1.0F, 0.0F, 0.0F, 1.0F }
        },
        {
            { 0.5F, -0.5F, 0.0F },
            { 0.0F, 1.0F, 0.0F, 1.0F }
        },
        {
            { -0.5F, -0.5F, 0.0F },
            { 0.0F, 0.0F, 1.0F, 1.0F }
        }
    };

    HRESULT CompileShader(
        const wchar_t* filePath,
        const char* entryPoint,
        const char* target,
        ComPtr<ID3DBlob>& bytecode)
    {
        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG;
        compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ComPtr<ID3DBlob> diagnostics;

        const HRESULT result = D3DCompileFromFile(
            filePath,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint,
            target,
            compileFlags,
            0,
            bytecode.ReleaseAndGetAddressOf(),
            diagnostics.ReleaseAndGetAddressOf());

        if (diagnostics != nullptr)
        {
            const auto* message = static_cast<const char*>(
                diagnostics->GetBufferPointer());

            OutputDebugStringA(message);
        }

        return result;
    }
}

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

        if (!CreateTriangleVertexBuffer())
        {
            return false;
        }

        if (!CreateShadersAndInputLayout())
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

        ID3D11Buffer* vertexBuffers[]{
        vertexBuffer_.Get()
        };

        constexpr UINT stride = sizeof(Vertex);
        constexpr UINT offset = 0;

        deviceContext_->IASetVertexBuffers(
            0,
            1,
            vertexBuffers,
            &stride,
            &offset);

        deviceContext_->IASetInputLayout(
            inputLayout_.Get());

        deviceContext_->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        deviceContext_->VSSetShader(
            vertexShader_.Get(),
            nullptr,
            0);

        deviceContext_->PSSetShader(
            pixelShader_.Get(),
            nullptr,
            0);

        deviceContext_->Draw(3, 0);

        swapChain_->Present(1, 0);
    }

    bool D3D11Renderer::CreateTriangleVertexBuffer()
    {
        D3D11_BUFFER_DESC bufferDescription{};
        bufferDescription.ByteWidth =
            static_cast<UINT>(sizeof(TriangleVertices));

        bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
        bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = TriangleVertices;

        const HRESULT result = device_->CreateBuffer(
            &bufferDescription,
            &initialData,
            vertexBuffer_.ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }

    bool D3D11Renderer::CreateShadersAndInputLayout()
    {
        constexpr wchar_t ShaderPath[] =
            L"Client/Rendering/Shaders/Triangle.hlsl";

        ComPtr<ID3DBlob> vertexShaderBytecode;
        ComPtr<ID3DBlob> pixelShaderBytecode;

        HRESULT result = CompileShader(
            ShaderPath,
            "VSMain",
            "vs_5_0",
            vertexShaderBytecode);

        if (FAILED(result))
        {
            return false;
        }

        result = CompileShader(
            ShaderPath,
            "PSMain",
            "ps_5_0",
            pixelShaderBytecode);

        if (FAILED(result))
        {
            return false;
        }

        result = device_->CreateVertexShader(
            vertexShaderBytecode->GetBufferPointer(),
            vertexShaderBytecode->GetBufferSize(),
            nullptr,
            vertexShader_.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        result = device_->CreatePixelShader(
            pixelShaderBytecode->GetBufferPointer(),
            pixelShaderBytecode->GetBufferSize(),
            nullptr,
            pixelShader_.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        const D3D11_INPUT_ELEMENT_DESC inputElements[]{
            {
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                static_cast<UINT>(offsetof(Vertex, position)),
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            {
                "COLOR",
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                0,
                static_cast<UINT>(offsetof(Vertex, color)),
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        };

        result = device_->CreateInputLayout(
            inputElements,
            static_cast<UINT>(std::size(inputElements)),
            vertexShaderBytecode->GetBufferPointer(),
            vertexShaderBytecode->GetBufferSize(),
            inputLayout_.ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }
}