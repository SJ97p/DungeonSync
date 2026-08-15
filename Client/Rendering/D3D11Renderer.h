#pragma once
#include "RenderItem.h"
#include "Camera.h"
#include "InstanceData.h"
#include "RenderStatistics.h"
#include "WicTextureLoader.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <span>
#include <cstdint>
#include <cstddef>
#include <array>

namespace DungeonSync::Rendering
{
    enum class SpriteSubmissionMode
    {
        InstancedBatch,
        PerInstance
    };

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

        [[nodiscard]]
        const RenderStatistics&
            Statistics() const noexcept;

        void SetVSyncEnabled(bool enabled) noexcept;

        [[nodiscard]]
        bool IsVSyncEnabled() const noexcept;

        void BeginGpuTimingGeneration() noexcept;

        void Render(
            const Camera& camera,
            std::span<const RenderItem> renderItems,
            SpriteSubmissionMode submissionMode =
            SpriteSubmissionMode::InstancedBatch);

    private:
        struct GpuTimingQuerySet
        {
            Microsoft::WRL::ComPtr<ID3D11Query>
                disjointQuery;

            Microsoft::WRL::ComPtr<ID3D11Query>
                startTimestampQuery;

            Microsoft::WRL::ComPtr<ID3D11Query>
                endTimestampQuery;

            std::uint64_t generation{};
            bool pending{};
        };

        [[nodiscard]] bool CreateDeviceAndSwapChain(
            HWND window,
            std::uint32_t width,
            std::uint32_t height);

        [[nodiscard]] bool CreateRenderTarget();
        [[nodiscard]] bool CreateDepthBuffer();
        [[nodiscard]] bool CreateSpriteGeometryBuffers();
        [[nodiscard]] bool CreateGroundGeometryBuffers();
        [[nodiscard]] bool CreateBackgroundGeometryBuffers();
        [[nodiscard]] bool CreateShadersAndInputLayout();
        [[nodiscard]] bool CreateGroundShadersAndInputLayout();
        [[nodiscard]] bool CreateConstantBuffer();
        [[nodiscard]]
        bool EnsureInstanceBufferCapacity(
            std::size_t requiredCapacity);
        [[nodiscard]] bool CreateTextureResources();
        [[nodiscard]] bool CreateSpriteSampler();
        [[nodiscard]] bool CreateGroundSampler();
        [[nodiscard]] bool CreateSpriteBlendState();
        [[nodiscard]]
        bool CreateGpuTimingQueries();

        void ResolveGpuTimingQueries() noexcept;

        [[nodiscard]]
        bool BeginGpuTimingQuery() noexcept;

        void EndGpuTimingQuery() noexcept;
        static constexpr std::size_t
            GpuTimingQuerySetCount = 4;

        std::array<
            GpuTimingQuerySet,
            GpuTimingQuerySetCount>
            gpuTimingQuerySets_{};

        std::size_t gpuTimingWriteIndex_{};
        std::size_t activeGpuTimingQueryIndex_{};
        bool gpuTimingQueryActive_{};

        std::uint64_t gpuTimingGeneration_{ 1 };
        std::uint64_t gpuTimingSampleSerial_{};
        float latestGpuMilliseconds_{};
        bool latestGpuTimingValid_{};

        Microsoft::WRL::ComPtr<ID3D11Device> device_;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext_;
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBuffer_;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> groundVertexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> groundIndexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> backgroundVertexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> backgroundIndexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> groundVertexShader_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> groundPixelShader_;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> spriteSampler_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> groundSampler_;
        Microsoft::WRL::ComPtr<ID3D11BlendState> spriteBlendState_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> groundInputLayout_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> instanceBuffer_;
        WicTextureLoader textureLoader_{};
        LoadedTexture spriteAtlas_{};
        LoadedTexture groundTexture_{};
        LoadedTexture backgroundTexture_{};

        D3D_FEATURE_LEVEL featureLevel_{};

        static constexpr std::size_t
            InitialInstanceCapacity = 1024;

        std::size_t instanceBufferCapacity_{};

        RenderStatistics statistics_{};
        std::uint32_t width_{};
        std::uint32_t height_{};
        UINT presentSyncInterval_{ 1 };
    };
}