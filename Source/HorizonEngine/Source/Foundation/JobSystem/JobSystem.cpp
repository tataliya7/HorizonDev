#include "JobSystem.h"
#include "../Logging/Logging.h"

#include <thread>
#include <semaphore>
#include <condition_variable>

#include <windows.h>
#include <optick.h>
#include <MPMCQueue.h>

namespace Horizon
{
    struct JobSystemThread
    {
        std::string name;
        uint32 index;
        std::thread thread;
    };

    struct JobSystemJobDeclaration
    {
        const char* name;
        JobSystemJobFunction function;
        JobSystemJobPriority priority;
        uint32 dependencyCounterIndex;
        uint32 accumulateCounterIndex;
    };

    struct JobSystemJobCounter
    {
        std::atomic_signed_lock_free counterValue;
        std::atomic_signed_lock_free referenceCount;
        std::list<uint32> waitingJobList;
        std::mutex waitingJobMutex;
    };

    struct JobSystem
    {
        static constexpr uint32 InvalidIndex = std::numeric_limits<uint32>::max();
        static constexpr uint32 MaxWorkerThreadCount = 128;
        static constexpr uint32 MaxJobCount = 8192;

        /**
         * A bounded multi-producer multi-consumer concurrent queue written in C++11.
         * Source code: https://github.com/rigtorp/MPMCQueue.
         */
        template <typename T>
        using JobQueue = rigtorp::mpmc::Queue<T>;

        JobSystem()
            : bootCounter(0)
            , pendingJobCount(0)
            , exitRequested(false)
            , workerThreadCount(0)
            , workerThreads()
            , jobCounters()
            , freeCounters(MaxJobCount)
            , jobs()
            , freeJobs(MaxJobCount)
            , jobQueueLowPriority(MaxJobCount)
            , jobQueueNormalPriority(MaxJobCount)
            , jobQueueHighPriority(MaxJobCount)
        {

        }

        ~JobSystem()
        {

        }

        std::atomic<int> bootCounter;

        std::atomic<int> pendingJobCount;

        std::atomic<bool> exitRequested;

        uint32 workerThreadCount;

        JobSystemThread workerThreads[MaxWorkerThreadCount];

        std::mutex waiterLock;
        std::condition_variable waiterCondition;

        JobSystemJobCounter jobCounters[MaxJobCount];
        JobQueue<uint32> freeCounters;

        JobSystemJobDeclaration jobs[MaxJobCount];
        JobQueue<uint32> freeJobs;

        JobQueue<uint32> jobQueueLowPriority;
        JobQueue<uint32> jobQueueNormalPriority;
        JobQueue<uint32> jobQueueHighPriority;

        bool IsInitialized() const
        {
            return bootCounter.load() < 0;
        }

        bool IsExitRequested() const
        {
            return exitRequested.load(std::memory_order_relaxed);
        }

        bool HasAnyPendingJob() const
        {
            return pendingJobCount.load(std::memory_order_relaxed);
        }

        void RequestExit()
        {
            exitRequested.store(true);
        }

        uint32 AllocateJobCounter()
        {
            uint32 jobCounterIndex = InvalidIndex;
            while (!freeCounters.try_pop(jobCounterIndex))
            {
                // @todo Log warning
            }
            return jobCounterIndex;
        }

        void ReleaseJobCounter(uint32 jobCounterIndex)
        {
            assert(jobCounterIndex != InvalidIndex);
            freeCounters.push(jobCounterIndex);
        }

        void IncrementJobCounterReference(uint32 jobCounterIndex)
        {
            if (jobCounterIndex != InvalidIndex)
            {
                jobCounters[jobCounterIndex].referenceCount.fetch_add(1);
            }
        }

        void DecrementJobCounterReference(uint32 jobCounterIndex)
        {
            if (jobCounterIndex != InvalidIndex)
            {
                if (jobCounters[jobCounterIndex].referenceCount.fetch_sub(1) == 1)
                {
                    ReleaseJobCounter(jobCounterIndex);
                }
            }
        }

