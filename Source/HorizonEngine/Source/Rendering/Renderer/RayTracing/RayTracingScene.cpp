#include "RayTracingScene.h"

#include "Rendering/Renderer/RenderScene.h"

namespace Horizon
{
    RayTracingScene::RayTracingScene()
    {

    }

    RayTracingScene::~RayTracingScene()
    {

    }

    uint32 RayTracingScene::AddRayTracingInstance(const RayTracingInstance& instance)
    {
        uint32 rayTracingInstanceHandle = static_cast<uint32>(rayTracingInstances.size());
        rayTracingInstances.push_back(instance);
        return rayTracingInstanceHandle;
    }

    void RayTracingScene::CreateRayTracingTLAS(RenderGraph& renderGraph)
    {
        static int first = 0;
        if (first == 0 && !rayTracingInstances.empty())
        {
            RenderBackendRayTracingAccelerationStructureBuildFlags rayTracingSceneTLASBuildFlags = RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastTrace;

            uint32 instanceCount = static_cast<uint32>(rayTracingInstances.size());
            std::vector<RenderBackendRayTracingInstance> instances(instanceCount);
            for (uint32 i = 0; i < instanceCount; i++)
            {
                instances[i] = rayTracingInstances[i].instance;
            }

            RenderBackendRayTracingTopLevelAccelerationStructureDescription rayTracingSceneTLASDescription =
            {
                .buildFlags = rayTracingSceneTLASBuildFlags,
                .geometryFlags = RenderBackendRayTracingGeometryFlags::Opaque,
                .instanceCount = instanceCount,
                .instances = instances.data(),
            };
            rayTracingTLAS = renderGraph.GetRenderBackend()->CreateRayTracingTopLevelAccelerationStructure(&rayTracingSceneTLASDescription, "RayTracingSceneTLAS");

            first = 1;
        }
    }

    void RayTracingScene::BuildRayTracingTLAS(RenderGraph& renderGraph)
    {
        RenderBackendRayTracingAccelerationStructureHandle tlas = GetRayTracingTLAS();

        if (tlas)
        {
            renderGraph.AddPass(
                std::format("BuildRayTracingScene"),
                RenderGraphPassFlags::Compute | RenderGraphPassFlags::NoCulling,
                [&](RenderGraphBuilder& builder)
                {
                    // todo

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                        {
                            commandList.BuildRayTracingTopLevelAccelerationStructure(tlas);
                        };
                });
        }
    }

    void RayTracingScene::RequestBuildRayTracingBLAS(RenderBackendRayTracingAccelerationStructureHandle blas)
    {
        RayTracingGeometry rayTracingGeometry =
        {
            .state = RayTracingGeometryState::BuildRequired,
            .blas = blas
        };
        rayTracingGeometriesToBuild.emplace_back(rayTracingGeometry);
    }

    void RayTracingScene::BuildRayTracingBLASes(RenderGraph& renderGraph)
    {
        for (const RayTracingGeometry& geometry : rayTracingGeometriesToBuild)
        {
            if (geometry.blas)
            {
                renderGraph.AddPass(
                std::format("BuildBLASes"),
                RenderGraphPassFlags::Compute | RenderGraphPassFlags::NoCulling,
                [&](RenderGraphBuilder& builder)
                {
                    // todo

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        commandList.BuildRayTracingBottomLevelAccelerationStructure(geometry.blas);
                    };
                });
            }
        }

        ClearRayTracingBLASBuildRequests();
    }

    void RayTracingScene::ClearRayTracingBLASBuildRequests()
    {
        rayTracingGeometriesToBuild.clear();
    }

    namespace RayTracing
    {
        void GatherRayTracingInstances(RenderScene* scene)
        {
            RayTracingScene* rayTracingScene = scene->GetRayTracingScene();

            for (MeshRenderObject* mesh : scene->meshes)
            {
                if (mesh->bottomLevelAccelerationStructure)
                {
                    RenderBackendRayTracingAccelerationStructureBuildFlags rayTracingSceneTLASBuildFlags = true ?
                    RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastTrace:
                    RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastBuild;

                    RayTracingInstance rayTracingInstance = {};
                    rayTracingInstance.instance.transformMatrix = mesh->localToWorldMatrix;
                    rayTracingInstance.instance.instanceID = 0;
                    rayTracingInstance.instance.instanceMask = 0xff;
                    rayTracingInstance.instance.instanceContributionToHitGroupIndex = 0;
                    rayTracingInstance.instance.flags = RenderBackendRayTracingInstanceFlags::TriangleFacingCullDisable;
                    rayTracingInstance.instance.blas = mesh->bottomLevelAccelerationStructure;

                    rayTracingScene->AddRayTracingInstance(rayTracingInstance);

                    // if ()
                    // {
                    //     rayTracingGeometriesToBuild
                    // }
                }
            }
        }
    }
}