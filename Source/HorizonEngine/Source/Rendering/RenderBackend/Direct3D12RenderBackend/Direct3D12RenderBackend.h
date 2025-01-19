#pragma once

#include "Direct3D12RenderBackendCommon.h"

namespace Horizon
{
    struct Direct3D12RenderBackendDesc
    {
        bool useDebugLayers;
        bool useGPUBasedValidation;
        bool useHardwareRayTracing;
    };

    RenderBackend* RenderBackendCreateDirect3D12(const Direct3D12RenderBackendDesc* desc);

    void RenderBackendDestroyDirect3D12(RenderBackend* backend);
}
