#pragma once

#include <sl.h>
#include <sl_consts.h>
#include <sl_hooks.h>
#include <sl_version.h>

#include <sl_dlss.h>
#include <sl_reflex.h>
#include <sl_dlss_g.h>

namespace Streamline
{
    class StreamlineContext
    {
    public:

        void GetNewFrameToken();

        bool IsInitialized() const
        {
            return isInitialized;
        }

        bool CheckReflexSupport() const
        {
            return isReflexSupported;
        }

        bool ReflexSetOptions(const sl::ReflexOptions& options);
        void ReflexSleep();
        void ReflexSetMarkerInputSample();
        void ReflexSetMarkerSimulationStart();
        void ReflexSetMarkerSimulationEnd();
        void ReflexSetMarkerRenderSubmitStart();
        void ReflexSetMarkerRenderSubmitEnd();
        void ReflexSetMarkerPresentStart();
        void ReflexSetMarkerPresentEnd();
        //void ReflexSetMarkerTriggerFlash();
        void ReflexSetMarkerPCLatencyPing();

    private:

        sl::FrameToken* currentFrameToken = nullptr;

        bool isInitialized = true;
        bool isReflexSupported = true;
    };
}