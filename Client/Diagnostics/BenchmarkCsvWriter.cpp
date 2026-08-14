#include "BenchmarkCsvWriter.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <system_error>

namespace
{
    void WriteFrameTimeFields(
        std::ofstream& stream,
        const DungeonSync::Diagnostics::
        FrameTimeSnapshot& snapshot)
    {
        stream
            << snapshot.sampleCount << ','
            << snapshot.averageMilliseconds << ','
            << snapshot.percentile95Milliseconds << ','
            << snapshot.percentile99Milliseconds << ','
            << snapshot.maximumMilliseconds << ','
            << snapshot.framesOver16Milliseconds << ','
            << snapshot.framesOver33Milliseconds;
    }
}

namespace DungeonSync::Diagnostics
{
    bool BenchmarkCsvWriter::Append(
        const std::filesystem::path& filePath,
        const BenchmarkResult& result)
    {
        std::error_code errorCode;

        const std::filesystem::path parentPath =
            filePath.parent_path();

        if (!parentPath.empty())
        {
            std::filesystem::create_directories(
                parentPath,
                errorCode);

            if (errorCode)
            {
                return false;
            }
        }

        const bool writeHeader =
            !std::filesystem::exists(
                filePath,
                errorCode) ||
            std::filesystem::file_size(
                filePath,
                errorCode) == 0;

        if (errorCode)
        {
            return false;
        }

        std::ofstream stream(
            filePath,
            std::ios::out |
            std::ios::app);

        if (!stream)
        {
            return false;
        }

        if (writeHeader)
        {
            stream
                << "timestamp_ms,"
                << "scenario,"
                << "submission_mode,"
                << "build,"
                << "instances,"
                << "draw_calls,"
                << "dropped_instances,"
                << "instance_capacity,"
                << "frame_samples,"
                << "frame_avg_ms,"
                << "frame_p95_ms,"
                << "frame_p99_ms,"
                << "frame_max_ms,"
                << "frames_over_16_67_ms,"
                << "frames_over_33_33_ms,"
                << "cpu_samples,"
                << "cpu_avg_ms,"
                << "cpu_p95_ms,"
                << "cpu_p99_ms,"
                << "cpu_max_ms,"
                << "cpu_over_16_67_ms,"
                << "cpu_over_33_33_ms,"
                << "gpu_samples,"
                << "gpu_avg_ms,"
                << "gpu_p95_ms,"
                << "gpu_p99_ms,"
                << "gpu_max_ms,"
                << "gpu_over_16_67_ms,"
                << "gpu_over_33_33_ms,"
                << "present_samples,"
                << "present_avg_ms,"
                << "present_p95_ms,"
                << "present_p99_ms,"
                << "present_max_ms,"
                << "present_over_16_67_ms,"
                << "present_over_33_33_ms\n";
        }

        const auto timestamp =
            std::chrono::duration_cast<
            std::chrono::milliseconds>(
                std::chrono::
                system_clock::now()
                .time_since_epoch())
            .count();

        stream << std::fixed
            << std::setprecision(6);

        stream
            << timestamp << ','
            << result.scenarioName << ','
            << result.submissionMode << ','
            << result.buildConfiguration << ','
            << result.instanceCount << ','
            << result.drawCallCount << ','
            << result.droppedInstanceCount << ','
            << result.instanceBufferCapacity << ',';

        WriteFrameTimeFields(
            stream,
            result.frameTime);

        stream << ',';

        WriteFrameTimeFields(
            stream,
            result.cpuSubmissionTime);

        stream << ',';

        WriteFrameTimeFields(
            stream,
            result.gpuTime);

        stream << ',';

        WriteFrameTimeFields(
            stream,
            result.presentTime);

        stream << '\n';

        return static_cast<bool>(stream);
    }
}