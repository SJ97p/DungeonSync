#pragma once

#include <cstddef>

namespace DungeonSync::Rendering
{
    struct RenderStatistics
    {
        std::size_t drawCallCount{};
        std::size_t instanceCount{};
    };
}