#pragma once

#include "Foundation/StdHeaders.h"
#include "Foundation/Definitions.h"
#include "Foundation/FundamentalTypes.h"

namespace Horizon
{
    enum
    {
        JOB_SYSTEM_MAX_WORKER_THREAD_COUNT = 128,
        JOB_SYSTEM_MAX_FIBER_COUNT = 256,
        JOB_SYSTEM_MAX_ATOMIC_COUNTER_COUNT = 4096,
    };

    using JobSystemCounterHandle = uint32;

    enum class JobSystemPriority
    {
        Low,
        Normal,
        High,
    };

    struct JobSystemJobContext
    {

    };

    using JobSystemJobFunction = std::function<void(JobSystemJobContext)>;

    void JobSystemInit(uint32 workerThreadCount, uint32 fiberCount, uint32 fiberStackSize);
    void JobSystemExit();
    void JobSystemWaitForCounter(JobSystemCounterHandle counter);
    void JobSystemWaitForCounterAndFree(JobSystemCounterHandle counter);
    void JobSystemWaitForCounterAndFreeWithoutFiber(JobSystemCounterHandle counter);

    JobSystemCounterHandle JobSystemDispatchJob(const char* jobName, JobSystemPriority jobPriority, const JobSystemJobFunction& jobFunction);
}