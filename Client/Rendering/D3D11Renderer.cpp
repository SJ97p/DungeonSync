#include "D3D11Renderer.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "Vertex.h"
#include "RenderItem.h"
#include <iterator>
#include <cstddef>
#include <cstring>

using Microsoft::WRL::ComPtr;


namespace
{
    const DungeonSync::Rendering::Vertex CubeVertices[]{
    {
        { -0.5F, -0.5F, -0.5F },
        { 1.0F, 0.0F, 0.0F, 1.0F }
    },
    {
        { -0.5F,  0.5F, -0.5F },
        { 0.0F, 1.0F, 0.0F, 1.0F }
    },
    {
        {  0.5F,  0.5F, -0.5F },
        { 0.0F, 0.0F, 1.0F, 1.0F }
    },
    {
        {  0.5F, -0.5F, -0.5F },
        { 1.0F, 1.0F, 0.0F, 1.0F }
    },
    {
        { -0.5F, -0.5F,  0.5F },
        { 1.0F, 0.0F, 1.0F, 1.0F }
    },
    {
        { -0.5F,  0.5F,  0.5F },
        { 0.0F, 1.0F, 1.0F, 1.0F }
    },
    {
        {  0.5F,  0.5F,  0.5F },
        { 1.0F, 1.0F, 1.0F, 1.0F }
    },
    {
        {  0.5F, -0.5F,  0.5F },
        { 0.4F, 0.4F, 0.4F, 1.0F }
    }
    };

    constexpr std::uint16_t CubeIndices[]{
        // 카메라에 가까운 면
        0, 1, 2,
        0, 2, 3,

        // 뒤쪽 면
        4, 7, 6,
        4, 6, 5,

        // 왼쪽 면
        4, 5, 1,
        4, 1, 0,

        // 오른쪽 면
        3, 2, 6,
        3, 6, 7,

        // 위쪽 면
        1, 5, 6,
        1, 6, 2,

        // 아래쪽 면
        4, 0, 3,
        4, 3, 7
    };

    struct alignas(16) ObjectConstants
    {
        DirectX::XMFLOAT4X4 worldViewProjection;
        DirectX::XMFLOAT4 tintColor;
    };

    static_assert(sizeof(ObjectConstants) % 16 == 0);

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

        if (!CreateDepthBuffer())
        {
            return false;
        }

        if (!CreateCubeGeometryBuffers())
        {
            return false;
        }

        if (!CreateConstantBuffer())
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

