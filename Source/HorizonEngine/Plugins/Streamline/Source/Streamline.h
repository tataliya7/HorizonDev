#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"
#include "Engine/Core/CoreModule.h"

namespace Horizon
{
    class StreamlineContext : public FrameListener
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

        void OnControllerInputSample(uint32 frameIndex) override;
        void OnSimulationBegin(uint32 frameIndex) override;
        void OnSimulationEnd(uint32 frameIndex) override;
        void OnRenderSubmitBegin(uint32 frameIndex) override;
        void OnRenderSubmitEnd(uint32 frameIndex) override;
        void OnPresentBegin(uint32 frameIndex) override;
        void OnPresentEnd(uint32 frameIndex) override;

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