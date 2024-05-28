#include "RenderScene.h"

#include <optick.h>

namespace Horizon
{
    void RenderScene::AddLocalFogVolume(LocalFogVolumeRenderProxy* localFogVolume)
    {
        assert(localFogVolume);

        localFogVolumes.push_back(localFogVolume);
    }

    void RenderScene::RemoveLocalFogVolume(LocalFogVolumeRenderProxy* localFogVolume)
    {
        assert(localFogVolume);

        // Avoid the overhead of moving the items as the order does not matter.
        auto iter = std::ranges::find(localFogVolumes, localFogVolume);
        if (iter != localFogVolumes.end())
        {
            std::swap(*iter, localFogVolumes.back());
            localFogVolumes.pop_back();
        }
    }

    bool RenderScene::HasAnyLocalFogVolume() const
    {
        return !localFogVolumes.empty();
    }

    void RenderScene::GetRenderStatistics(RenderStatistics& statistics) const
    {

    }

    void RenderScene::UpdateGeometry()
    {
        OPTICK_EVENT();

        for (auto& entity : entities)
        {
            if (entity->IsVisible())
            {
                if (entity->IsDirty())
                {
                    entity->UpdateGeometry();
                }
            }
        }
    }

    void RenderScene::SetSkyAtmosphere()
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