#pragma once

#include "Foundation/Definitions.h"
#include "Foundation/StdHeaders.h"
#include "Foundation/FundamentalTypes.h"

namespace Horizon
{
    enum class JobSystemJobPriority : uint8
    {
        Low,
        Normal,
        High,
    };

    struct JobSystemJobContext
    {

    };

    using JobSystemJobFunction = std::function<void(const JobSystemJobContext&)>;

    class JobSystemJobCounterReference
    {
    public:

        static JobSystemJobCounterReference Null;

        ~JobSystemJobCounterReference();

        JobSystemJobCounterReference(const JobSystemJobCounterReference& other);
        JobSystemJobCounterReference& operator=(const JobSystemJobCounterReference& other);

        JobSystemJobCounterReference(JobSystemJobCounterReference&& other) = delete;
        JobSystemJobCounterReference& operator=(JobSystemJobCounterReference&& other) = delete;

        uint32 GetHandle() const
        {
            return handle;
        }

    private:

        friend JobSystemJobCounterReference JobSystemRunJob(const char* name, JobSystemJobPriority priority, const JobSystemJobCounterReference& dependency, const JobSystemJobFunction& function);

        friend JobSystemJobCounterReference JobSystemCombineDependencies(const JobSystemJobCounterReference* dependencies, uint32 dependencyCount);

        JobSystemJobCounterReference(uint32 handle);

        uint32 handle;
    };

    void JobSystemInit(uint32 workerThreadCount);

    void JobSystemExit();

    JobSystemJobCounterReference JobSystemRunJob(const char* name, JobSystemJobPriority priority, const JobSystemJobCounterReference& dependency, const JobSystemJobFunction& function);

    JobSystemJobCounterReference JobSystemCombineDependencies(const JobSystemJobCounterReference* dependencies, uint32 dependencyCount);

    void JobSystemWaitForCounter(const JobSystemJobCounterReference& counter);

}