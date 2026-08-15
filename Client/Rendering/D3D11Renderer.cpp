#include "D3D11Renderer.h"
#include "Vertex.h"
#include "RenderItem.h"

#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <filesystem>
#include <cstdio>
#include <iterator>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <limits>
#include <chrono>

using Microsoft::WRL::ComPtr;


namespace
{
    const DungeonSync::Rendering::Vertex
        SpriteVertices[]{
        {
            { -0.5F, 0.0F, 0.0F },
            { 0.0F, 1.0F }
        },
        {
            { -0.5F, 1.0F, 0.0F },
            { 0.0F, 0.0F }
        },
        {
            { 0.5F, 1.0F, 0.0F },
            { 1.0F, 0.0F }
        },
        {
            { 0.5F, 0.0F, 0.0F },
            { 1.0F, 1.0F }
        }
    };

    constexpr std::uint16_t SpriteIndices[]{
        0, 1, 2,
        0, 2, 3
    };

    const DungeonSync::Rendering::Vertex
        GroundVertices[]{
        {
            { -8.0F, 0.0F, -3.0F },
            { 0.0F, 0.0F }
        },
        {
            { -8.0F, 0.0F, 5.0F },
            { 0.0F, 3.0F }
        },
        {
            { 8.0F, 0.0F, 5.0F },
            { 6.0F, 3.0F }
        },
        {
            { 8.0F, 0.0F, -3.0F },
            { 6.0F, 0.0F }
        }
    };

    constexpr std::uint16_t GroundIndices[]{
        0, 1, 2,
        0, 2, 3
    };

    const DungeonSync::Rendering::Vertex
        BackgroundVertices[]{
        {
            { -8.0F, -4.0F, 2.0F },
            { 0.0F, 1.0F }
        },
        {
            { -8.0F, 4.0F, 2.0F },
            { 0.0F, 0.0F }
        },
        {
            { 8.0F, 4.0F, 2.0F },
            { 1.0F, 0.0F }
        },
        {
            { 8.0F, -4.0F, 2.0F },
            { 1.0F, 1.0F }
        }
    };

