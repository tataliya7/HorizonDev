#pragma once

#include "Core/CoreCommon.h"

namespace HE
{
    struct DateTime
    {
        uint64 time;
    };

    struct FileStatData
    {
        bool isValid;
        bool isDirectory;
        bool isReadOnly;
        DateTime creationTime;
        DateTime accessTime;
        DateTime modificationTime;
        int64 size;
    };

    const char* GetBaseName(const char* path);
    std::string GetDirectory(const std::string& path);
    uint32 GetNumberOfProcessors();
    FileStatData GetFileAttributeData(const char* path);
    void OSSuspendCurrentThread(float seconds);
    void YieldCPU();
    uint32 OSGetCurrentThreadID();
    void SwitchToAnotherFiber(uint64 handle);
    uint64 OpenDLL(const char* path);
    void* GetSymbolFromDLL(uint64 handle, const char* name);
    void CloseDLL(uint64 handle);
    bool GetExePath(char* path, uint32 size);
    void FindFiles(const std::string& directory, const std::string& extension, std::vector<std::string>& outPaths);

    struct OSEventHandle
    {
        uint64 handle = 0;

        operator bool() const
        {
            return (handle != 0);
        }
    };

    extern OSEventHandle OSCreateEvent(bool manualReset);
    extern void OSDestroyEvent(OSEventHandle event);
    extern void OSSignalEvent(OSEventHandle event);
    extern void OSResetEvent(OSEventHandle event);
    extern bool OSWaitEvent(OSEventHandle event, uint32 waitTime);

    enum class OSThreadPriority
    {
        Lowest,
        BelowNormal,
        Normal,
        AboveNormal,
        Highest,
        TimeCritical,
    };

    class Runnable
    {
    public:
        virtual bool Init() { return true; }
        virtual uint32 Run() = 0;
        virtual void Stop() {}
        virtual void Exit() {}
    };

    class RunnableThread
    {
    public:

        static RunnableThread* Create(
            Runnable* runnable,
            const wchar_t* threadName,
            uint32 stackSize = 0,
            OSThreadPriority priority = OSThreadPriority::Normal);

        RunnableThread(Runnable* runnable, const wchar_t* threadName, OSThreadPriority priority)
            : runnable(runnable)
            , name(threadName)
            , priority(priority)
        {

        }

        virtual ~RunnableThread()
        {

        }

        const std::wstring& GetThreadName() const
        {
            return name;
        }

        uint32 GetThreadID() const
        {
            return threadID;
        }

        OSThreadPriority GetPriority() const
        {
            return priority;
        }

        virtual void WaitForCompletion() = 0;

    protected:

        virtual bool Init(uint32 stackSize) = 0;

        std::wstring name;

        uint32 threadID;

        OSThreadPriority priority;

        Runnable* runnable;
    };
}