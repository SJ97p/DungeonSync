#pragma once

#include <array>
#include <cstddef>

namespace DungeonSync::Diagnostics
{
    struct FrameTimeSnapshot
    {
        std::size_t sampleCount{};

        float averageMilliseconds{};
        float percentile95Milliseconds{};
        float percentile99Milliseconds{};
        float maximumMilliseconds{};

        std::size_t framesOver16Milliseconds{};
        std::size_t framesOver33Milliseconds{};
    };

    class FrameTimeProfiler final
    {
    public:
        static constexpr std::size_t SampleCapacity = 600;

        void RecordFrame(float deltaSeconds) noexcept;

        void RecordMilliseconds(
            float milliseconds) noexcept;

        [[nodiscard]]
        FrameTimeSnapshot
            CaptureSnapshot() const noexcept;

        void Reset() noexcept;

    private:
        std::array<float, SampleCapacity>
            frameTimesMilliseconds_{};

        std::size_t nextSampleIndex_{};
        std::size_t sampleCount_{};
    };
}