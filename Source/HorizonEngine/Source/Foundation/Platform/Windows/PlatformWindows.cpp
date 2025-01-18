#include "../PlatformGenericAPI.h"
#include "PlatformWindows.h"

#include "Foundation/Logging/Logging.h"

namespace Horizon
{
    void OSYieldCPUProcessor()
    {
        YieldProcessor();
    }

    uint32 OSGetNumberOfProcessors()
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwNumberOfProcessors;
    }

    uint32 OSGetCurrentThreadID()
    {
        return GetCurrentThreadId();
    }

    void OSSuspendCurrentThread(float seconds)
    {
        Sleep(DWORD(lroundf(seconds * 1000.0f)));
    }

    void OSSwitchToAnotherFiber(uint64 handle)
    {
        SwitchToFiber(reinterpret_cast<void*>(handle));
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

    void OSMemoryBarrier()
    {
        // x86
        _mm_sfence();
    }

    OSLibraryHandle OSLoadLibrary(const char* filename)
    {
        std::filesystem::path absolutePath = std::filesystem::absolute(filename);

        if (std::filesystem::exists(absolutePath))
        {
            // Try to find a loaded library.
            HMODULE handle = GetModuleHandleW(absolutePath.wstring().c_str());

            if (handle != NULL)
            {
                return handle;
            }

            handle = LoadLibraryW(absolutePath.wstring().c_str());

            if (handle != NULL)
            {
                LogVerbose(GLogger, std::format("Loaded: {}.", absolutePath.string()));
            }
            else
            {
                DWORD lastError = GetLastError();
                LogError(GLogger, std::format("Failed to load library: {}. Error: {}.", absolutePath.string(), lastError));
            }

            return handle;
        }

        return NULL;
    }

    void OSFreeLibrary(OSLibraryHandle handle)
    {
        FreeLibrary(static_cast<HMODULE>(handle));
    }

    void* OSGetSymbolAddressFromLibrary(OSLibraryHandle handle, const char* name)
    {
        FARPROC proc = GetProcAddress(static_cast<HMODULE>(handle), name);
        if (proc == NULL)
        {
            DWORD lastError = GetLastError();
            return nullptr;
        }
        return reinterpret_cast<void*>(proc);
    }
}