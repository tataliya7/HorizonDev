#include "RenderScene.h"

#include <optick.h>

namespace Horizon
{
    SkyAtmosphereRenderProxy::SkyAtmosphereRenderProxy()
    {

    }

    SkyAtmosphereRenderProxy::~SkyAtmosphereRenderProxy()
    {

    }

    RenderScene::RenderScene()
        : atmosphericLight(nullptr)
        , activeSkyAtmosphere(nullptr)
    {

    }

    RenderScene::~RenderScene()
    {

    }

    void RenderScene::Release()
    {

    }

    void RenderScene::AddMesh(MeshRenderProxy* mesh)
    {
    }

    void RenderScene::RemoveMesh(MeshRenderProxy* mesh)
    {
    }

    void RenderScene::AddLight(LightRenderProxy* light)
    {
    }

    void RenderScene::RemoveLight(LightRenderProxy* light)
    {
    }

    bool RenderScene::HasAtmosphericLight() const
    {
        return atmosphericLight != nullptr;
    }

    DistantLightRenderProxy* RenderScene::GetAtmosphericLight() const
    {
        return atmosphericLight;
    }

    bool RenderScene::HasActiveSkyAtmosphere() const
    {
        return activeSkyAtmosphere != nullptr;
    }

    SkyAtmosphereRenderProxy* RenderScene::GetActiveSkyAtmosphere() const
    {
        return activeSkyAtmosphere;
    }

    void RenderScene::AddSkyAtmosphere(SkyAtmosphereRenderProxy* skyAtmosphere)
    {
        assert(skyAtmosphere != nullptr);
        assert(std::ranges::find(skyAtmospheres, skyAtmosphere) == skyAtmospheres.end());

        skyAtmospheres.push_back(skyAtmosphere);

        activeSkyAtmosphere = skyAtmospheres.back();
    }

    void RenderScene::RemoveSkyAtmosphere(SkyAtmosphereRenderProxy* skyAtmosphere)
    {
        assert(skyAtmosphere != nullptr);
        assert(std::ranges::find(skyAtmospheres, skyAtmosphere) != skyAtmospheres.end());

        skyAtmospheres.erase(std::ranges::remove(skyAtmospheres, skyAtmosphere).begin(), skyAtmospheres.end());

        activeSkyAtmosphere = skyAtmospheres.empty() ? nullptr : skyAtmospheres.back();
    }

    bool RenderScene::HasAnyLocalFogVolume() const
    {
        return !localFogVolumes.empty();
    }

    void RenderScene::AddLocalFogVolume(LocalFogVolumeRenderProxy* localFogVolume)
    {
        assert(localFogVolume != nullptr);
        assert(std::ranges::find(localFogVolumes, localFogVolume) == localFogVolumes.end());

        localFogVolumes.push_back(localFogVolume);
    }

    void RenderScene::RemoveLocalFogVolume(LocalFogVolumeRenderProxy* localFogVolume)
    {
        assert(localFogVolume != nullptr);
        assert(std::ranges::find(localFogVolumes, localFogVolume) != localFogVolumes.end());

        // Avoid the overhead of moving the items as the order does not matter.
        auto iter = std::ranges::find(localFogVolumes, localFogVolume);
        //if (iter != localFogVolumes.end())
        {
            std::swap(*iter, localFogVolumes.back());
            localFogVolumes.pop_back();
        }
    }

    void RenderScene::GetRenderStatistics(RenderStatistics& statistics) const
    {

    }

#if 0
    void RenderScene::UpdateRayTracingAccelerationStructures(SceneView* view, RenderBackendCommandList* commandList)
    {
        return;
        /* commandList->CopyBuffer(
             instanceUploadBuffer,
             0,
             instanceBuffer,
             0,
             instanceBufferDesc.size);*/

        Scene* scene = view->scene;
        EntityManager* entityManager = scene->GetEntityManager();

        entityManager->GetView<MeshComponent>().each([&](EntityHandle entity, MeshComponent& mesh)
            {
                RayTracingGeometry& rayTracingGeometry = mesh.rayTracingGeometry;
                switch (rayTracingGeometry.state)
                {
                case RayTracingGeometryState::BuildRequired:
                    commandList->BuildRayTracingBottomLevelAccelerationStructure(rayTracingGeometry.blas);
                    break;
                case RayTracingGeometryState::UpdateRequired:
                    commandList->UpdateRayTracingBottomLevelAccelerationStructure(rayTracingGeometry.blas, rayTracingGeometry.blas);
                    break;
                default:
                    break;
                }
                rayTracingGeometry.state = RayTracingGeometryState::UpToDate;
            });

        commandList->BuildRayTracingTopLevelAccelerationStructure(rayTracingScene);

        SetShouldUpdateRayTracingScene(false);
    }
#endif
}