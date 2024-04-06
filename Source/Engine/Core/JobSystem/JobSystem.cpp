#include "Core/JobSystem/JobSystem.h"

#include <windows.h>
#include <MPMCQueue.h>
#include <optick.h>

namespace HE
{
    /**
     * A bounded multi-producer multi-consumer concurrent queue written in C++11.
     * Source code: https://github.com/rigtorp/MPMCQueue.
     */
    template<typename T>
    using MPMCQueue = rigtorp::mpmc::Queue<T>;

    struct JobSystemQueuedJob
    {
        JobSystemJobDecl workload;
        JobSystemCounterHandle counter;
    };

    struct Semaphore
    {
        uint64 handle;
    };

    struct JobSystemFiber;
    struct JobSystemWaitingJob
    {
        uint32 condition;
        JobSystemCounterHandle counter;
        JobSystemFiber* fiber;
    };

    struct JobSystemFiber
    {
        uint64 handle;
        uint32 index;
        JobSystemWaitingJob waitingJobToSchedule;
    };

    struct JobSystemThread
    {
        std::wstring name;
        uint32 index;
        uint64 handle;
        uint32 threadID;
    };

    struct JobSystemAtomicCounter
    {
        uint32 index;
        std::atomic<uint32> atomic;
    };

    std::atomic<bool> GJobSystemRequestQuit;
    std::atomic<int> GJobSystemBootAtomicCounter;
    std::atomic<uint32> GJobSystemNextWorkerThreadIndex;
    std::map<uint32, uint32> GJobSystemSemaphoreLookupTable;
    Semaphore GJobSystemSemaphores[JOB_SYSTEM_MAX_WORKER_THREAD_COUNT];
    uint32 GJobSystemWorkerThreadCount;
    uint32 GJobSystemFiberCount;
    MPMCQueue<JobSystemQueuedJob> GJobSystemLowPriorityJobQueue(JOB_SYSTEM_MAX_ATOMIC_COUNTER_COUNT);
    MPMCQueue<JobSystemQueuedJob> GJobSystemNormalPriorityJobQueue(JOB_SYSTEM_MAX_ATOMIC_COUNTER_COUNT);
    MPMCQueue<JobSystemQueuedJob> GJobSystemHighPriorityJobQueue(JOB_SYSTEM_MAX_ATOMIC_COUNTER_COUNT);
    MPMCQueue<JobSystemWaitingJob> GJobSystemWaitList(JOB_SYSTEM_MAX_FIBER_COUNT);
    std::map<uint32, uint32> GJobSystemWorkerThreadLookupTable;
    JobSystemThread GJobSystemWorkerThreads[JOB_SYSTEM_MAX_WORKER_THREAD_COUNT];
    JobSystemFiber GJobSystemFibers[JOB_SYSTEM_MAX_FIBER_COUNT];
    MPMCQueue<uint32> GJobSystemFreeFiberList(JOB_SYSTEM_MAX_FIBER_COUNT);
    JobSystemAtomicCounter GJobSystemAtomicCounters[JOB_SYSTEM_MAX_ATOMIC_COUNTER_COUNT];
    MPMCQueue<uint32> GJobSystemFreeCounterQueue(JOB_SYSTEM_MAX_ATOMIC_COUNTER_COUNT);

    static bool IsJobSystemInitialized()
    {
        return GJobSystemBootAtomicCounter.load() < 0;
    }

