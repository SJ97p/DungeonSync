#include "AsyncTextureLoader.h"

#include <objbase.h>

#include <algorithm>
#include <chrono>
#include <utility>

namespace DungeonSync::Rendering
{
    AsyncTextureLoader::~AsyncTextureLoader()
    {
        Stop();
    }

    bool AsyncTextureLoader::Start()
    {
        std::lock_guard lock(mutex_);

        if (running_)
        {
            return true;
        }

        requestQueue_.clear();
        completedQueue_.clear();
        pendingPaths_.clear();

        nextRequestId_ = 1;

        decodedBytesAwaitingUpload_ = 0;
        peakDecodedBytes_ = 0;

        rejectedRequestCount_ = 0;
        failedRequestCount_ = 0;

        decoding_ = false;
        stopRequested_ = false;
        running_ = true;

        try
        {
            workerThread_ = std::thread(
                &AsyncTextureLoader::WorkerMain,
                this);
        }
        catch (...)
        {
            running_ = false;
            return false;
        }

        return true;
    }

    void AsyncTextureLoader::Stop() noexcept
    {
        {
            std::lock_guard lock(mutex_);

            if (!running_ &&
                !workerThread_.joinable())
            {
                return;
            }

            stopRequested_ = true;
            requestQueue_.clear();
        }

        requestCondition_.notify_all();
        completionSpaceCondition_.notify_all();

        if (workerThread_.joinable())
        {
            workerThread_.join();
        }

        {
            std::lock_guard lock(mutex_);

            requestQueue_.clear();
            completedQueue_.clear();
            pendingPaths_.clear();

            decodedBytesAwaitingUpload_ = 0;

            decoding_ = false;
            running_ = false;
        }
    }

    std::optional<std::uint64_t>
        AsyncTextureLoader::Request(
            const std::filesystem::path& path)
    {
        const std::filesystem::path normalizedPath =
            NormalizePath(path);

        if (normalizedPath.empty())
        {
            std::lock_guard lock(mutex_);
            ++rejectedRequestCount_;

            return std::nullopt;
        }

        std::uint64_t requestId = 0;

        {
            std::lock_guard lock(mutex_);

            if (!running_ ||
                stopRequested_ ||
                requestQueue_.size() >=
                MaximumQueuedRequestCount ||
                pendingPaths_.contains(
                    normalizedPath))
            {
                ++rejectedRequestCount_;
                return std::nullopt;
            }

            requestId = nextRequestId_++;

            requestQueue_.push_back(
                AsyncTextureLoadRequest{
                    requestId,
                    normalizedPath
                });

            pendingPaths_.insert(normalizedPath);
        }

        requestCondition_.notify_one();

        return requestId;
    }

    bool AsyncTextureLoader::TryPopCompleted(
        AsyncTextureLoadResult& outputResult)
    {
        {
            std::lock_guard lock(mutex_);

            if (completedQueue_.empty())
            {
                return false;
            }

            const std::size_t decodedByteCount =
                completedQueue_
                .front()
                .image
                .pixels
                .size();

            outputResult =
                std::move(completedQueue_.front());

            completedQueue_.pop_front();
            pendingPaths_.erase(outputResult.path);

            decodedBytesAwaitingUpload_ -=
                (std::min)(
                    decodedBytesAwaitingUpload_,
                    decodedByteCount);
        }

        completionSpaceCondition_.notify_one();

        return true;
    }

    AsyncTextureLoaderStatistics
        AsyncTextureLoader::Statistics()
        const noexcept
    {
        std::lock_guard lock(mutex_);

        return AsyncTextureLoaderStatistics{
            requestQueue_.size(),
            completedQueue_.size(),
            decodedBytesAwaitingUpload_,
            peakDecodedBytes_,
            rejectedRequestCount_,
            failedRequestCount_,
            decoding_,
            running_
        };
    }

    void AsyncTextureLoader::WorkerMain() noexcept
    {
        const HRESULT comResult =
            CoInitializeEx(
                nullptr,
                COINIT_MULTITHREADED);

        const bool comInitialized =
            SUCCEEDED(comResult);

        WicTextureLoader decoder;

        const bool decoderInitialized =
            comInitialized &&
            decoder.Initialize();

        while (true)
        {
            AsyncTextureLoadRequest request{};

            {
                std::unique_lock lock(mutex_);

                requestCondition_.wait(
                    lock,
                    [this]()
                    {
                        return
                            stopRequested_ ||
                            !requestQueue_.empty();
                    });

                if (stopRequested_)
                {
                    break;
                }

                request =
                    std::move(requestQueue_.front());

                requestQueue_.pop_front();
                decoding_ = true;
            }

            AsyncTextureLoadResult result{};
            result.requestId = request.requestId;
            result.path = std::move(request.path);

            const auto decodeStart =
                std::chrono::steady_clock::now();

            if (decoderInitialized)
            {
                result.succeeded =
                    decoder.DecodeFromFile(
                        result.path,
                        result.image);
            }

            const auto decodeEnd =
                std::chrono::steady_clock::now();

            result.decodeMilliseconds =
                std::chrono::duration<
                float,
                std::milli>(
                    decodeEnd - decodeStart)
                .count();

            {
                std::unique_lock lock(mutex_);

                decoding_ = false;

                if (!result.succeeded)
                {
                    ++failedRequestCount_;
                }

                completionSpaceCondition_.wait(
                    lock,
                    [this]()
                    {
                        return
                            stopRequested_ ||
                            completedQueue_.size() <
                            MaximumCompletedResultCount;
                    });

                if (stopRequested_)
                {
                    break;
                }

                const std::size_t decodedByteCount =
                    result.image.pixels.size();

                decodedBytesAwaitingUpload_ +=
                    decodedByteCount;

                peakDecodedBytes_ =
                    (std::max)(
                        peakDecodedBytes_,
                        decodedBytesAwaitingUpload_);

                completedQueue_.push_back(
                    std::move(result));
            }
        }

        if (comInitialized)
        {
            CoUninitialize();
        }
    }

    std::filesystem::path
        AsyncTextureLoader::NormalizePath(
            const std::filesystem::path& path)
    {
        if (path.empty())
        {
            return {};
        }

        return path.lexically_normal();
    }
}