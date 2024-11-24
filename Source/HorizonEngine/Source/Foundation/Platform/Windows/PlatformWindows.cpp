#include "../PlatformGenericAPI.h"
#include "PlatformWindows.h"

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
}