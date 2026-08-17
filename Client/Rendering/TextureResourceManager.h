#pragma once

#include "AsyncTextureLoader.h"
#include "WicTextureLoader.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <unordered_map>

namespace DungeonSync::Rendering
{
    class D3D11Renderer;

    enum class TextureLoadState
    {
        Unloaded,
        Queued,
        ReadyForUpload,
        Ready,
        Failed
    };

    struct TextureResourceManagerStatistics
    {
        std::size_t resourceCount{};
        std::size_t readyResourceCount{};
        std::size_t failedResourceCount{};

        std::uint64_t estimatedGpuBytes{};

        std::size_t queuedRequestCount{};
        std::size_t completedResultCount{};

        std::size_t decodedBytesAwaitingUpload{};
        std::size_t peakDecodedBytes{};

        std::size_t rejectedRequestCount{};
        std::size_t failedDecodeCount{};
        std::size_t failedUploadCount{};

        float lastDecodeMilliseconds{};
        float lastUploadMilliseconds{};
        float lastRequestToReadyMilliseconds{};

        bool decoding{};
        bool running{};
    };

    class TextureResourceManager final
    {
    public:
        TextureResourceManager() = default;
        ~TextureResourceManager();

        TextureResourceManager(
            const TextureResourceManager&) = delete;

        TextureResourceManager& operator=(
            const TextureResourceManager&) = delete;

        TextureResourceManager(
            TextureResourceManager&&) = delete;

        TextureResourceManager& operator=(
            TextureResourceManager&&) = delete;

        [[nodiscard]]
        bool Start(D3D11Renderer& renderer);

        void Stop() noexcept;

        [[nodiscard]]
        bool RequestAsync(
            const std::filesystem::path& path);

        void Update();

        [[nodiscard]]
        const LoadedTexture* Find(
            const std::filesystem::path& path) const;

        [[nodiscard]]
        TextureLoadState State(
            const std::filesystem::path& path) const;

        [[nodiscard]]
        TextureResourceManagerStatistics
            Statistics() const noexcept;

    private:
        struct TextureEntry
        {
            std::filesystem::path path;

            LoadedTexture texture;

            TextureLoadState state{
                TextureLoadState::Unloaded
            };

            std::uint64_t requestId{};
            std::uint64_t estimatedGpuBytes{};

            float decodeMilliseconds{};
            float uploadMilliseconds{};
            float requestToReadyMilliseconds{};

            std::chrono::steady_clock::time_point
                requestStartTime{};
        };

        [[nodiscard]]
        static std::filesystem::path NormalizePath(
            const std::filesystem::path& path);

        AsyncTextureLoader asyncLoader_;

        std::unordered_map<
            std::filesystem::path,
            TextureEntry>
            resources_;

        D3D11Renderer* renderer_{};

        std::size_t failedUploadCount_{};

        float lastDecodeMilliseconds_{};
        float lastUploadMilliseconds_{};
        float lastRequestToReadyMilliseconds_{};

        bool running_{};
    };
}