        uint32 AllocateJob()
        {
            uint32 jobIndex = InvalidIndex;
            while (!freeJobs.try_pop(jobIndex))
            {
                // @todo Log warning
            }
            return jobIndex;
        }

        void ReleaseJob(uint32 jobIndex)
        {
            assert(jobIndex != InvalidIndex);
            jobs[jobIndex] = {};
            freeJobs.push(jobIndex);
        }

        void EnqueueJob(uint32 jobIndex, JobSystemJobPriority priority)
        {
            assert(jobIndex != InvalidIndex);
            if (priority == JobSystemJobPriority::High)
            {
                jobQueueHighPriority.push(jobIndex);
            }
            else if (priority == JobSystemJobPriority::Normal)
            {
                jobQueueNormalPriority.push(jobIndex);
            }
            else
            {
                jobQueueLowPriority.push(jobIndex);
            }

            pendingJobCount.fetch_add(1);

            waiterCondition.notify_one();
        }

        uint32 DequeueJob()
        {
            uint32 jobIndex = InvalidIndex;

            jobQueueHighPriority.try_pop(jobIndex);

            if (jobIndex == InvalidIndex)
            {
                jobQueueNormalPriority.try_pop(jobIndex);
            }

            if (jobIndex == InvalidIndex)
            {
                jobQueueLowPriority.try_pop(jobIndex);
            }

            return jobIndex;
        }

        bool TryPushWaitingJobList(uint32 counterIndex, uint32 jobIndex)
        {
            bool succeed = false;
            JobSystemJobCounter& jobCounter = jobCounters[counterIndex];
            if (jobCounter.counterValue.load() != 0)
            {
                if (jobCounter.waitingJobMutex.try_lock())
                {
                    if (jobCounter.counterValue.load() != 0)
                    {
                        jobCounter.waitingJobList.emplace_back(jobIndex);
                        succeed = true;
                    }
                    jobCounter.waitingJobMutex.unlock();
                }
            }
            return succeed;
        }

        void FlushWaitingJobList(uint32 counterIndex, uint32 counterValueToSubtract)
        {
            assert(counterIndex != InvalidIndex);
            JobSystemJobCounter& jobCounter = jobCounters[counterIndex];
            if (jobCounter.counterValue.fetch_sub(counterValueToSubtract) == counterValueToSubtract)
            {
                jobCounter.waitingJobMutex.lock();
                while (!jobCounter.waitingJobList.empty())
                {
                    uint32 waitingJobIndex = jobCounter.waitingJobList.front();
                    jobCounter.waitingJobList.pop_front();

                    JobSystemJobDeclaration& waitingJob = jobs[waitingJobIndex];

                    EnqueueJob(waitingJobIndex, waitingJob.priority);
                }
                jobCounter.waitingJobMutex.unlock();
            }
        }
    };

    static JobSystem JobSystemInstance;

    static void JobSystemSetThreadName(const char* name)
    {
        std::string_view utf8name(name);
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8name.data(), static_cast<int>(utf8name.size()), nullptr, 0);

