#include "FrameTimeProfiler.h"

#include <algorithm>
#include <cmath>

namespace DungeonSync::Diagnostics
{
    void FrameTimeProfiler::RecordFrame(
        float deltaSeconds) noexcept
    {
        constexpr float MillisecondsPerSecond =
            1000.0F;

        RecordMilliseconds(
            deltaSeconds *
            MillisecondsPerSecond);
    }

    void FrameTimeProfiler::RecordMilliseconds(
        float milliseconds) noexcept
    {
        if (!std::isfinite(milliseconds) ||
            milliseconds <= 0.0F)
        {
            return;
        }

        frameTimesMilliseconds_[nextSampleIndex_] =
            milliseconds;

        nextSampleIndex_ =
            (nextSampleIndex_ + 1) %
            SampleCapacity;

        sampleCount_ =
            (std::min)(
                sampleCount_ + 1,
                SampleCapacity);
    }

    FrameTimeSnapshot
        FrameTimeProfiler::CaptureSnapshot()
        const noexcept
    {
        FrameTimeSnapshot snapshot{};
        snapshot.sampleCount = sampleCount_;

        if (sampleCount_ == 0)
        {
            return snapshot;
        }

        std::array<float, SampleCapacity>
            sortedSamples{};

        for (std::size_t index = 0;
            index < sampleCount_;
            ++index)
        {
            sortedSamples[index] =
                frameTimesMilliseconds_[index];
        }

        std::sort(
            sortedSamples.begin(),
            sortedSamples.begin() +
            sampleCount_);

        float totalMilliseconds = 0.0F;

        for (std::size_t index = 0;
            index < sampleCount_;
            ++index)
        {
            const float frameMilliseconds =
                sortedSamples[index];

            totalMilliseconds +=
                frameMilliseconds;

            if (frameMilliseconds > 16.67F)
            {
                ++snapshot.framesOver16Milliseconds;
            }

            if (frameMilliseconds > 33.33F)
            {
                ++snapshot.framesOver33Milliseconds;
            }
        }

        snapshot.averageMilliseconds =
            totalMilliseconds /
            static_cast<float>(sampleCount_);

        const auto percentileIndex =
            [this](float percentile)
            {
                const float rank =
                    std::ceil(
                        percentile *
                        static_cast<float>(
                            sampleCount_));

                const std::size_t index =
                    static_cast<std::size_t>(
                        rank) - 1;

                return (std::min)(
                    index,
                    sampleCount_ - 1);
            };

        snapshot.percentile95Milliseconds =
            sortedSamples[
                percentileIndex(0.95F)];

        snapshot.percentile99Milliseconds =
            sortedSamples[
                percentileIndex(0.99F)];

        snapshot.maximumMilliseconds =
            sortedSamples[sampleCount_ - 1];

        return snapshot;
    }

    std::size_t FrameTimeProfiler::SampleCount()
        const noexcept
    {
        return sampleCount_;
    }

    void FrameTimeProfiler::Reset() noexcept
    {
        frameTimesMilliseconds_ = {};
        nextSampleIndex_ = 0;
        sampleCount_ = 0;
    }
}