#include "Core/CoreCommon.h"
#include "Core/Platform/PlatformGeneric.h"
#include "Core/Logging/Logging.h"
#include "Core/Platform/Windows/PlatformWindows.h"

#define NOMINMAX
#include <windows.h>

namespace Horizon
{
    uint64 gMainThreadID = 0;

    void OSMemoryBarrier()
    {
        // x86
        _mm_sfence();
    }

    const char* GetBaseName(const char* path)
    {
        const char* fslash = strrchr(path, '/');
        const char* bslash = strrchr(path, '\\');
        const char* slash = fslash > bslash ? fslash : bslash;
        return slash ? slash + 1 : path;
    }

    std::wstring GetDirectory(const std::wstring& path)
    {
        const auto& index = std::max(path.rfind('\\'), path.rfind('/'));
        if (std::wstring::npos != index)
        {
            return path.substr(0, index);
        }
        return TEXT("");
    }

    FileStatData GetFileAttributeData(const char* path)
    {
        FileStatData data = {};

        wchar_t filename[100];
        wsprintf(filename, L"%s", path);

        WIN32_FILE_ATTRIBUTE_DATA win32FileAttributeData;
        DWORD result = GetFileAttributesExW(filename, GetFileExInfoStandard, &win32FileAttributeData);

        data.isValid = (result != 0);
        data.isDirectory = (win32FileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        if (data.isValid)
        {
            data.modificationTime.time = ((uint64)(win32FileAttributeData.ftLastWriteTime.dwHighDateTime) << 32) | win32FileAttributeData.ftLastWriteTime.dwLowDateTime;
        }
        if (data.isValid && !data.isDirectory)
        {
            data.size = ((uint64)(win32FileAttributeData.nFileSizeHigh) << 32) | win32FileAttributeData.nFileSizeLow;
        }

        return data;
    }

    bool IsInMainThread()
    {
        return GetCurrentThreadId() == (DWORD)gMainThreadID;
    }

    uint32 GetNumberOfProcessors()
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwNumberOfProcessors;
    }

    void OSSuspendCurrentThread(float seconds)
    {
        Sleep((DWORD)(seconds * 1000.0f + 0.5f));
    }

    void YieldCPU()
    {
        YieldProcessor();
    }

    uint32 OSGetCurrentThreadID()
    {
        return GetCurrentThreadId();
    }

    void SwitchToAnotherFiber(uint64 handle)
    {
        SwitchToFiber((void*)handle);
    }

    uint64 OpenDLL(const char* path)
    {
        const size_t size = strlen(path) + 1;
        wchar_t* pathW = new wchar_t[size];
        mbstowcs(pathW, path, size);
        DWORD len = GetFullPathNameW(pathW, 0, 0, 0);
        wchar_t* fullPathW = new wchar_t[len];
        GetFullPathNameW(pathW, len, fullPathW, 0);
        HMODULE handle = LoadLibraryExW(fullPathW, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!handle)
        {
            const DWORD err = GetLastError();
            LogError(GLogger, std::format("Error occurs when load {}. LoadLibraryEx() returns error code: {}.", path, err));
        }
        delete[] pathW;
        delete[] fullPathW;
        return (uint64)handle;
    }

    void* GetSymbolFromDLL(uint64 handle, const char* name)
    {
        return GetProcAddress((HMODULE)handle, name);
    }

    void CloseDLL(uint64 handle)
    {
        FreeLibrary((HMODULE)handle);
    }

    bool GetExePath(char* path, uint32 size)
    {
        const DWORD result = GetModuleFileNameA(0, path, size);
        if (result == 0)
        {
            return false;
        }
        else if (result >= size)
        {
            return false;
        }
        return true;
    }

    void FindFiles(const std::wstring& directory, const std::wstring& extension, std::vector<std::wstring>& outPaths)
    {
        WIN32_FIND_DATA fileData;
        HANDLE hFind;
        std::wstring filename = directory + extension;
        hFind = FindFirstFile(filename.data(), &fileData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            outPaths.push_back(directory + fileData.cFileName);
            while (FindNextFile(hFind, &fileData) != 0)
            {
                outPaths.push_back(directory + fileData.cFileName);
            }
        }
    }

    bool OSTryEnterCriticalSection(OSCriticalSection* criticalSection)
    {
        if (TryEnterCriticalSection(&criticalSection->GetHandle()))
        {
            return true;
        }
        return false;
    }

    void OSEnterCriticalSection(OSCriticalSection* criticalSection)
    {
        EnterCriticalSection(&criticalSection->GetHandle());
    }

