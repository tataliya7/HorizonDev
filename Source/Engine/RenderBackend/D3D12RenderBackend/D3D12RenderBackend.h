#pragma once

#include "D3D12RenderBackendCommon.h"

namespace HE
{
    struct D3D12RenderBackendDesc
    {
        bool useDebugLayers;
        bool useGPUBasedValidation;
    };

    RenderBackend* RenderBackendCreateD3D12(const D3D12RenderBackendDesc* desc);

    void RenderBackendDestroyD3D12(RenderBackend* backend);
}
