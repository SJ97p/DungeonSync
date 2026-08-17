#include "ProcessMemorySampler.h"

#include <Windows.h>
#include <Psapi.h>

namespace DungeonSync::Diagnostics
{
    ProcessMemorySnapshot
        ProcessMemorySampler::Capture() noexcept
    {
        PROCESS_MEMORY_COUNTERS_EX counters{};
        counters.cb = sizeof(counters);

        const BOOL succeeded = GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(
                &counters),
            sizeof(counters));

        if (succeeded == FALSE)
        {
            return {};
        }

        ProcessMemorySnapshot snapshot{};
        snapshot.workingSetBytes = counters.WorkingSetSize;
        snapshot.peakWorkingSetBytes =
            counters.PeakWorkingSetSize;
        snapshot.privateBytes = counters.PrivateUsage;
        snapshot.valid = true;

        return snapshot;
    }
}