    void OSLeaveCriticalSection(OSCriticalSection* criticalSection)
    {
        LeaveCriticalSection(&criticalSection->GetHandle());
    }

    OSEventHandle OSCreateEvent(bool manualReset)
    {
        OSEventHandle event;
        event.handle = (uint64)CreateEvent(nullptr, (BOOL)manualReset, FALSE, nullptr);
        return event;
    }

    void OSDestroyEvent(OSEventHandle event)
    {
        if (event)
        {
            CloseHandle((HANDLE)event.handle);
        }
    }

    void OSSignalEvent(OSEventHandle event)
    {
        SetEvent((HANDLE)event.handle);
    }

    void OSResetEvent(OSEventHandle event)
    {
        ResetEvent((HANDLE)event.handle);
    }

    bool OSWaitEvent(OSEventHandle event, uint32 waitTime)
    {
        return (WaitForSingleObject((HANDLE)event.handle, waitTime) == (DWORD)0x00000000L) ? true : false;
    }

    class WindowsRunnableThread final : public RunnableThread
    {
    public:

        static DWORD ThreadEntry(LPVOID thread)
        {
            assert(thread);
            WindowsRunnableThread* runnableThread = (WindowsRunnableThread*)thread;
            //ThreadManager::Get().AddThread(runnableThread->GetThreadID(), runnableThread);
            return runnableThread->Run();
        }

        WindowsRunnableThread(Runnable* runnable, const wchar_t* threadName, OSThreadPriority priority)
            : RunnableThread(runnable, threadName, priority)
            , threadHandle(NULL)
            , threadInitEvent(NULL)
        {

        }

        ~WindowsRunnableThread()
        {
            if (threadHandle)
            {
                Kill(true);
            }
        }

        bool Kill(bool wait)
        {
            if (runnable)
            {
                runnable->Stop();
            }

            if (wait)
            {
                WaitForCompletion();
            }

            CloseHandle(threadHandle);
            threadHandle = NULL;

            return true;
        }

        uint32 Run()
        {
            uint32 exitCode = 1;
            assert(runnable);

            if (runnable->Init())
            {
                SetEvent(threadInitEvent);

                // TODO: Setup TLS

                exitCode = runnable->Run();

                runnable->Exit();

                // TODO: Cleanup TLS
            }
            else
            {
                SetEvent(threadInitEvent);
            }

            return exitCode;
        }

        void WaitForCompletion() override
        {
            WaitForSingleObject(threadHandle, INFINITE);
        }

    private:
        bool Init(uint32 stackSize) override;
        HANDLE threadHandle;
        HANDLE threadInitEvent;
    };

    static int ConvertToWindowsThreadPriority(OSThreadPriority priority)
    {
        switch (priority)
        {
        case OSThreadPriority::Lowest:          return THREAD_PRIORITY_LOWEST;
        case OSThreadPriority::BelowNormal:     return THREAD_PRIORITY_BELOW_NORMAL;
        case OSThreadPriority::Normal:          return THREAD_PRIORITY_NORMAL;
        case OSThreadPriority::AboveNormal:     return THREAD_PRIORITY_ABOVE_NORMAL;
        case OSThreadPriority::Highest:         return THREAD_PRIORITY_HIGHEST;
        case OSThreadPriority::TimeCritical:    return THREAD_PRIORITY_TIME_CRITICAL;
        default:                                return THREAD_PRIORITY_NORMAL;
        }
    }

    bool WindowsRunnableThread::Init(uint32 stackSize)
    {
        assert(threadInitEvent == NULL);
        threadInitEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (threadInitEvent == NULL)
        {
            return false;
        }
        threadHandle = CreateThread(NULL, stackSize, WindowsRunnableThread::ThreadEntry, this, STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED, (DWORD*)&threadID);
        if (threadHandle)
        {
            SetThreadDescription(threadHandle, name.c_str());

            ResumeThread(threadHandle);
            WaitForSingleObject(threadInitEvent, INFINITE);
        }
        else
        {
            return false;
        }
        CloseHandle(threadInitEvent);
        threadInitEvent = NULL;
        return true;
    }

    RunnableThread* RunnableThread::Create(Runnable* runnable, const wchar_t* threadName, uint32 stackSize, OSThreadPriority priority)
    {
        RunnableThread* newThread = new WindowsRunnableThread(runnable, threadName, priority);

        if (newThread)
        {
            bool succeed = newThread->Init(stackSize);
            if (succeed)
            {
                // TODO
            }
        }

        return newThread;
    }
}