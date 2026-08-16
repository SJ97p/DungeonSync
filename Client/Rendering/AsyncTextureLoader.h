#pragma once

#include "WicTextureLoader.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_set>

namespace DungeonSync::Rendering
{
    struct AsyncTextureLoadRequest
    {
        std::uint64_t requestId{};
        std::filesystem::path path;
    };

    struct AsyncTextureLoadResult
    {
        std::uint64_t requestId{};
        std::filesystem::path path;

        DecodedImage image;

        float decodeMilliseconds{};
        bool succeeded{};
    };

    struct AsyncTextureLoaderStatistics
    {
        std::size_t queuedRequestCount{};
        std::size_t completedResultCount{};

        std::size_t decodedBytesAwaitingUpload{};
        std::size_t peakDecodedBytes{};

        std::size_t rejectedRequestCount{};
        std::size_t failedRequestCount{};

        bool decoding{};
        bool running{};
    };

    class AsyncTextureLoader final
    {
    public:
        AsyncTextureLoader() = default;
        ~AsyncTextureLoader();

        AsyncTextureLoader(
            const AsyncTextureLoader&) = delete;

        AsyncTextureLoader& operator=(
            const AsyncTextureLoader&) = delete;

        AsyncTextureLoader(
            AsyncTextureLoader&&) = delete;

        AsyncTextureLoader& operator=(
            AsyncTextureLoader&&) = delete;

        [[nodiscard]]
        bool Start();

        void Stop() noexcept;

        [[nodiscard]]
        std::optional<std::uint64_t> Request(
            const std::filesystem::path& path);

        [[nodiscard]]
        bool TryPopCompleted(
            AsyncTextureLoadResult& outputResult);

        [[nodiscard]]
        AsyncTextureLoaderStatistics
            Statistics() const noexcept;

    private:
        void WorkerMain() noexcept;

        [[nodiscard]]
        static std::filesystem::path NormalizePath(
            const std::filesystem::path& path);

        static constexpr std::size_t
            MaximumQueuedRequestCount = 32;

        static constexpr std::size_t
            MaximumCompletedResultCount = 8;

        mutable std::mutex mutex_;

        std::condition_variable requestCondition_;
        std::condition_variable completionSpaceCondition_;

        std::thread workerThread_;

        std::deque<AsyncTextureLoadRequest>
            requestQueue_;

        std::deque<AsyncTextureLoadResult>
            completedQueue_;

        std::unordered_set<std::filesystem::path>
            pendingPaths_;

        std::uint64_t nextRequestId_{ 1 };

        std::size_t decodedBytesAwaitingUpload_{};
        std::size_t peakDecodedBytes_{};

        std::size_t rejectedRequestCount_{};
        std::size_t failedRequestCount_{};

        bool decoding_{};
        bool stopRequested_{};
        bool running_{};
    };
}