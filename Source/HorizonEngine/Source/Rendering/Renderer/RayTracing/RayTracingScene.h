#pragma once

#include "Rendering/Renderer/RendererCommon.h"

namespace Horizon
{
    class RenderScene;

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

    struct RayTracingInstance
    {
        RenderBackendRayTracingInstance instance;
    };

    class RayTracingScene
    {
    public:

        RayTracingScene();
        ~RayTracingScene();

        uint32 AddRayTracingInstance(const RayTracingInstance& instance);

        void RequestBuildRayTracingBLAS(RenderBackendRayTracingAccelerationStructureHandle blas);

        void ClearRayTracingBLASBuildRequests();

        void BuildRayTracingBLASes(RenderGraph& renderGraph);

        void CreateRayTracingTLAS(RenderGraph& renderGraph);

        void BuildRayTracingTLAS(RenderGraph& renderGraph);

        RenderBackendRayTracingAccelerationStructureHandle GetRayTracingTLAS() const
        {
            return rayTracingTLAS;
        }

        // TODO
        RenderBackendRayTracingAccelerationStructureHandle bottomLevelAccelerationStructure;

    //private:

        std::vector<RayTracingInstance> rayTracingInstances;

        std::vector<RayTracingGeometry> rayTracingGeometriesToBuild;

        RenderBackendRayTracingAccelerationStructureHandle rayTracingTLAS;

        //std::vector<RayTracingInstance> instances;
    };

    namespace RayTracing
    {
        void GatherRayTracingInstances(RenderScene* scene);
    }
}