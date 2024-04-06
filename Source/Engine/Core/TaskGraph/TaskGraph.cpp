module;

#include "Core/CoreModule.h"

module HorizonEngine.Core.TaskGraph;

namespace HE
{
    class TaskGraphSystemTaskGraph;
    static TaskGraphSystemTaskGraph* GTaskGraph = nullptr;

    std::atomic<uint32> GTaskGraphSystemEnterCSCounter = 0;
    std::atomic<uint32> GTaskGraphSystemLeaveCSCounter = 0;
    std::atomic<uint32> GLastThreadID = 0;

    bool TaskGraphSystemEvent::AddSubsequent(TaskGraphSystemTaskNodeBase* task)
    {
        OSEnterCriticalSection(&subsequentsCS);
        bool succeeded = false;
        if (IsCompleted())
        {
            succeeded = false;
        }
        else
        {
            subsequents.push_back(task);
            succeeded = true;
        }
        OSLeaveCriticalSection(&subsequentsCS);

        return succeeded;
    }

    void TaskGraphSystemEvent::DispatchSubsequents()
    {
        OSEnterCriticalSection(&subsequentsCS);

        isCompleted.store(true);

        OSMemoryBarrier();

        std::vector<TaskGraphSystemTaskNodeBase*> tasks = subsequents;
        subsequents.clear();

        OSLeaveCriticalSection(&subsequentsCS);

        for (uint32 i = 0; i < tasks.size(); i++)
        {
            tasks[i]->PrerequisitesComplete(1);
        }
    }

    class TaskGraphSystemPendingTaskQueue
    {
    public:
        TaskGraphSystemPendingTaskQueue()
        {

        }

        ~TaskGraphSystemPendingTaskQueue()
        {

        }

        void Push(TaskGraphSystemTaskNodeBase* task)
        {
            OSEnterCriticalSection(&queueCS);
            queue.push(task);
            OSLeaveCriticalSection(&queueCS);
        }

        TaskGraphSystemTaskNodeBase* Pop()
        {
            OSEnterCriticalSection(&queueCS);
            GTaskGraphSystemEnterCSCounter++;
            GLastThreadID.store(OSGetCurrentThreadID());
            TaskGraphSystemTaskNodeBase* task = nullptr;
            if (!queue.empty())
            {
                task = queue.front();
                queue.pop();
            }
            OSLeaveCriticalSection(&queueCS);
            GTaskGraphSystemLeaveCSCounter++;
            return task;
        }

    private:
        OSCriticalSection queueCS;
        std::queue<TaskGraphSystemTaskNodeBase*> queue;
    };

    class TaskGraphSystemWorkerThreadRunnable final : public Runnable
    {
    public:

        void RequestQuit()
        {
            requestQuit = true;
            WakeUp();
        }

        void WakeUp()
        {
            OSSignalEvent(wakeUpEvent);
        }

        bool Init() override
        {
            return true;
        }

        uint32 Run() override
        {
            while (!requestQuit)
            {
                ProcessTasks();
            }

            return 0;
        }

        void Stop() override
        {
            RequestQuit();
        }

        virtual void Exit() override
        {

        }

    private:

        uint64 ProcessTasks();

        bool requestQuit;
        OSEventHandle wakeUpEvent;
        std::vector<TaskGraphSystemTaskNodeBase*> tasks;
    };

    struct TaskGraphSystemWorkerThread
    {
        TaskGraphSystemWorkerThreadRunnable* runnable;
        RunnableThread* runnableThread;
    };

    class TaskGraphSystemTaskGraph
    {
    public:
        TaskGraphSystemTaskGraph(uint32 numWorkerThreads)
            : numWorkerThreads(numWorkerThreads)
            , pendingTasks()
        {

        }

        ~TaskGraphSystemTaskGraph()
        {

        }

        void Init()
        {
            if (TaskGraphSystemIsMultiThreaded())
            {
                workerThreads.resize(numWorkerThreads);
                for (uint32 threadIndex = 0; threadIndex < numWorkerThreads; threadIndex++)
                {
                    workerThreads[threadIndex].runnable = new TaskGraphSystemWorkerThreadRunnable();
                    workerThreads[threadIndex].runnableThread = RunnableThread::Create(
                        workerThreads[threadIndex].runnable,
                        std::format(L"TaskGraphSystemWorkerThread{}", threadIndex).c_str());
                }
            }
            else
            {
                // TODO
            }
        }

