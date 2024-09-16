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

        bool IsInitialized() const
        {
            return isInitialized;
        }

        bool CheckReflexSupport() const
        {
            return isReflexSupported;
        }

        void GetFrameToken(uint32 frameIndex);

        //bool ReflexSetOptions(const sl::ReflexOptions& options);
        void ReflexSleep(uint32 frameIndex);
        void ReflexSetMarkerControllerInputSample(uint32 frameIndex);
        void ReflexSetMarkerSimulationStart(uint32 frameIndex);
        void ReflexSetMarkerSimulationEnd(uint32 frameIndex);
        void ReflexSetMarkerRenderSubmitStart(uint32 frameIndex);
        void ReflexSetMarkerRenderSubmitEnd(uint32 frameIndex);
        void ReflexSetMarkerPresentStart(uint32 frameIndex);
        void ReflexSetMarkerPresentEnd(uint32 frameIndex);
        void ReflexSetMarkerPCLatencyPing(uint32 frameIndex);

    private:

        //sl::FrameToken* currentFrameToken = nullptr;
        //void* currentFrameToken = nullptr;

        bool isInitialized = true;
        bool isReflexSupported = true;
    };
}