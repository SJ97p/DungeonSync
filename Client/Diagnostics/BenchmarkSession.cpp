#include "BenchmarkSession.h"

#include <algorithm>

namespace DungeonSync::Diagnostics
{
    void BenchmarkSession::Start() noexcept
    {
        scenarioIndex_ = 0;
        phaseElapsedSeconds_ = 0.0F;

        phase_ =
            BenchmarkSessionPhase::WarmingUp;
    }

    void BenchmarkSession::Stop() noexcept
    {
        scenarioIndex_ = 0;
        phaseElapsedSeconds_ = 0.0F;

        phase_ =
            BenchmarkSessionPhase::Idle;
    }

    BenchmarkSessionEvent BenchmarkSession::Update(
        float deltaSeconds,
        std::size_t collectedSampleCount) noexcept
    {
        const float safeDeltaSeconds =
            (std::max)(deltaSeconds, 0.0F);

        switch (phase_)
        {
        case BenchmarkSessionPhase::WarmingUp:
            phaseElapsedSeconds_ +=
                safeDeltaSeconds;

            if (phaseElapsedSeconds_ <
                WarmupSeconds ||
                collectedSampleCount <
                TargetSampleCount)
            {
                return BenchmarkSessionEvent::None;
            }

            phaseElapsedSeconds_ = 0.0F;
            phase_ =
                BenchmarkSessionPhase::Measuring;

            return BenchmarkSessionEvent::
                MeasurementStarted;

        case BenchmarkSessionPhase::Measuring:
            phaseElapsedSeconds_ +=
                safeDeltaSeconds;

            if (phaseElapsedSeconds_ <
                MinimumMeasurementSeconds ||
                collectedSampleCount <
                TargetSampleCount)
            {
                return BenchmarkSessionEvent::None;
            }

            phaseElapsedSeconds_ = 0.0F;
            phase_ =
                BenchmarkSessionPhase::
                AwaitingResult;

            return BenchmarkSessionEvent::
                MeasurementFinished;

        default:
            return BenchmarkSessionEvent::None;
        }
    }

    bool BenchmarkSession::
        AdvanceAfterResult() noexcept
    {
        if (phase_ !=
            BenchmarkSessionPhase::
            AwaitingResult)
        {
            return false;
        }

        ++scenarioIndex_;
        phaseElapsedSeconds_ = 0.0F;

        if (scenarioIndex_ >=
            Scenarios.size())
        {
            phase_ =
                BenchmarkSessionPhase::Completed;

            return false;
        }

        phase_ =
            BenchmarkSessionPhase::WarmingUp;

        return true;
    }

    bool BenchmarkSession::IsActive()
        const noexcept
    {
        return phase_ !=
            BenchmarkSessionPhase::Idle &&
            phase_ !=
            BenchmarkSessionPhase::Completed;
    }

    BenchmarkSessionPhase
        BenchmarkSession::Phase() const noexcept
    {
        return phase_;
    }

    const BenchmarkScenario&
        BenchmarkSession::CurrentScenario()
        const noexcept
    {
        return Scenarios[scenarioIndex_];
    }

    std::size_t
        BenchmarkSession::CurrentScenarioNumber()
        const noexcept
    {
        return scenarioIndex_ + 1;
    }

    float BenchmarkSession::PhaseElapsedSeconds()
        const noexcept
    {
        return phaseElapsedSeconds_;
    }
}