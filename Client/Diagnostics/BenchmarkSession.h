#pragma once

#include "FrameTimeProfiler.h"

#include <array>
#include <cstddef>

namespace DungeonSync::Diagnostics
{
    enum class BenchmarkSubmissionMode
    {
        InstancedBatch,
        PerInstance
    };

    struct BenchmarkScenario
    {
        std::size_t instanceCount{};
        BenchmarkSubmissionMode submissionMode{
            BenchmarkSubmissionMode::InstancedBatch
        };
    };

    enum class BenchmarkSessionEvent
    {
        None,
        MeasurementStarted,
        MeasurementFinished
    };

    enum class BenchmarkSessionPhase
    {
        Idle,
        WarmingUp,
        Measuring,
        AwaitingResult,
        Completed
    };

    class BenchmarkSession final
    {
    public:
        static constexpr float WarmupSeconds = 2.0F;
        static constexpr std::size_t
            TargetSampleCount =
            FrameTimeProfiler::SampleCapacity;

        static constexpr std::size_t ScenarioCount = 8;

        void Start() noexcept;
        void Stop() noexcept;

        [[nodiscard]]
        BenchmarkSessionEvent Update(
            float deltaSeconds,
            std::size_t collectedSampleCount) noexcept;

        [[nodiscard]]
        bool AdvanceAfterResult() noexcept;

        [[nodiscard]]
        bool IsActive() const noexcept;

        [[nodiscard]]
        BenchmarkSessionPhase Phase() const noexcept;

        [[nodiscard]]
        const BenchmarkScenario&
            CurrentScenario() const noexcept;

        [[nodiscard]]
        std::size_t CurrentScenarioNumber()
            const noexcept;

        [[nodiscard]]
        float PhaseElapsedSeconds() const noexcept;

    private:
        static constexpr std::array<
            BenchmarkScenario,
            ScenarioCount>
            Scenarios{
                BenchmarkScenario{
                    100,
                    BenchmarkSubmissionMode::
                        InstancedBatch
                },
                BenchmarkScenario{
                    100,
                    BenchmarkSubmissionMode::
                        PerInstance
                },
                BenchmarkScenario{
                    1000,
                    BenchmarkSubmissionMode::
                        InstancedBatch
                },
                BenchmarkScenario{
                    1000,
                    BenchmarkSubmissionMode::
                        PerInstance
                },
                BenchmarkScenario{
                    3000,
                    BenchmarkSubmissionMode::
                        InstancedBatch
                },
                BenchmarkScenario{
                    3000,
                    BenchmarkSubmissionMode::
                        PerInstance
                },
                BenchmarkScenario{
                    10000,
                    BenchmarkSubmissionMode::
                        InstancedBatch
                },
                BenchmarkScenario{
                    10000,
                    BenchmarkSubmissionMode::
                        PerInstance
                }
        };

        BenchmarkSessionPhase phase_{
            BenchmarkSessionPhase::Idle
        };

        std::size_t scenarioIndex_{};
        float phaseElapsedSeconds_{};
    };
}