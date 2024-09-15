#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    class StreamlineContext
    {
    public:

        void Init();
        void Exit();

        void Test(RenderBackend* renderBackend);
        //void GetNewFrameToken();

        bool IsInitialized() const
        {
            return isInitialized;
        }

        bool CheckReflexSupport() const
        {
            return isReflexSupported;
        }

        // bool ReflexSetOptions(const sl::ReflexOptions& options);
        // void ReflexSleep();
        // void ReflexSetMarkerControllerInputSample();
        // void ReflexSetMarkerSimulationStart();
        // void ReflexSetMarkerSimulationEnd();
        // void ReflexSetMarkerRenderSubmitStart();
        // void ReflexSetMarkerRenderSubmitEnd();
        // void ReflexSetMarkerPresentStart();
        // void ReflexSetMarkerPresentEnd();
        // //void ReflexSetMarkerTriggerFlash();
        // void ReflexSetMarkerPCLatencyPing();

    private:

        //sl::FrameToken* currentFrameToken = nullptr;

        bool isInitialized = true;
        bool isReflexSupported = true;
    };
}