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

    using JobSystemJobFunction = void(*)(void*);
    struct JobSystemJobDecl
    {
        void* data;
        JobSystemJobFunction func;

        JobSystemJobDecl() : data(nullptr), func(nullptr) {}
        JobSystemJobDecl(void* data, JobSystemJobFunction func) : data(data), func(func) {}
    };

    void JobSystemInit(uint32 workerThreadCount, uint32 fiberCount, uint32 fiberStackSize);
    void JobSystemExit();
    JobSystemCounterHandle JobSystemRunJobs(JobSystemJobDecl* jobs, uint32 jobCount);
    JobSystemCounterHandle JobSystemRunJobs(JobSystemJobDecl* jobs, uint32 jobCount, JobSystemPriority priority);
    void JobSystemWaitForCounter(JobSystemCounterHandle counter);
    void JobSystemWaitForCounterAndFree(JobSystemCounterHandle counter);
    void JobSystemWaitForCounterAndFreeWithoutFiber(JobSystemCounterHandle counter);

    JobSystemCounterHandle JobSystemDispatchJob(const std::function<void(JobSystemJobContext)>& jobFunction);
}