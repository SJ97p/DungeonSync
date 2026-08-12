#include "D3D11Renderer.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "Vertex.h"
#include "RenderItem.h"
#include <iterator>
#include <cstddef>
#include <cstring>
#include <algorithm>

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

    struct alignas(16) SceneConstants
    {
        DirectX::XMFLOAT4X4 viewProjection;
    };

    static_assert(sizeof(SceneConstants) % 16 == 0);

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

        if (!CreateInstanceBuffer())
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
        const Camera& camera,
        std::span<const RenderItem> renderItems)
    {
        using namespace DirectX;

        const XMVECTOR cameraPosition =
            XMLoadFloat3(&camera.position);

        const XMVECTOR cameraTarget =
            XMLoadFloat3(&camera.target);

        const XMVECTOR cameraUp =
            XMLoadFloat3(&camera.up);

        const XMMATRIX view = XMMatrixLookAtLH(
            cameraPosition,
            cameraTarget,
            cameraUp);

        const float aspectRatio =
            static_cast<float>(width_) /
            static_cast<float>(height_);

        const XMMATRIX projection =
            XMMatrixPerspectiveFovLH(
                camera.fieldOfViewRadians,
                aspectRatio,
                camera.nearPlane,
                camera.farPlane);

        const XMMATRIX viewProjection =
            view * projection;

        SceneConstants sceneConstants{};

        XMStoreFloat4x4(
            &sceneConstants.viewProjection,
            viewProjection);

        D3D11_MAPPED_SUBRESOURCE
            mappedSceneConstants{};

        HRESULT result = deviceContext_->Map(
            constantBuffer_.Get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mappedSceneConstants);

        if (FAILED(result))
        {
            return;
        }

        std::memcpy(
            mappedSceneConstants.pData,
            &sceneConstants,
            sizeof(sceneConstants));

        deviceContext_->Unmap(
            constantBuffer_.Get(),
            0);

        const std::size_t instanceCount =
            std::min(
                renderItems.size(),
                MaxInstanceCount);

        if (instanceCount == 0)
        {
            swapChain_->Present(1, 0);
            return;
        }

        D3D11_MAPPED_SUBRESOURCE
            mappedInstances{};

        result = deviceContext_->Map(
            instanceBuffer_.Get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mappedInstances);

        if (FAILED(result))
        {
            return;
        }

        auto* instanceData =
            static_cast<InstanceData*>(
                mappedInstances.pData);

        for (std::size_t index = 0;
            index < instanceCount;
            ++index)
        {
            const RenderItem& item =
                renderItems[index];

            instanceData[index].worldRow0 =
                DirectX::XMFLOAT4{
                    item.world._11,
                    item.world._12,
                    item.world._13,
                    item.world._14
            };

            instanceData[index].worldRow1 =
                DirectX::XMFLOAT4{
                    item.world._21,
                    item.world._22,
                    item.world._23,
                    item.world._24
            };

            instanceData[index].worldRow2 =
                DirectX::XMFLOAT4{
                    item.world._31,
                    item.world._32,
                    item.world._33,
                    item.world._34
            };

            instanceData[index].worldRow3 =
                DirectX::XMFLOAT4{
                    item.world._41,
                    item.world._42,
                    item.world._43,
                    item.world._44
            };

            instanceData[index].tintColor =
                item.tintColor;
        }

        deviceContext_->Unmap(
            instanceBuffer_.Get(),
            0);

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
            vertexBuffer_.Get(),
            instanceBuffer_.Get()
        };


        constexpr UINT strides[]{
            sizeof(Vertex),
            sizeof(InstanceData)
        };
        constexpr UINT offsets[]{
            0,
            0
        };

        deviceContext_->IASetVertexBuffers(
            0,
            2,
            vertexBuffers,
            strides,
            offsets);

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

        deviceContext_->DrawIndexedInstanced(
            static_cast<UINT>(
                std::size(CubeIndices)),
            static_cast<UINT>(
                instanceCount),
            0,
            0,
            0);

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
            },
{
    "INSTANCE_WORLD",
    0,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    1,
    static_cast<UINT>(
        offsetof(InstanceData, worldRow0)),
    D3D11_INPUT_PER_INSTANCE_DATA,
    1
},
{
    "INSTANCE_WORLD",
    1,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    1,
    static_cast<UINT>(
        offsetof(InstanceData, worldRow1)),
    D3D11_INPUT_PER_INSTANCE_DATA,
    1
},
{
    "INSTANCE_WORLD",
    2,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    1,
    static_cast<UINT>(
        offsetof(InstanceData, worldRow2)),
    D3D11_INPUT_PER_INSTANCE_DATA,
    1
},
{
    "INSTANCE_WORLD",
    3,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    1,
    static_cast<UINT>(
        offsetof(InstanceData, worldRow3)),
    D3D11_INPUT_PER_INSTANCE_DATA,
    1
},
{
    "INSTANCE_COLOR",
    0,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    1,
    static_cast<UINT>(
        offsetof(InstanceData, tintColor)),
    D3D11_INPUT_PER_INSTANCE_DATA,
    1
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
        bufferDescription.ByteWidth =
            sizeof(SceneConstants);
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

    bool D3D11Renderer::CreateInstanceBuffer()
    {
        D3D11_BUFFER_DESC bufferDescription{};

        bufferDescription.ByteWidth =
            static_cast<UINT>(
                sizeof(InstanceData) *
                MaxInstanceCount);

        bufferDescription.Usage =
            D3D11_USAGE_DYNAMIC;

        bufferDescription.BindFlags =
            D3D11_BIND_VERTEX_BUFFER;

        bufferDescription.CPUAccessFlags =
            D3D11_CPU_ACCESS_WRITE;

        const HRESULT result = device_->CreateBuffer(
            &bufferDescription,
            nullptr,
            instanceBuffer_.ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }
}