    constexpr std::uint16_t BackgroundIndices[]{
        0, 1, 2,
        0, 2, 3
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

        if (!textureLoader_.Initialize())
        {
            OutputDebugStringA(
                "Failed to initialize WIC texture loader.\n");

            return false;
        }

        if (!CreateTextureResources())
        {
            OutputDebugStringA(
                "Failed to create texture resources.\n");

            return false;
        }

        if (!CreateSpriteSampler())
        {
            OutputDebugStringA(
                "Failed to create sprite sampler.\n");

            return false;
        }

        if (!CreateGroundSampler())
        {
            OutputDebugStringA(
                "Failed to create ground sampler.\n");

            return false;
        }

        if (!CreateSpriteBlendState())
        {
            OutputDebugStringA(
                "Failed to create sprite blend state.\n");

            return false;
        }

        if (!CreateGpuTimingQueries())
        {
            OutputDebugStringA(
                "Failed to create GPU timing queries.\n");

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

        if (!CreateSpriteGeometryBuffers())
        {
            return false;
        }

        if (!CreateGroundGeometryBuffers())
        {
            return false;
        }

        if (!CreateBackgroundGeometryBuffers())
        {
            return false;
        }

        if (!CreateConstantBuffer())
        {
            return false;
        }

        if (!EnsureInstanceBufferCapacity(
            InitialInstanceCapacity))
        {
            return false;
        }

        if (!CreateShadersAndInputLayout())
        {
            return false;
        }

        if (!CreateGroundShadersAndInputLayout())
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

    bool D3D11Renderer::CreateTextureResources()
    {
        const std::filesystem::path playerTexturePath{
            L"Assets/Textures/Sprites/"
            L"dungeon_sprite_atlas.png"
        };

        if (!textureLoader_.LoadFromFile(
            *device_.Get(),
            playerTexturePath,
            spriteAtlas_))
        {
            OutputDebugStringW(
                L"Failed to load player texture: ");

            OutputDebugStringW(
                playerTexturePath.c_str());

            OutputDebugStringW(L"\n");

            return false;
        }

        const std::filesystem::path groundTexturePath{
    L"Assets/Textures/Environment/"
    L"floor_stone_a.png"
        };

        if (!textureLoader_.LoadFromFile(
            *device_.Get(),
            groundTexturePath,
            groundTexture_))
        {
            OutputDebugStringW(
                L"Failed to load ground texture: ");

            OutputDebugStringW(
                groundTexturePath.c_str());

            OutputDebugStringW(L"\n");

            return false;
        }

        const std::filesystem::path
            backgroundTexturePath{
                L"Assets/Textures/Environment/"
                L"aqueduct_far_background.png"
        };

        if (!textureLoader_.LoadFromFile(
            *device_.Get(),
            backgroundTexturePath,
            backgroundTexture_))
        {
            OutputDebugStringW(
                L"Failed to load background texture: ");

            OutputDebugStringW(
                backgroundTexturePath.c_str());

            OutputDebugStringW(L"\n");

            return false;
        }

        char message[128]{};

        std::snprintf(
            message,
            sizeof(message),
            "Loaded sprite atlas"
            " | width: %u"
            " | height: %u\n",
            spriteAtlas_.width,
            spriteAtlas_.height);

        OutputDebugStringA(message);

        std::snprintf(
            message,
            sizeof(message),
            "Loaded ground texture"
            " | width: %u"
            " | height: %u\n",
            groundTexture_.width,
            groundTexture_.height);

        OutputDebugStringA(message);

        std::snprintf(
            message,
            sizeof(message),
            "Loaded background texture"
            " | width: %u"
            " | height: %u\n",
            backgroundTexture_.width,
            backgroundTexture_.height);

        OutputDebugStringA(message);

        return true;
    }

    bool D3D11Renderer::CreateSpriteSampler()
    {
        D3D11_SAMPLER_DESC description{};

        description.Filter =
            D3D11_FILTER_MIN_MAG_MIP_LINEAR;

        description.AddressU =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        description.AddressV =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        description.AddressW =
            D3D11_TEXTURE_ADDRESS_CLAMP;

        description.MinLOD = 0.0F;
        description.MaxLOD =
            D3D11_FLOAT32_MAX;

        const HRESULT result =
            device_->CreateSamplerState(
                &description,
                spriteSampler_
                .ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }

    bool D3D11Renderer::CreateGroundSampler()
    {
        D3D11_SAMPLER_DESC description{};

        description.Filter =
            D3D11_FILTER_MIN_MAG_MIP_LINEAR;

        description.AddressU =
            D3D11_TEXTURE_ADDRESS_WRAP;

        description.AddressV =
            D3D11_TEXTURE_ADDRESS_WRAP;

        description.AddressW =
            D3D11_TEXTURE_ADDRESS_WRAP;

        description.MinLOD = 0.0F;
        description.MaxLOD =
            D3D11_FLOAT32_MAX;

        const HRESULT result =
            device_->CreateSamplerState(
                &description,
                groundSampler_
                .ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }

    bool D3D11Renderer::CreateSpriteBlendState()
    {
        D3D11_BLEND_DESC description{};

        description.RenderTarget[0]
            .BlendEnable = TRUE;

        description.RenderTarget[0]
            .SrcBlend = D3D11_BLEND_SRC_ALPHA;

        description.RenderTarget[0]
            .DestBlend =
            D3D11_BLEND_INV_SRC_ALPHA;

        description.RenderTarget[0]
            .BlendOp = D3D11_BLEND_OP_ADD;

        description.RenderTarget[0]
            .SrcBlendAlpha = D3D11_BLEND_ONE;

        description.RenderTarget[0]
            .DestBlendAlpha = D3D11_BLEND_ZERO;

        description.RenderTarget[0]
            .BlendOpAlpha = D3D11_BLEND_OP_ADD;

        description.RenderTarget[0]
            .RenderTargetWriteMask =
            D3D11_COLOR_WRITE_ENABLE_ALL;

        const HRESULT result =
            device_->CreateBlendState(
                &description,
                spriteBlendState_
                .ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }

    void D3D11Renderer::Render(
        const Camera& camera,
        std::span<const RenderItem> renderItems,
        SpriteSubmissionMode submissionMode)
    {
        using namespace DirectX;
        statistics_ = {};

        ResolveGpuTimingQueries();

        statistics_.gpuMilliseconds =
            latestGpuMilliseconds_;

        statistics_.gpuTimingValid =
            latestGpuTimingValid_;

        statistics_.gpuTimingSampleSerial =
            gpuTimingSampleSerial_;

        const auto renderSubmissionStart =
            std::chrono::steady_clock::now();

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

        const float orthographicWidth =
            camera.orthographicHeight *
            aspectRatio;

        const XMMATRIX projection =
            XMMatrixOrthographicLH(
                orthographicWidth,
                camera.orthographicHeight,
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

        statistics_.submittedInstanceCount =
            renderItems.size();

        if (!EnsureInstanceBufferCapacity(
            renderItems.size()))
        {
            OutputDebugStringA(
                "Failed to grow instance buffer."
                " Rendering available instances only.\n");
        }

        const std::size_t instanceCount =
            (std::min)(
                renderItems.size(),
                instanceBufferCapacity_);

        statistics_.instanceCount =
            instanceCount;

        statistics_.droppedInstanceCount =
            renderItems.size() -
            instanceCount;

        statistics_.instanceBufferCapacity =
            instanceBufferCapacity_;

        if (instanceCount == 0)
        {
            const auto renderSubmissionEnd =
                std::chrono::steady_clock::now();

            statistics_.cpuSubmissionMilliseconds =
                std::chrono::duration<
                    float,
                    std::milli>(
                        renderSubmissionEnd -
                        renderSubmissionStart)
                    .count();

            const auto presentStart =
                std::chrono::steady_clock::now();

            swapChain_->Present(
                presentSyncInterval_,
                0);

            const auto presentEnd =
                std::chrono::steady_clock::now();

            statistics_.presentMilliseconds =
                std::chrono::duration<
                float,
                std::milli>(
                    presentEnd -
                    presentStart)
                .count();
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

            instanceData[index].uvRectangle =
                item.uvRectangle;
        }

        deviceContext_->Unmap(
            instanceBuffer_.Get(),
            0);

        const bool gpuTimingStarted =
            BeginGpuTimingQuery();

        (void)gpuTimingStarted;

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

        deviceContext_->OMSetBlendState(
            nullptr,
            nullptr,
            0xFFFFFFFF);

        // Background pass
        ID3D11Buffer* backgroundVertexBuffers[]{
            backgroundVertexBuffer_.Get()
        };

        constexpr UINT backgroundStrides[]{
            sizeof(Vertex)
        };

        constexpr UINT backgroundOffsets[]{
            0
        };

        deviceContext_->IASetVertexBuffers(
            0,
            1,
            backgroundVertexBuffers,
            backgroundStrides,
            backgroundOffsets);

        deviceContext_->IASetIndexBuffer(
            backgroundIndexBuffer_.Get(),
            DXGI_FORMAT_R16_UINT,
            0);

        deviceContext_->IASetInputLayout(
            groundInputLayout_.Get());

        deviceContext_->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D11Buffer* backgroundConstantBuffers[]{
            constantBuffer_.Get()
        };

        deviceContext_->VSSetConstantBuffers(
            0,
            1,
            backgroundConstantBuffers);

        deviceContext_->VSSetShader(
            groundVertexShader_.Get(),
            nullptr,
            0);

        deviceContext_->PSSetShader(
            groundPixelShader_.Get(),
            nullptr,
            0);

        ID3D11ShaderResourceView*
            backgroundShaderResources[]{
                backgroundTexture_
                    .shaderResourceView
                    .Get()
        };

        deviceContext_->PSSetShaderResources(
            0,
            1,
            backgroundShaderResources);

        ID3D11SamplerState* backgroundSamplers[]{
            spriteSampler_.Get()
        };

        deviceContext_->PSSetSamplers(
            0,
            1,
            backgroundSamplers);

        deviceContext_->DrawIndexed(
            static_cast<UINT>(
                std::size(BackgroundIndices)),
            0,
            0);

        ++statistics_.drawCallCount;

        // Ground pass
        ID3D11Buffer* groundVertexBuffers[]{
            groundVertexBuffer_.Get()
        };

        constexpr UINT groundStrides[]{
            sizeof(Vertex)
        };

        constexpr UINT groundOffsets[]{
            0
        };

        deviceContext_->IASetVertexBuffers(
            0,
            1,
            groundVertexBuffers,
            groundStrides,
            groundOffsets);

        deviceContext_->IASetIndexBuffer(
            groundIndexBuffer_.Get(),
            DXGI_FORMAT_R16_UINT,
            0);

        deviceContext_->IASetInputLayout(
            groundInputLayout_.Get());

        deviceContext_->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D11Buffer* groundConstantBuffers[]{
            constantBuffer_.Get()
        };

        deviceContext_->VSSetConstantBuffers(
            0,
            1,
            groundConstantBuffers);

        deviceContext_->VSSetShader(
            groundVertexShader_.Get(),
            nullptr,
            0);

        deviceContext_->PSSetShader(
            groundPixelShader_.Get(),
            nullptr,
            0);

        ID3D11ShaderResourceView*
            groundShaderResources[]{
                groundTexture_
                    .shaderResourceView
                    .Get()
        };

        deviceContext_->PSSetShaderResources(
            0,
            1,
            groundShaderResources);

        ID3D11SamplerState* groundSamplers[]{
            groundSampler_.Get()
        };

        deviceContext_->PSSetSamplers(
            0,
            1,
            groundSamplers);

        deviceContext_->DrawIndexed(
            static_cast<UINT>(
                std::size(GroundIndices)),
            0,
            0);

        ++statistics_.drawCallCount;

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

        ID3D11ShaderResourceView*
            shaderResources[]{
                spriteAtlas_
                    .shaderResourceView
                    .Get()
        };

        deviceContext_->PSSetShaderResources(
            0,
            1,
            shaderResources);

        ID3D11SamplerState* samplers[]{
            spriteSampler_.Get()
        };

        deviceContext_->PSSetSamplers(
            0,
            1,
            samplers);

        constexpr float blendFactor[]{
            0.0F,
            0.0F,
            0.0F,
            0.0F
        };

        deviceContext_->OMSetBlendState(
            spriteBlendState_.Get(),
            blendFactor,
            0xFFFFFFFF);

        const UINT spriteIndexCount =
            static_cast<UINT>(
                std::size(SpriteIndices));

        if (submissionMode ==
            SpriteSubmissionMode::InstancedBatch)
        {
            deviceContext_->DrawIndexedInstanced(
                spriteIndexCount,
                static_cast<UINT>(instanceCount),
                0,
                0,
                0);

            ++statistics_.drawCallCount;
        }
        else
        {
            for (std::size_t instanceIndex = 0;
                instanceIndex < instanceCount;
                ++instanceIndex)
            {
                deviceContext_->DrawIndexedInstanced(
                    spriteIndexCount,
                    1,
                    0,
                    0,
                    static_cast<UINT>(
                        instanceIndex));

                ++statistics_.drawCallCount;
            }
        }

        EndGpuTimingQuery();

        // Measure CPU render command submission.
        const auto renderSubmissionEnd =
            std::chrono::steady_clock::now();

        statistics_.cpuSubmissionMilliseconds =
            std::chrono::duration<
            float,
            std::milli>(
                renderSubmissionEnd -
                renderSubmissionStart)
            .count();

        // Measure Present wait separately.
        const auto presentStart =
            std::chrono::steady_clock::now();

        swapChain_->Present(
            presentSyncInterval_,
            0);

        const auto presentEnd =
            std::chrono::steady_clock::now();

        statistics_.presentMilliseconds =
            std::chrono::duration<
            float,
            std::milli>(
                presentEnd -
                presentStart)
            .count();
    }

    const RenderStatistics&
        D3D11Renderer::Statistics() const noexcept
    {
        return statistics_;
    }

    void D3D11Renderer::SetVSyncEnabled(
        bool enabled) noexcept
    {
        presentSyncInterval_ =
            enabled ? 1U : 0U;
    }

    bool D3D11Renderer::IsVSyncEnabled()
        const noexcept
    {
        return presentSyncInterval_ != 0;
    }

    void D3D11Renderer::
        BeginGpuTimingGeneration() noexcept
    {
        ++gpuTimingGeneration_;

        if (gpuTimingGeneration_ == 0)
        {
            gpuTimingGeneration_ = 1;
        }

        latestGpuMilliseconds_ = 0.0F;
        latestGpuTimingValid_ = false;
    }

    bool D3D11Renderer::CreateSpriteGeometryBuffers()
    {
        D3D11_BUFFER_DESC vertexBufferDescription{};
        vertexBufferDescription.ByteWidth =
            static_cast<UINT>(sizeof(SpriteVertices));

        vertexBufferDescription.Usage =
            D3D11_USAGE_IMMUTABLE;

        vertexBufferDescription.BindFlags =
            D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexInitialData{};
        vertexInitialData.pSysMem = SpriteVertices;

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
            static_cast<UINT>(sizeof(SpriteIndices));

        indexBufferDescription.Usage =
            D3D11_USAGE_IMMUTABLE;

        indexBufferDescription.BindFlags =
            D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexInitialData{};
        indexInitialData.pSysMem = SpriteIndices;

        result = device_->CreateBuffer(
            &indexBufferDescription,
            &indexInitialData,
            indexBuffer_.ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }

    bool D3D11Renderer::CreateGroundGeometryBuffers()
    {
        D3D11_BUFFER_DESC vertexBufferDescription{};

        vertexBufferDescription.ByteWidth =
            static_cast<UINT>(
                sizeof(GroundVertices));

        vertexBufferDescription.Usage =
            D3D11_USAGE_IMMUTABLE;

        vertexBufferDescription.BindFlags =
            D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexInitialData{};
        vertexInitialData.pSysMem =
            GroundVertices;

        HRESULT result = device_->CreateBuffer(
            &vertexBufferDescription,
            &vertexInitialData,
            groundVertexBuffer_
            .ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        D3D11_BUFFER_DESC indexBufferDescription{};

        indexBufferDescription.ByteWidth =
            static_cast<UINT>(
                sizeof(GroundIndices));

        indexBufferDescription.Usage =
            D3D11_USAGE_IMMUTABLE;

        indexBufferDescription.BindFlags =
            D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexInitialData{};
        indexInitialData.pSysMem =
            GroundIndices;

        result = device_->CreateBuffer(
            &indexBufferDescription,
            &indexInitialData,
            groundIndexBuffer_
            .ReleaseAndGetAddressOf());

        return SUCCEEDED(result);
    }

    bool D3D11Renderer::
        CreateBackgroundGeometryBuffers()
    {
        D3D11_BUFFER_DESC vertexDescription{};

        vertexDescription.ByteWidth =
            static_cast<UINT>(
                sizeof(BackgroundVertices));

        vertexDescription.Usage =
            D3D11_USAGE_IMMUTABLE;

        vertexDescription.BindFlags =
            D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexData{};
        vertexData.pSysMem =
            BackgroundVertices;

        HRESULT result = device_->CreateBuffer(
            &vertexDescription,
            &vertexData,
            backgroundVertexBuffer_
            .ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        D3D11_BUFFER_DESC indexDescription{};

        indexDescription.ByteWidth =
            static_cast<UINT>(
                sizeof(BackgroundIndices));

        indexDescription.Usage =
            D3D11_USAGE_IMMUTABLE;

        indexDescription.BindFlags =
            D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexData{};
        indexData.pSysMem =
            BackgroundIndices;

        result = device_->CreateBuffer(
            &indexDescription,
            &indexData,
            backgroundIndexBuffer_
            .ReleaseAndGetAddressOf());

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
                "TEXCOORD",
                0,
                DXGI_FORMAT_R32G32_FLOAT,
                0,
                static_cast<UINT>(
                    offsetof(
                        Vertex,
                        textureCoordinate)),
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
            },
            {
                "INSTANCE_UV",
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                1,
                static_cast<UINT>(
                    offsetof(
                        InstanceData,
                        uvRectangle)),
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

    bool D3D11Renderer::
        CreateGroundShadersAndInputLayout()
    {
        constexpr wchar_t ShaderPath[] =
            L"Client/Rendering/Shaders/Ground.hlsl";

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
            groundVertexShader_
            .ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        result = device_->CreatePixelShader(
            pixelShaderBytecode->GetBufferPointer(),
            pixelShaderBytecode->GetBufferSize(),
            nullptr,
            groundPixelShader_
            .ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        const D3D11_INPUT_ELEMENT_DESC
            inputElements[]{
            {
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                static_cast<UINT>(
                    offsetof(Vertex, position)),
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            {
                "TEXCOORD",
                0,
                DXGI_FORMAT_R32G32_FLOAT,
                0,
                static_cast<UINT>(
                    offsetof(
                        Vertex,
                        textureCoordinate)),
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            }
        };

        result = device_->CreateInputLayout(
            inputElements,
            static_cast<UINT>(
                std::size(inputElements)),
            vertexShaderBytecode->GetBufferPointer(),
            vertexShaderBytecode->GetBufferSize(),
            groundInputLayout_
            .ReleaseAndGetAddressOf());

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

    bool D3D11Renderer::CreateGpuTimingQueries()
    {
        for (GpuTimingQuerySet& querySet :
            gpuTimingQuerySets_)
        {
            D3D11_QUERY_DESC disjointDescription{};
            disjointDescription.Query =
                D3D11_QUERY_TIMESTAMP_DISJOINT;

            HRESULT result = device_->CreateQuery(
                &disjointDescription,
                querySet.disjointQuery
                .ReleaseAndGetAddressOf());

            if (FAILED(result))
            {
                return false;
            }

            D3D11_QUERY_DESC timestampDescription{};
            timestampDescription.Query =
                D3D11_QUERY_TIMESTAMP;

            result = device_->CreateQuery(
                &timestampDescription,
                querySet.startTimestampQuery
                .ReleaseAndGetAddressOf());

            if (FAILED(result))
            {
                return false;
            }

            result = device_->CreateQuery(
                &timestampDescription,
                querySet.endTimestampQuery
                .ReleaseAndGetAddressOf());

            if (FAILED(result))
            {
                return false;
            }
        }

        return true;
    }

    void D3D11Renderer::
        ResolveGpuTimingQueries() noexcept
    {
        for (GpuTimingQuerySet& querySet :
            gpuTimingQuerySets_)
        {
            if (!querySet.pending)
            {
                continue;
            }

            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT
                disjointData{};

            const HRESULT disjointResult =
                deviceContext_->GetData(
                    querySet.disjointQuery.Get(),
                    &disjointData,
                    sizeof(disjointData),
                    D3D11_ASYNC_GETDATA_DONOTFLUSH);

            if (disjointResult == S_FALSE)
            {
                continue;
            }

            if (FAILED(disjointResult))
            {
                querySet.pending = false;
                continue;
            }

            UINT64 startTimestamp = 0;
            UINT64 endTimestamp = 0;

            const HRESULT startResult =
                deviceContext_->GetData(
                    querySet.startTimestampQuery.Get(),
                    &startTimestamp,
                    sizeof(startTimestamp),
                    D3D11_ASYNC_GETDATA_DONOTFLUSH);

            const HRESULT endResult =
                deviceContext_->GetData(
                    querySet.endTimestampQuery.Get(),
                    &endTimestamp,
                    sizeof(endTimestamp),
                    D3D11_ASYNC_GETDATA_DONOTFLUSH);

            if (startResult == S_FALSE ||
                endResult == S_FALSE)
            {
                continue;
            }

            querySet.pending = false;

            if (FAILED(startResult) ||
                FAILED(endResult) ||
                disjointData.Disjoint ||
                disjointData.Frequency == 0 ||
                endTimestamp < startTimestamp)
            {
                continue;
            }

            constexpr double
                MillisecondsPerSecond = 1000.0;

            const double elapsedTicks =
                static_cast<double>(
                    endTimestamp -
                    startTimestamp);

            if (querySet.generation !=
                gpuTimingGeneration_)
            {
                continue;
            }

            latestGpuMilliseconds_ =
                static_cast<float>(
                    elapsedTicks *
                    MillisecondsPerSecond /
                    static_cast<double>(
                        disjointData.Frequency));

            latestGpuTimingValid_ = true;

            ++gpuTimingSampleSerial_;

            return;
        }
    }

    bool D3D11Renderer::
        BeginGpuTimingQuery() noexcept
    {
        if (gpuTimingQueryActive_)
        {
            return false;
        }

        for (std::size_t offset = 0;
            offset < GpuTimingQuerySetCount;
            ++offset)
        {
            const std::size_t queryIndex =
                (gpuTimingWriteIndex_ + offset) %
                GpuTimingQuerySetCount;

            GpuTimingQuerySet& querySet =
                gpuTimingQuerySets_[queryIndex];

            if (querySet.pending)
            {
                continue;
            }

            activeGpuTimingQueryIndex_ =
                queryIndex;

            querySet.generation =
                gpuTimingGeneration_;

            deviceContext_->Begin(
                querySet.disjointQuery.Get());

            deviceContext_->End(
                querySet.startTimestampQuery.Get());

            gpuTimingQueryActive_ = true;

            return true;
        }

        return false;
    }

    void D3D11Renderer::
        EndGpuTimingQuery() noexcept
    {
        if (!gpuTimingQueryActive_)
        {
            return;
        }

        GpuTimingQuerySet& querySet =
            gpuTimingQuerySets_[
                activeGpuTimingQueryIndex_];

        deviceContext_->End(
            querySet.endTimestampQuery.Get());

        deviceContext_->End(
            querySet.disjointQuery.Get());

        querySet.pending = true;

        gpuTimingWriteIndex_ =
            (activeGpuTimingQueryIndex_ + 1) %
            GpuTimingQuerySetCount;

        gpuTimingQueryActive_ = false;
    }

    bool D3D11Renderer::
        EnsureInstanceBufferCapacity(
            std::size_t requiredCapacity)
    {
        if (requiredCapacity <=
            instanceBufferCapacity_)
        {
            return true;
        }

        std::size_t newCapacity =
            (std::max)(
                InitialInstanceCapacity,
                instanceBufferCapacity_);

        while (newCapacity < requiredCapacity)
        {
            const std::size_t maximumSize =
                (std::numeric_limits<
                    std::size_t>::max)();

            if (newCapacity >
                maximumSize / 2)
            {
                return false;
            }

            newCapacity *= 2;
        }

        const std::size_t maximumBufferBytes =
            (std::numeric_limits<UINT>::max)();

        if (newCapacity >
            maximumBufferBytes /
            sizeof(InstanceData))
        {
            return false;
        }

        D3D11_BUFFER_DESC description{};

        description.ByteWidth =
            static_cast<UINT>(
                sizeof(InstanceData) *
                newCapacity);

        description.Usage =
            D3D11_USAGE_DYNAMIC;

        description.BindFlags =
            D3D11_BIND_VERTEX_BUFFER;

        description.CPUAccessFlags =
            D3D11_CPU_ACCESS_WRITE;

        ComPtr<ID3D11Buffer> newInstanceBuffer;

        const HRESULT result =
            device_->CreateBuffer(
                &description,
                nullptr,
                newInstanceBuffer
                .ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        const std::size_t previousCapacity =
            instanceBufferCapacity_;

        instanceBuffer_.Swap(newInstanceBuffer);
        instanceBufferCapacity_ = newCapacity;

        char message[192]{};

        std::snprintf(
            message,
            sizeof(message),
            "Instance buffer resized"
            " | previous: %zu"
            " | current: %zu"
            " | bytes: %zu\n",
            previousCapacity,
            instanceBufferCapacity_,
            sizeof(InstanceData) *
            instanceBufferCapacity_);

        OutputDebugStringA(message);

        return true;
    }
}
