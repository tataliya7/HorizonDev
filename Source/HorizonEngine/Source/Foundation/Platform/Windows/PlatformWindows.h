#pragma once

#include "Foundation/StdHeaders.h"
#include "Foundation/Definitions.h"
#include "Foundation/FundamentalTypes.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace Horizon
{
    class OSCriticalSection
    {
    public:
        OSCriticalSection(const OSCriticalSection&) = delete;
        OSCriticalSection& operator=(const OSCriticalSection&) = delete;

        OSCriticalSection()
        {
            InitializeCriticalSectionAndSpinCount(&criticalSection, (DWORD)4000);
        }

        ~OSCriticalSection()
        {
            DeleteCriticalSection(&criticalSection);
        }

        CRITICAL_SECTION& GetHandle()
        {
            return criticalSection;
        }

    private:

        CRITICAL_SECTION criticalSection;
    };

    extern bool OSTryEnterCriticalSection(OSCriticalSection* criticalSection);
    extern void OSEnterCriticalSection(OSCriticalSection* criticalSection);
    extern void OSLeaveCriticalSection(OSCriticalSection* criticalSection);

    void OSMemoryBarrier();
}