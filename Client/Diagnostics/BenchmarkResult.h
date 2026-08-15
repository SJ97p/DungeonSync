#pragma once

#include "FrameTimeProfiler.h"

#include <cstddef>
#include <string>

namespace DungeonSync::Diagnostics
{
    struct BenchmarkResult
    {
        std::string scenarioName;
        std::string submissionMode;
        std::string buildConfiguration;
        std::string presentationMode;

        std::size_t instanceCount{};
        std::size_t drawCallCount{};
        std::size_t droppedInstanceCount{};
        std::size_t instanceBufferCapacity{};

        FrameTimeSnapshot frameTime;
        FrameTimeSnapshot cpuSubmissionTime;
        FrameTimeSnapshot gpuTime;
        FrameTimeSnapshot presentTime;
    };
}