        std::wstring utf16name;
        utf16name.resize(size);
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8name.data(), static_cast<int>(utf8name.size()), utf16name.data(), static_cast<int>(utf16name.size()));

        SetThreadDescription(GetCurrentThread(), utf16name.data());
    }

    static void JobSystemWorkerThreadEntry(JobSystemThread* thread)
    {
        OPTICK_THREAD(thread->name.c_str());
        JobSystemSetThreadName(thread->name.c_str());

        JobSystemInstance.bootCounter.fetch_sub(1);
        while (!JobSystemInstance.IsInitialized())
        {
            std::this_thread::yield();
        }

        // Thread main loop
        while (!JobSystemInstance.IsExitRequested())
        {
            bool finishAnyJob = false;

            uint32 jobIndex = JobSystemInstance.DequeueJob();

            if (jobIndex != JobSystem::InvalidIndex)
            {
                JobSystemJobDeclaration& job = JobSystemInstance.jobs[jobIndex];

                if (job.function)
                {
                    JobSystemJobContext context =
                    {

                    };

                    job.function(context);
                }

                JobSystemInstance.FlushWaitingJobList(job.accumulateCounterIndex, 1);

                JobSystemInstance.DecrementJobCounterReference(job.dependencyCounterIndex);
                JobSystemInstance.DecrementJobCounterReference(job.accumulateCounterIndex);
                JobSystemInstance.ReleaseJob(jobIndex);

                finishAnyJob = true;
            }

            if (!finishAnyJob)
            {
                std::unique_lock lock(JobSystemInstance.waiterLock);
                if (!JobSystemInstance.HasAnyPendingJob())
                {
                    JobSystemInstance.waiterCondition.wait(lock);
                }
            }
        }
    }

    void JobSystemInit(uint32 workerThreadCount)
    {
        //assert(IsInMainThread());
        assert(!JobSystemInstance.IsInitialized());

        if (workerThreadCount == 0)
        {
            unsigned int hardwareConcurrentThreadCount = std::thread::hardware_concurrency();
            workerThreadCount = (hardwareConcurrentThreadCount + 1) / 2;
        }

        workerThreadCount = std::clamp(workerThreadCount, 1u, JobSystem::MaxWorkerThreadCount);
        JobSystemInstance.workerThreadCount = workerThreadCount;

        // Bind the main thread to CPU 0
        //DWORD_PTR affinityMask = 1ull;
        //assert(SetThreadAffinityMask(GetCurrentThread(), affinityMask) > 0);

        JobSystemInstance.bootCounter.store(static_cast<int>(workerThreadCount));
        JobSystemInstance.exitRequested.store(false);

        for (uint32 workerThreadIndex = 0; workerThreadIndex < workerThreadCount; workerThreadIndex++)
        {
            JobSystemThread& workerThread = JobSystemInstance.workerThreads[workerThreadIndex];
            workerThread.name = std::format("JobSystemWorkerThread {}", workerThreadIndex);
            workerThread.index = workerThreadIndex;
            workerThread.thread = std::thread(&JobSystemWorkerThreadEntry, &workerThread);
        }

        // Wait until all worker threads are initialized
        while (JobSystemInstance.bootCounter.load() != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        for (uint32 jobCounterIndex = 0; jobCounterIndex < JobSystem::MaxJobCount; jobCounterIndex++)
        {
            JobSystemInstance.freeCounters.push(jobCounterIndex);
        }

        for (uint32 jobIndex = 0; jobIndex < JobSystem::MaxJobCount; jobIndex++)
        {
            JobSystemInstance.freeJobs.push(jobIndex);
        }

        JobSystemInstance.bootCounter.store(-1);
    }

    void JobSystemExit()
    {
        //assert(IsInMainThread());
        assert(JobSystemInstance.IsInitialized());

        JobSystemInstance.RequestExit();

        for (JobSystemThread& workerThread : JobSystemInstance.workerThreads)
        {
            if (workerThread.thread.joinable())
            {
                workerThread.thread.join();
            }
        }
    }

    JobSystemJobCounterReference JobSystemJobCounterReference::Null = JobSystemJobCounterReference(JobSystem::InvalidIndex);

    JobSystemJobCounterReference::JobSystemJobCounterReference(uint32 handle)
        : handle(handle)
    {

    }

    JobSystemJobCounterReference::~JobSystemJobCounterReference()
    {
        JobSystemInstance.DecrementJobCounterReference(handle);
    }

    JobSystemJobCounterReference::JobSystemJobCounterReference(const JobSystemJobCounterReference& other)
    {
        handle = other.handle;
        JobSystemInstance.IncrementJobCounterReference(handle);
    }

    JobSystemJobCounterReference& JobSystemJobCounterReference::operator=(const JobSystemJobCounterReference& other)
    {
        handle = other.handle;
        JobSystemInstance.IncrementJobCounterReference(handle);
        return *this;
    }

    JobSystemJobCounterReference JobSystemRunJob(const char* name, JobSystemJobPriority priority, const JobSystemJobCounterReference& dependency, const JobSystemJobFunction& function)
    {
        assert(JobSystemInstance.IsInitialized());

        uint32 dependencyCounterIndex = dependency.GetHandle();
        uint32 accumulateCounterIndex = JobSystemInstance.AllocateJobCounter();

        JobSystemJobCounter& accumulateCounter = JobSystemInstance.jobCounters[accumulateCounterIndex];
        accumulateCounter.counterValue.store(1);
        accumulateCounter.referenceCount.store(1);

        uint32 jobIndex = JobSystemInstance.AllocateJob();
        JobSystemJobDeclaration& job = JobSystemInstance.jobs[jobIndex];
        job.name = name;
        job.function = function;
        job.priority = priority;
        job.dependencyCounterIndex = dependencyCounterIndex;
        job.accumulateCounterIndex = accumulateCounterIndex;
        JobSystemInstance.IncrementJobCounterReference(dependencyCounterIndex);
        JobSystemInstance.IncrementJobCounterReference(accumulateCounterIndex);

        bool shouldEnqueueJob = true;
        if (dependencyCounterIndex != JobSystem::InvalidIndex)
        {
            if (JobSystemInstance.TryPushWaitingJobList(dependencyCounterIndex, jobIndex))
            {
                shouldEnqueueJob = false;
            }
        }

        if (shouldEnqueueJob)
        {
            JobSystemInstance.EnqueueJob(jobIndex, priority);
        }

        return accumulateCounterIndex;
    }

    JobSystemJobCounterReference JobSystemCombineDependencies(const JobSystemJobCounterReference* dependencies, uint32 dependencyCount)
    {
        uint32 accumulateCounterIndex = JobSystemInstance.AllocateJobCounter();

        JobSystemJobCounter& accumulateCounter = JobSystemInstance.jobCounters[accumulateCounterIndex];
        accumulateCounter.counterValue.store(dependencyCount);
        accumulateCounter.referenceCount.store(1);

        uint32 waitingJobCount = 0;
        for (uint32 i = 0; i < dependencyCount; i++)
        {
            const JobSystemJobCounterReference& dependency = dependencies[i];
            uint32 dependencyCounterIndex = dependency.GetHandle();

            uint32 jobIndex = JobSystemInstance.AllocateJob();
            JobSystemJobDeclaration& job = JobSystemInstance.jobs[jobIndex];
            job.name = "ResolveDependency";
            job.function = {};
            job.priority = JobSystemJobPriority::High;
            job.dependencyCounterIndex = dependencyCounterIndex;
            job.accumulateCounterIndex = accumulateCounterIndex;
            JobSystemInstance.IncrementJobCounterReference(dependencyCounterIndex);
            JobSystemInstance.IncrementJobCounterReference(accumulateCounterIndex);

            if (dependencyCounterIndex != JobSystem::InvalidIndex)
            {
                if (JobSystemInstance.TryPushWaitingJobList(dependencyCounterIndex, jobIndex))
                {
                    waitingJobCount++;
                }
                else
                {
                    JobSystemInstance.DecrementJobCounterReference(dependencyCounterIndex);
                    JobSystemInstance.DecrementJobCounterReference(accumulateCounterIndex);
                    JobSystemInstance.ReleaseJob(jobIndex);
                }
            }
        }

        uint32 finishedDependencyCount = dependencyCount - waitingJobCount;
        JobSystemInstance.FlushWaitingJobList(accumulateCounterIndex, finishedDependencyCount);

        return accumulateCounterIndex;
    }

    void JobSystemWaitForCounter(const JobSystemJobCounterReference& counter)
    {
        assert(JobSystemInstance.IsInitialized());

        uint32 jobCounterIndex = counter.GetHandle();
        const JobSystemJobCounter& jobCounter = JobSystemInstance.jobCounters[jobCounterIndex];
        while (jobCounter.counterValue.load() != 0)
        {

        }
    }
}