    static uint32 GetNumberOfProcessors()
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwNumberOfProcessors;
    }

    static void SuspendCurrentThread(float seconds)
    {
        Sleep((DWORD)(seconds * 1000.0f + 0.5f));
    }

    static void JobSystemYieldCPU()
    {
        YieldProcessor();
    }

    static uint32 JobSystemGetCurrentThreadID()
    {
        return GetCurrentThreadId();
    }

    static void JobSystemSwitchToFiber(uint64 handle)
    {
        SwitchToFiber((LPVOID)handle);
    }

    static uint64 CreateSemaphoreEXT(uint32 initialCount)
    {
        uint64 handle = (uint64)CreateSemaphoreW(NULL, initialCount, INT_MAX, NULL);
        return handle;
    }

    static void SemaphoreAdd(uint64 semaphore, uint32 count)
    {
        ReleaseSemaphore((HANDLE)semaphore, count, NULL);
    }

    static void SemaphoreWait(uint64 semaphore)
    {
        WaitForSingleObject((HANDLE)semaphore, 0xFFFFFFFF);
    }

    static void JobSystemLockThreadToCPUCores(uint64 threadHandle, uint64 mask)
    {
        SetThreadAffinityMask((HANDLE)threadHandle, (DWORD_PTR)mask);
    }

    static JobSystemFiber* GetCurrentFiberData()
    {
        assert(IsThreadAFiber());
        return (JobSystemFiber*)GetFiberData();
    }

    static uint64 JobSystemConvertCurrentThreadToFiber(void* fiber)
    {
        return (uint64)ConvertThreadToFiberEx(fiber, FIBER_FLAG_FLOAT_SWITCH);
    }

    static bool JobSystemConvertCurrentFiberToThread()
    {
        return ConvertFiberToThread();
    }

    static uint32 LoadCounter(JobSystemCounterHandle handle)
    {
        assert(handle);
        uint32 index = handle - 1;
        return GJobSystemAtomicCounters[index].atomic.load(std::memory_order_acquire);
    }

    static void StoreCounter(JobSystemCounterHandle handle, uint32 value)
    {
        assert(handle);
        uint32 index = handle - 1;
        GJobSystemAtomicCounters[index].atomic.store(value);
    }

    static void FetchSubCounter(JobSystemCounterHandle handle)
    {
        assert(handle);
        uint32 index = handle - 1;
        GJobSystemAtomicCounters[index].atomic.fetch_sub(1);

    }

    static void FreeCounter(JobSystemCounterHandle handle)
    {
        assert(handle);
        uint32 index = handle - 1;
        GJobSystemFreeCounterQueue.push(GJobSystemAtomicCounters[index].index);
    }

    static bool FindFreeCounter(uint32& outIndex)
    {
        return GJobSystemFreeCounterQueue.try_pop(outIndex);
    }

    static bool FindFreeFiber(uint32& outIndex)
    {
        return GJobSystemFreeFiberList.try_pop(outIndex);
    }

    static void JobSystemFreeFiber(uint32 fiberIndex)
    {
        GJobSystemFreeFiberList.push(fiberIndex);
    }

    static void JobSystemFiberEntry(JobSystemFiber* currentFiber)
    {
        while (!GJobSystemRequestQuit)
        {
            uint32 threadID = JobSystemGetCurrentThreadID();
            uint32 workerThreadIndex = GJobSystemWorkerThreadLookupTable[threadID];
            const JobSystemThread& thread = GJobSystemWorkerThreads[workerThreadIndex];

            // std::wcout << std::format(L"Thread Name: {}, Fiber Index:{}", thread.name, currentFiber->index) << std::endl;

            if (currentFiber->waitingJobToSchedule.fiber != nullptr)
            {
                GJobSystemWaitList.push(currentFiber->waitingJobToSchedule);
                currentFiber->waitingJobToSchedule.fiber = nullptr;
            }

            JobSystemWaitingJob waitingJob;
            const bool wakeUpAnyWaitingJobs = GJobSystemWaitList.try_pop(waitingJob);
            if (wakeUpAnyWaitingJobs)
            {
                if (LoadCounter(waitingJob.counter) == waitingJob.condition)
                {
                    JobSystemFreeFiber(currentFiber->index);
                    JobSystemSwitchToFiber(waitingJob.fiber->handle);
                    continue;
                }
                else
                {
                    GJobSystemWaitList.push(waitingJob);
                }
            }

            JobSystemQueuedJob queuedJob;
            if (GJobSystemNormalPriorityJobQueue.try_pop(queuedJob))
            {
                assert(queuedJob.workload.func);
                queuedJob.workload.func(queuedJob.workload.data);
                FetchSubCounter(queuedJob.counter);
                continue;
                //std::wcout << std::format(L"Thread Name: {}, Fiber Index:{}", thread.name, currentFiber->index) << std::endl;

                // When a job is resumed we first free the current fiber
                //JobSystemFreeFiber(currentFiber->index);
            }

            if (!wakeUpAnyWaitingJobs)
            {
                SemaphoreWait(GJobSystemSemaphores[GJobSystemSemaphoreLookupTable[threadID]].handle);
            }
        }

        uint32 threadID = JobSystemGetCurrentThreadID();
        uint32 workerThreadIndex = GJobSystemWorkerThreadLookupTable.at(threadID);
        if (currentFiber->handle != GJobSystemFibers[workerThreadIndex].handle)
        {
            JobSystemSwitchToFiber(GJobSystemFibers[workerThreadIndex].handle);
        }
        JobSystemConvertCurrentFiberToThread();
    }

    static void JobSystemWorkerThreadEntry(JobSystemThread* thread)
    {
        OPTICK_THREAD(thread->name.c_str());

        uint32 threadID = thread->threadID;
        uint32 workerThreadIndex = thread->index;

        uint64 initialFiberHandle = JobSystemConvertCurrentThreadToFiber(&GJobSystemFibers[workerThreadIndex]);

        GJobSystemFibers[workerThreadIndex].index = workerThreadIndex;
        GJobSystemFibers[workerThreadIndex].handle = initialFiberHandle;
        GJobSystemFibers[workerThreadIndex].waitingJobToSchedule.fiber = nullptr;

        GJobSystemBootAtomicCounter.fetch_sub(1);
        
        while (!IsJobSystemInitialized())
        {
            JobSystemYieldCPU();
        }

        JobSystemFiberEntry(&GJobSystemFibers[workerThreadIndex]);
    }

    static VOID WINAPI FiberProc(LPVOID lpFiberParameter)
    {
        JobSystemFiber* fiber = (JobSystemFiber*)lpFiberParameter;
        JobSystemFiberEntry(fiber);
    }

    static DWORD WINAPI ThreadProc(LPVOID lpThreadParameter)
    {
        JobSystemThread* thread = (JobSystemThread*)lpThreadParameter;
        JobSystemWorkerThreadEntry(thread);
        return 0;
    }

    static uint64 CreateFiber(uint32 stackSize, JobSystemFiber* fiber)
    {
        uint64 handle = (uint64)CreateFiberEx(stackSize, stackSize, FIBER_FLAG_FLOAT_SWITCH, FiberProc, fiber);
        return handle;
    }

    static void CreateWokerThread(uint32 stackSize, JobSystemThread* thread)
    {
        DWORD threadID;
        HANDLE handle = CreateThread(NULL, stackSize, ThreadProc, thread, CREATE_SUSPENDED, &threadID);
        assert(handle);

        if (!thread->name.empty())
        {
            SetThreadDescription(handle, thread->name.c_str());
        }
        
        // Worker thread are locked to cores
        // Avoid context swtitches and unwanted core switching
        // Kernel threads can otherwise cause ripple effects across the cores 
        DWORD_PTR affinityMask = 1ull << thread->index;
        assert(SetThreadAffinityMask(handle, affinityMask) > 0);

        assert(SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST) != FALSE);

        ResumeThread(handle);

        memcpy(&thread->handle, &handle, sizeof(handle));
        thread->threadID = threadID;
    }

    void JobSystemInit(uint32 workerThreadCount, uint32 fiberCount, uint32 fiberStackSize)
    {
        assert(!IsJobSystemInitialized());
        assert(workerThreadCount <= JOB_SYSTEM_MAX_WORKER_THREAD_COUNT);
        assert((fiberCount <= JOB_SYSTEM_MAX_FIBER_COUNT) && (fiberCount >= workerThreadCount) && ((fiberCount & (fiberCount - 1)) == 0));

        GJobSystemBootAtomicCounter.store(workerThreadCount);
        GJobSystemRequestQuit.store(false);

        for (uint32 workerThreadIndex = 0; workerThreadIndex < workerThreadCount; workerThreadIndex++)
        {
            GJobSystemWorkerThreads[workerThreadIndex].name = std::format(L"JobSystemWorkerThread {}", workerThreadIndex);
            GJobSystemWorkerThreads[workerThreadIndex].index = workerThreadIndex;
            CreateWokerThread(0, &GJobSystemWorkerThreads[workerThreadIndex]);
            GJobSystemWorkerThreadLookupTable.emplace(GJobSystemWorkerThreads[workerThreadIndex].threadID, GJobSystemWorkerThreads[workerThreadIndex].index);

            GJobSystemSemaphores[workerThreadIndex].handle = CreateSemaphoreEXT(0);

            const uint32 key = GJobSystemWorkerThreads[workerThreadIndex].threadID;
            GJobSystemSemaphoreLookupTable.emplace(key, workerThreadIndex);
        }

        // Wait until all worker threads are initialized
        while (GJobSystemBootAtomicCounter.load() != 0)
        {
            SuspendCurrentThread(0.01f);
        }

        // TODO: simplify this
        while (!GJobSystemFreeCounterQueue.empty())
        {
            uint32 temp;
            GJobSystemFreeCounterQueue.pop(temp);
        }

        for (uint32 counterIndex = 0; counterIndex < JOB_SYSTEM_MAX_ATOMIC_COUNTER_COUNT; counterIndex++)
        {
            GJobSystemAtomicCounters[counterIndex].index = counterIndex;
            GJobSystemFreeCounterQueue.push(counterIndex);
        }

        for (uint32 fiberIndex = workerThreadCount; fiberIndex < fiberCount; fiberIndex++)
        {
            GJobSystemFibers[fiberIndex].handle = CreateFiber(fiberStackSize, &GJobSystemFibers[fiberIndex]);
            GJobSystemFibers[fiberIndex].index = fiberIndex;

            GJobSystemFreeFiberList.push(fiberIndex);
        }

        GJobSystemNextWorkerThreadIndex = 0;
        GJobSystemWorkerThreadCount = workerThreadCount;
        GJobSystemFiberCount = fiberCount;
        assert(GJobSystemBootAtomicCounter.load() == 0);
        GJobSystemBootAtomicCounter.store(-1);
    }

    void JobSystemExit()
    {
        assert(IsJobSystemInitialized());
        // TODO

        GJobSystemRequestQuit.store(true);

        // for ()
        // {
        //     DeleteFiber();
        // }
    }

    JobSystemCounterHandle JobSystemRunJobs(JobSystemJobDecl* jobs, uint32 jobCount)
    {
        assert(IsJobSystemInitialized());

        uint32 freeCounterIndex;
        while (!FindFreeCounter(freeCounterIndex));

        JobSystemCounterHandle freeCounter = freeCounterIndex + 1;
        StoreCounter(freeCounter, jobCount);

        JobSystemQueuedJob job = {};
        job.counter = freeCounter;

        for (uint32 jobIndex = 0; jobIndex < jobCount; jobIndex++)
        {
            job.workload = jobs[jobIndex];

            GJobSystemNormalPriorityJobQueue.push(job);

            uint32 workerThreadIndex = GJobSystemNextWorkerThreadIndex.fetch_add(1);
            SemaphoreAdd(GJobSystemSemaphores[workerThreadIndex % GJobSystemWorkerThreadCount].handle, 1);
        }

        return freeCounter;
    }

    void JobSystemWaitForCounter(JobSystemCounterHandle counter)
    {
        assert(IsJobSystemInitialized());

        if (LoadCounter(counter) != 0)
        {
            uint32 freeFiberIndex;
            while (!FindFreeFiber(freeFiberIndex));

            JobSystemFiber* currentFiber = GetCurrentFiberData();
            JobSystemFiber* nextFiber = &GJobSystemFibers[freeFiberIndex];
            nextFiber->waitingJobToSchedule = {
                0,
                counter,
                currentFiber
            };

            JobSystemSwitchToFiber(nextFiber->handle);
        }
    }

    void JobSystemWaitForCounterAndFree(JobSystemCounterHandle counter)
    {
        assert(IsJobSystemInitialized());

        JobSystemWaitForCounter(counter);
        FreeCounter(counter);
    }

    void JobSystemWaitForCounterAndFreeWithoutFiber(JobSystemCounterHandle counter)
    {
        assert(IsJobSystemInitialized());
        // TODO: check main thread

        while (LoadCounter(counter) != 0);
        FreeCounter(counter);
    }
}
