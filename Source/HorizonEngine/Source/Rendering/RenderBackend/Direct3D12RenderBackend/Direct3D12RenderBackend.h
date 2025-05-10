#pragma once

#include "Direct3D12RenderBackendCommon.h"

namespace Horizon
{
    RenderBackend* RenderBackendCreateDirect3D12(const RenderBackendDesc* desc);

    void RenderBackendDestroyDirect3D12(RenderBackend* backend);
}
