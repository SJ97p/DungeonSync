#pragma once

#include <cstddef>
#include <cstdint>

namespace DungeonSync::Rendering
{
    struct RenderStatistics
    {
        std::size_t drawCallCount{};

        std::size_t submittedInstanceCount{};
        std::size_t instanceCount{};
        std::size_t droppedInstanceCount{};
        std::size_t instanceBufferCapacity{};

        float cpuSubmissionMilliseconds{};
        float presentMilliseconds{};
        float gpuMilliseconds{};
        bool gpuTimingValid{};
        std::uint64_t gpuTimingSampleSerial{};
    };
}