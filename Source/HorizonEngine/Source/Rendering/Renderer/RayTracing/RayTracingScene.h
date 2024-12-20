#pragma once

#include "Rendering/Renderer/RendererCommon.h"

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

        RayTracingScene();
        ~RayTracingScene();

        RenderBackendRayTracingAccelerationStructureHandle GetTLAS()
        {
            return topLevelAccelerationStructure;
        }
        std::vector<RayTracingGeometry*> geometriesToBuild;

        // TODO
        RenderBackendRayTracingAccelerationStructureHandle bottomLevelAccelerationStructure;

    //private:

        RenderBackendRayTracingAccelerationStructureHandle topLevelAccelerationStructure;

        RenderBackendBufferHandle instanceUploadBuffer;
        RenderBackendBufferHandle transformUploadBuffer;

        uint32 transformMatrixCount = 0;
        uint64 transformBufferSize = 0;
        std::vector<Matrix4x4> rowMajorTransforms;
        RenderBackendBufferHandle transformBufferRowMajor;
        RenderBackendBufferHandle transformBufferRowMajorUpload;
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