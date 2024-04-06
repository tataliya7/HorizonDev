module;

#include "Core/CoreModule.h"

export module HorizonEngine.Core.TaskGraph;

export namespace HE
{
    class TaskGraphSystemEvent;
    class TaskGraphSystemTaskNodeBase;

    using TaskGraphSystemEventRef = std::shared_ptr<TaskGraphSystemEvent>;

    extern void TaskGraphSystemInit(uint32 numWorkerThreads);
    extern void TaskGraphSystemExit();
    extern bool TaskGraphSystemIsMultiThreaded();
    extern TaskGraphSystemEventRef TaskGraphSystemDispatchTask(TaskGraphSystemTaskNodeBase* task, TaskGraphSystemEventRef* prerequisites, uint32 numPrerequisites);
    extern void TaskGraphSystemWaitForEvents(TaskGraphSystemEventRef* events, uint32 numEvents);

    enum class TaskGraphSystemTaskState
    {
        Cancelled,
        Completed,
        Failed,
        Pending,
        Running,
    };

    class TaskGraphSystemEvent
    {
    public:

        TaskGraphSystemEvent()
            : isCompleted(false)
        {

        }

        virtual ~TaskGraphSystemEvent()
        {

        }

        bool IsCompleted() const
        {
            return isCompleted.load();
        }

        bool AddSubsequent(TaskGraphSystemTaskNodeBase* task);

        void DispatchSubsequents();

    private:
        std::atomic<bool> isCompleted;
        OSCriticalSection subsequentsCS;
        std::vector<TaskGraphSystemTaskNodeBase*> subsequents;
    };

    class TaskGraphSystemTaskNodeBase
    {
    public:

        TaskGraphSystemTaskNodeBase(TaskGraphSystemEventRef completionEvent)
            : numPrerequisitesOutstanding(0)
            , completionEvent(completionEvent)
        {

        }

        virtual ~TaskGraphSystemTaskNodeBase()
        {

        }

        virtual void Execute() = 0;

        void PrerequisitesComplete(uint32 numAlreadyFinishedPrequistes);

        TaskGraphSystemEventRef GetCompletionEvent()
        {
            return completionEvent;
        }

        void SetNumPrerequisitesOutstanding(int32 value)
        {
            numPrerequisitesOutstanding.store(value);
        }

    protected:

        std::atomic<int32> numPrerequisitesOutstanding;

        TaskGraphSystemEventRef completionEvent;
    };

    template <typename TaskType>
    class TaskGraphSystemTaskNode final : public TaskGraphSystemTaskNodeBase
    {
    public:

        TaskGraphSystemTaskNode(TaskGraphSystemEventRef completionEvent)
            : TaskGraphSystemTaskNodeBase(completionEvent)
            , userDefinedTask()
        {

        }

        virtual ~TaskGraphSystemTaskNode()
        {

        }

        template <typename... Args>
        static TaskGraphSystemTaskNode* Create(Args&&... args)
        {
            TaskGraphSystemEventRef completionEvent = TaskGraphSystemEventRef(new TaskGraphSystemEvent()); // TODO: pool?
            TaskGraphSystemTaskNode* node = new TaskGraphSystemTaskNode(completionEvent);
            new((void*)&node->userDefinedTask) TaskType(std::forward<Args>(args)...);
            return node;
        }

    private:

        void Execute() override
        {
            TaskType* task = (TaskType*)&userDefinedTask;
            task->Execute();
            task->~TaskType();
        }

        template <uint64 size, uint32 alignment>
        struct AlignedBytes
        {
            alignas(alignment) uint8 data[size];
        };

        AlignedBytes<sizeof(TaskType), alignof(TaskType)> userDefinedTask;
    };

    class TaskGraphSystemSignalEventTask
    {
    public:

        TaskGraphSystemSignalEventTask(OSEventHandle event)
            : event(event)
        {

        }

        void Execute()
        {
            OSSignalEvent(event);
        }

    private:

        OSEventHandle event;
    };

    class TaskGraphSystemLamdaTaskImpl
    {
    public:

        using Function = std::function<void()>;

        TaskGraphSystemLamdaTaskImpl(Function&& function)
            : function(function)
        {

        }

        void Execute()
        {
            function();
        }

    private:

        Function function;
    };

    using TaskGraphSystemLamdaTask = TaskGraphSystemTaskNode<TaskGraphSystemLamdaTaskImpl>;
}