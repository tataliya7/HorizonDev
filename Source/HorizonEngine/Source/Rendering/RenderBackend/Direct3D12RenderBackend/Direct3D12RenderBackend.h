#pragma once

#include "Direct3D12RenderBackendCommon.h"

namespace Horizon
{
    struct D3D12RenderBackendDesc
    {
        bool useDebugLayers;
        bool useGPUBasedValidation;
        bool useHardwareRayTracing;
    };

    RenderBackend* RenderBackendCreateD3D12(const D3D12RenderBackendDesc* desc);

    void RenderBackendDestroyD3D12(RenderBackend* backend);
}