    bool D3D11Renderer::CreateDepthBuffer()
    {
        D3D11_TEXTURE2D_DESC textureDescription{};
        textureDescription.Width = width_;
        textureDescription.Height = height_;
        textureDescription.MipLevels = 1;
        textureDescription.ArraySize = 1;
        textureDescription.Format =
            DXGI_FORMAT_D24_UNORM_S8_UINT;

        textureDescription.SampleDesc.Count = 1;
        textureDescription.Usage = D3D11_USAGE_DEFAULT;
        textureDescription.BindFlags =
            D3D11_BIND_DEPTH_STENCIL;

        HRESULT result = device_->CreateTexture2D(
            &textureDescription,
            nullptr,
            depthBuffer_.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        result = device_->CreateDepthStencilView(
            depthBuffer_.Get(),
            nullptr,
            depthStencilView_.ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }

    void D3D11Renderer::Render(
        std::span<const RenderItem> renderItems)
    {
        using namespace DirectX;

        const XMVECTOR cameraPosition =
            XMVectorSet(0.0F, 0.0F, -3.0F, 1.0F);

        const XMVECTOR cameraTarget =
            XMVectorZero();

        const XMVECTOR cameraUp =
            XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);

        const XMMATRIX view = XMMatrixLookAtLH(
            cameraPosition,
            cameraTarget,
            cameraUp);

        const float aspectRatio =
            static_cast<float>(width_) /
            static_cast<float>(height_);

        const XMMATRIX projection =
            XMMatrixPerspectiveFovLH(
                XM_PIDIV4,
                aspectRatio,
                0.1F,
                100.0F);

        // 3. Back Buffer를 렌더 타깃으로 연결하고 배경 지우기
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
            //nullptr);
            depthStencilView_.Get());

        deviceContext_->ClearRenderTargetView(
            renderTargetView_.Get(),
            backgroundColor);

        deviceContext_->ClearDepthStencilView(
            depthStencilView_.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0F,
            0);

        // 4. Vertex Buffer와 Index Buffer 연결
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

        deviceContext_->IASetIndexBuffer(
            indexBuffer_.Get(),
            DXGI_FORMAT_R16_UINT,
            0);

        // 5. 정점 입력 형식과 삼각형 조립 방식 설정
        deviceContext_->IASetInputLayout(
            inputLayout_.Get());

        deviceContext_->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 6. Constant Buffer를 Vertex Shader의 b0 슬롯에 연결
        ID3D11Buffer* constantBuffers[]{
            constantBuffer_.Get()
        };

        deviceContext_->VSSetConstantBuffers(
            0,
            1,
            constantBuffers);

        // 7. Vertex Shader와 Pixel Shader 연결
        deviceContext_->VSSetShader(
            vertexShader_.Get(),
            nullptr,
            0);

        deviceContext_->PSSetShader(
            pixelShader_.Get(),
            nullptr,
            0);

        const auto drawCube =
            [&](const RenderItem& item) -> bool
            {
                const XMMATRIX world =
                    XMLoadFloat4x4(&item.world);

                const XMMATRIX worldViewProjection =
                    world * view * projection;

                ObjectConstants constants{};

                XMStoreFloat4x4(
                    &constants.worldViewProjection,
                    worldViewProjection);
                
                constants.tintColor = item.tintColor;

                D3D11_MAPPED_SUBRESOURCE mappedResource{};

                const HRESULT mapResult = deviceContext_->Map(
                    constantBuffer_.Get(),
                    0,
                    D3D11_MAP_WRITE_DISCARD,
                    0,
                    &mappedResource);

                if (FAILED(mapResult))
                {
                    return false;
                }

                std::memcpy(
                    mappedResource.pData,
                    &constants,
                    sizeof(constants));

                deviceContext_->Unmap(
                    constantBuffer_.Get(),
                    0);

                deviceContext_->DrawIndexed(
                    static_cast<UINT>(std::size(CubeIndices)),
                    0,
                    0);

                return true;
            };

        for (const RenderItem& item : renderItems)
        {
            if (!drawCube(item))
            {
                return;
            }
        }

        // 9. 완성된 Back Buffer를 창에 표시
        swapChain_->Present(1, 0);
    }

    bool D3D11Renderer::CreateCubeGeometryBuffers()
    {
        D3D11_BUFFER_DESC vertexBufferDescription{};
        vertexBufferDescription.ByteWidth =
            static_cast<UINT>(sizeof(CubeVertices));

        vertexBufferDescription.Usage =
            D3D11_USAGE_IMMUTABLE;

        vertexBufferDescription.BindFlags =
            D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexInitialData{};
        vertexInitialData.pSysMem = CubeVertices;

        HRESULT result = device_->CreateBuffer(
            &vertexBufferDescription,
            &vertexInitialData,
            vertexBuffer_.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        D3D11_BUFFER_DESC indexBufferDescription{};

        indexBufferDescription.ByteWidth =
            static_cast<UINT>(sizeof(CubeIndices));

        indexBufferDescription.Usage =
            D3D11_USAGE_IMMUTABLE;

        indexBufferDescription.BindFlags =
            D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexInitialData{};
        indexInitialData.pSysMem = CubeIndices;

        result = device_->CreateBuffer(
            &indexBufferDescription,
            &indexInitialData,
            indexBuffer_.ReleaseAndGetAddressOf());

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

    bool D3D11Renderer::CreateConstantBuffer()
    {
        D3D11_BUFFER_DESC bufferDescription{};
        bufferDescription.ByteWidth = sizeof(ObjectConstants);
        bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
        bufferDescription.BindFlags =
            D3D11_BIND_CONSTANT_BUFFER;

        bufferDescription.CPUAccessFlags =
            D3D11_CPU_ACCESS_WRITE;

        const HRESULT result = device_->CreateBuffer(
            &bufferDescription,
            nullptr,
            constantBuffer_.ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }
}
