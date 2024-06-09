#pragma once

#include "Foundation/FoundationModule.h"

#include <ffx_fsr2.h>

namespace Horizon
{
    struct FidelityFxSuperResolution2State
    {
        FfxFsr2ContextDescription fsr2ContextDescription;
        FfxFsr2Context fsr2Context;
        bool initialized;
        //uint32 viewportID;
    };

    void FidelityFXSuperResolution2Message(FfxFsr2MsgType type, const wchar_t* message);

    bool FidelityFXSuperResolution2DispatchD3D12(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure);

    bool FidelityFXSuperResolution2DispatchVulkan(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure);
}