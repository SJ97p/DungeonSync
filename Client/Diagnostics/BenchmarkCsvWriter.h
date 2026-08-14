#pragma once

#include "BenchmarkResult.h"

#include <filesystem>

namespace DungeonSync::Diagnostics
{
    class BenchmarkCsvWriter final
    {
    public:
        [[nodiscard]]
        static bool Append(
            const std::filesystem::path& filePath,
            const BenchmarkResult& result);
    };
}