#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    enum class RayTracingGeometryState
    {
        Invalid = 0,
        BuildRequired = 1,
        UpdateRequired = 2,
        UpToDate = 3,
    };

    struct RayTracingGeometry
    {
        RayTracingGeometryState state;
        RenderBackendRayTracingAccelerationStructureHandle blas;

        bool IsUpToDate() const
        {
            return state == RayTracingGeometryState::UpToDate;
        }
    };

    class RayTracingScene
    {
    public:
        RenderBackendRayTracingAccelerationStructureHandle GetTLAS()
        {
            return tlas;
        }
        std::vector<RayTracingGeometry*> geometriesToBuild;
    private:
        RenderBackendRayTracingAccelerationStructureHandle tlas;

        RenderBackendBufferHandle instanceUploadBuffer;
        RenderBackendBufferHandle transformUploadBuffer;

        //std::vector<RayTracingInstance> instances;
    };

    // bool GatherRayTracingInstances(
    //     RenderGraph& renderGraph,
    //     SceneView& view,
    //     RayTracingScene& rayTracingScene)
    // {
    //     return false;
    // }
}