        void Exit()
        {
            if (TaskGraphSystemIsMultiThreaded())
            {
                for (uint32 threadIndex = 0; threadIndex < numWorkerThreads; threadIndex++)
                {
                    workerThreads[threadIndex].runnable->RequestQuit();
                }

                for (uint32 threadIndex = 0; threadIndex < numWorkerThreads; threadIndex++)
                {
                    delete workerThreads[threadIndex].runnableThread;
                    delete workerThreads[threadIndex].runnable;
                }
                workerThreads.clear();
            }
            else
            {
                // TODO
            }
        }

        void EnqueueTask(TaskGraphSystemTaskNodeBase* task)
        {
            if (TaskGraphSystemIsMultiThreaded())
            {
                pendingTasks.Push(task);
            }
            else
            {
                // TODO
            }
        }

        TaskGraphSystemTaskNodeBase* DequeueTask()
        {
            return pendingTasks.Pop();
        }

    private:

        TaskGraphSystemWorkerThread& GetWorkerThread(uint32 index)
        {
            return workerThreads[index];
        }

        void WakeUpThread(uint32 threadIndex)
        {
            GetWorkerThread(threadIndex).runnable->WakeUp();
        }

        uint32 numWorkerThreads;
        std::vector<TaskGraphSystemWorkerThread> workerThreads;
        TaskGraphSystemPendingTaskQueue pendingTasks;
    };

    void TaskGraphSystemTaskNodeBase::PrerequisitesComplete(uint32 numAlreadyFinishedPrequistes)
    {
        if (numPrerequisitesOutstanding.fetch_sub(numAlreadyFinishedPrequistes) == numAlreadyFinishedPrequistes)
        {
            GTaskGraph->EnqueueTask(this);
        }
    }

    uint64 TaskGraphSystemWorkerThreadRunnable::ProcessTasks()
    {
        uint64 processedTasks = 0;

        while (true)
        {
            TaskGraphSystemTaskNodeBase* task = GTaskGraph->DequeueTask();
            if (!task)
            {
                if (TaskGraphSystemIsMultiThreaded())
                {
                    OSWaitEvent(wakeUpEvent, UINT32_MAX);
                }

                if (requestQuit || !TaskGraphSystemIsMultiThreaded())
                {
                    break;
                }

                continue;
            }

            task->Execute();

            task->GetCompletionEvent()->DispatchSubsequents();

            bool deleteOnCompletion = true;
            if (deleteOnCompletion)
            {
                delete task;
            }

            processedTasks++;
        }

        return processedTasks;
    }

    void TaskGraphSystemInit(uint32 numWorkerThreads)
    {
        GTaskGraph = new TaskGraphSystemTaskGraph(numWorkerThreads);
        GTaskGraph->Init();
    }

    void TaskGraphSystemExit()
    {
        GTaskGraph->Exit();
        delete GTaskGraph;
        GTaskGraph = nullptr;
    }

    bool TaskGraphSystemIsMultiThreaded()
    {
        // TODO
        return true;
    }

    TaskGraphSystemEventRef TaskGraphSystemDispatchTask(TaskGraphSystemTaskNodeBase* task, TaskGraphSystemEventRef* prerequisites, uint32 numPrerequisites)
    {
        task->SetNumPrerequisitesOutstanding(numPrerequisites);
        uint32 alreadyCompletedPrerequisites = 0;
        if (prerequisites)
        {
            for (uint32 index = 0; index < numPrerequisites; index++)
            {
                TaskGraphSystemEventRef prerequisite = prerequisites[index];
                if (!prerequisite || !prerequisite->AddSubsequent(task))
                {
                    alreadyCompletedPrerequisites++;
                }
            }
        }
        TaskGraphSystemEventRef completionEvent = task->GetCompletionEvent();
        task->PrerequisitesComplete(alreadyCompletedPrerequisites);
        return completionEvent;
    }

    void TaskGraphSystemWaitForEvents(TaskGraphSystemEventRef* events, uint32 numEvents)
    {
        OSEventHandle event = OSCreateEvent(true);

        TaskGraphSystemDispatchTask(TaskGraphSystemTaskNode<TaskGraphSystemSignalEventTask>::Create(event), events, numEvents);

        OSWaitEvent(event, UINT32_MAX);
        OSDestroyEvent(event);
    }
}