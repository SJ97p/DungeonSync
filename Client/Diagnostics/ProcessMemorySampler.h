#pragma once

#include <cstdint>

namespace DungeonSync::Diagnostics
{
    struct ProcessMemorySnapshot
    {
        std::uint64_t workingSetBytes{};
        std::uint64_t peakWorkingSetBytes{};
        std::uint64_t privateBytes{};
        bool valid{};
    };

    class ProcessMemorySampler final
    {
    public:
        [[nodiscard]]
        static ProcessMemorySnapshot Capture() noexcept;
    };
}
