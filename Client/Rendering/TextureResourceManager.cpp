#include "TextureResourceManager.h"

#include "D3D11Renderer.h"

#include <chrono>
#include <utility>

namespace DungeonSync::Rendering
{
    TextureResourceManager::~TextureResourceManager()
    {
        Stop();
    }

    bool TextureResourceManager::Start(
        D3D11Renderer& renderer)
    {
        if (running_)
        {
            return true;
        }

        renderer_ = &renderer;

        resources_.clear();

        failedUploadCount_ = 0;

        lastDecodeMilliseconds_ = 0.0F;
        lastUploadMilliseconds_ = 0.0F;
        lastRequestToReadyMilliseconds_ = 0.0F;

        if (!asyncLoader_.Start())
        {
            renderer_ = nullptr;
            return false;
        }

        running_ = true;
        return true;
    }

    void TextureResourceManager::Stop() noexcept
    {
        asyncLoader_.Stop();

        resources_.clear();

        renderer_ = nullptr;
        running_ = false;
    }

    bool TextureResourceManager::RequestAsync(
        const std::filesystem::path& path)
    {
        if (!running_)
        {
            return false;
        }

        const std::filesystem::path normalizedPath =
            NormalizePath(path);

        if (normalizedPath.empty())
        {
            return false;
        }

        const auto existingEntry =
            resources_.find(normalizedPath);

        if (existingEntry != resources_.end() &&
            existingEntry->second.state !=
            TextureLoadState::Failed)
        {
            return false;
        }

        const std::optional<std::uint64_t> requestId =
            asyncLoader_.Request(normalizedPath);

        if (!requestId.has_value())
        {
            return false;
        }

        TextureEntry entry{};

        entry.path = normalizedPath;
        entry.state = TextureLoadState::Queued;
        entry.requestId = requestId.value();

        entry.requestStartTime =
            std::chrono::steady_clock::now();

        resources_.insert_or_assign(
            normalizedPath,
            std::move(entry));

        return true;
    }

    void TextureResourceManager::Update()
    {
        if (!running_ ||
            renderer_ == nullptr)
        {
            return;
        }

        AsyncTextureLoadResult completedResult{};

        if (!asyncLoader_.TryPopCompleted(
            completedResult))
        {
            return;
        }

        const std::filesystem::path normalizedPath =
            NormalizePath(completedResult.path);

        const auto entryIterator =
            resources_.find(normalizedPath);

        if (entryIterator == resources_.end())
        {
            return;
        }

        TextureEntry& entry =
            entryIterator->second;

        if (entry.requestId !=
            completedResult.requestId)
        {
            return;
        }

        entry.decodeMilliseconds =
            completedResult.decodeMilliseconds;

        lastDecodeMilliseconds_ =
            completedResult.decodeMilliseconds;

        if (!completedResult.succeeded)
        {
            entry.state = TextureLoadState::Failed;
            return;
        }

        entry.state =
            TextureLoadState::ReadyForUpload;

        LoadedTexture uploadedTexture{};

        const auto uploadStart =
            std::chrono::steady_clock::now();

        const bool uploadSucceeded =
            renderer_->UploadDecodedTexture(
                completedResult.image,
                uploadedTexture);

        const auto uploadEnd =
            std::chrono::steady_clock::now();

        entry.uploadMilliseconds =
            std::chrono::duration<
            float,
            std::milli>(
                uploadEnd - uploadStart)
            .count();

        lastUploadMilliseconds_ =
            entry.uploadMilliseconds;

        if (!uploadSucceeded)
        {
            entry.state = TextureLoadState::Failed;
            ++failedUploadCount_;

            return;
        }

        entry.texture =
            std::move(uploadedTexture);

        entry.estimatedGpuBytes =
            static_cast<std::uint64_t>(
                entry.texture.width) *
            entry.texture.height *
            4ULL;

        const auto readyTime =
            std::chrono::steady_clock::now();

        entry.requestToReadyMilliseconds =
            std::chrono::duration<
            float,
            std::milli>(
                readyTime -
                entry.requestStartTime)
            .count();

        lastRequestToReadyMilliseconds_ =
            entry.requestToReadyMilliseconds;

        entry.state = TextureLoadState::Ready;
    }

    const LoadedTexture*
        TextureResourceManager::Find(
            const std::filesystem::path& path) const
    {
        const std::filesystem::path normalizedPath =
            NormalizePath(path);

        const auto entryIterator =
            resources_.find(normalizedPath);

        if (entryIterator == resources_.end() ||
            entryIterator->second.state !=
            TextureLoadState::Ready)
        {
            return nullptr;
        }

        return &entryIterator->second.texture;
    }

    TextureLoadState
        TextureResourceManager::State(
            const std::filesystem::path& path) const
    {
        const std::filesystem::path normalizedPath =
            NormalizePath(path);

        const auto entryIterator =
            resources_.find(normalizedPath);

        if (entryIterator == resources_.end())
        {
            return TextureLoadState::Unloaded;
        }

        return entryIterator->second.state;
    }

    TextureResourceManagerStatistics
        TextureResourceManager::Statistics()
        const noexcept
    {
        TextureResourceManagerStatistics statistics{};

        statistics.resourceCount =
            resources_.size();

        for (const auto& [path, entry] : resources_)
        {
            (void)path;

            if (entry.state ==
                TextureLoadState::Ready)
            {
                ++statistics.readyResourceCount;

                statistics.estimatedGpuBytes +=
                    entry.estimatedGpuBytes;
            }
            else if (entry.state ==
                TextureLoadState::Failed)
            {
                ++statistics.failedResourceCount;
            }
        }

        const AsyncTextureLoaderStatistics
            loaderStatistics =
            asyncLoader_.Statistics();

        statistics.queuedRequestCount =
            loaderStatistics.queuedRequestCount;

        statistics.completedResultCount =
            loaderStatistics.completedResultCount;

        statistics.decodedBytesAwaitingUpload =
            loaderStatistics
            .decodedBytesAwaitingUpload;

        statistics.peakDecodedBytes =
            loaderStatistics.peakDecodedBytes;

        statistics.rejectedRequestCount =
            loaderStatistics.rejectedRequestCount;

        statistics.failedDecodeCount =
            loaderStatistics.failedRequestCount;

        statistics.failedUploadCount =
            failedUploadCount_;

        statistics.lastDecodeMilliseconds =
            lastDecodeMilliseconds_;

        statistics.lastUploadMilliseconds =
            lastUploadMilliseconds_;

        statistics.lastRequestToReadyMilliseconds =
            lastRequestToReadyMilliseconds_;

        statistics.decoding =
            loaderStatistics.decoding;

        statistics.running =
            running_ && loaderStatistics.running;

        return statistics;
    }

    std::filesystem::path
        TextureResourceManager::NormalizePath(
            const std::filesystem::path& path)
    {
        if (path.empty())
        {
            return {};
        }

        return path.lexically_normal();
    }
}