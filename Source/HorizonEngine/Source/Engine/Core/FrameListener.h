#pragma once

#include "Foundation/FoundationModule.h"

namespace Horizon
{
    class FrameListener
    {
    public:
        virtual ~FrameListener() = default;
        virtual void OnControllerInputSample(uint32 frameIndex) {}
        virtual void OnSimulationBegin(uint32 frameIndex) {}
        virtual void OnSimulationEnd(uint32 frameIndex) {}
        virtual void OnRenderSubmitBegin(uint32 frameIndex) {}
        virtual void OnRenderSubmitEnd(uint32 frameIndex) {}
        virtual void OnPresentBegin(uint32 frameIndex) {}
        virtual void OnPresentEnd(uint32 frameIndex) {}
    };
}