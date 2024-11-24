#pragma once

#include "Foundation/StdHeaders.h"
#include "Foundation/Definitions.h"
#include "Foundation/FundamentalTypes.h"

namespace Horizon
{
    void OSYieldCPUProcessor();

    uint32 OSGetNumberOfProcessors();

    uint32 OSGetCurrentThreadID();

    void OSSuspendCurrentThread(float seconds);

    void OSSwitchToAnotherFiber(uint64 handle);

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
}