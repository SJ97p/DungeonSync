#pragma once

#include <cstddef>

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
    };
}