#include "RenderScene.h"

#include <optick.h>

namespace Horizon
{
    LightRenderObject::LightRenderObject(const LightRenderObjectDescription& description)
        : color(description.color)
        , position(description.position)
        , direction(description.direction)
        , castRayTracingShadows(description.castRayTracingShadows)
        , usedAsAtmosphericLight(description.usedAsAtmosphericLight)
        , halfApexAngleInRadians(description.halfApexAngleInRadians)
        , atmosphericLightDiskColorFactor(description.atmosphericLightDiskColorFactor)
    {

    }

    LightRenderObject::~LightRenderObject()
    {

    }

    void LightRenderObject::GetLightShaderParameters(LightShaderParameters& parameters) const
    {

    }

    LocalVolumetricFogRenderObject::LocalVolumetricFogRenderObject()
        : transform(IdentityMatrix4x4)
        , emission(Vector3(0.0f, 0.0f, 0.0f))
    {

    }

    LocalVolumetricFogRenderObject::~LocalVolumetricFogRenderObject()
    {

    }

    SkyAtmosphereRenderObject::SkyAtmosphereRenderObject()
        : atmosphereParameters()
    {
    }

    SkyAtmosphereRenderObject::~SkyAtmosphereRenderObject()
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

    void RenderScene::AddMesh(MeshRenderObject* mesh)
    {
    }

    void RenderScene::RemoveMesh(MeshRenderObject* mesh)
    {
    }

    void RenderScene::AddLight(LightRenderObject* light)
    {
        assert(light != nullptr);
        assert(std::ranges::find(lights, light) == lights.end());

        if (light->IsUsedAsAtmosphericLight())
        {
            assert(atmosphericLight == nullptr);
            atmosphericLight = light;
        }

        lights.push_back(light);
    }

    void RenderScene::RemoveLight(LightRenderObject* light)
    {
    }

    bool RenderScene::HasAtmosphericLight() const
    {
        return atmosphericLight != nullptr;
    }

    LightRenderObject* RenderScene::GetAtmosphericLight() const
    {
        return atmosphericLight;
    }

    bool RenderScene::HasActiveSkyAtmosphere() const
    {
        return activeSkyAtmosphere != nullptr;
    }

    SkyAtmosphereRenderObject* RenderScene::GetActiveSkyAtmosphere() const
    {
        return activeSkyAtmosphere;
    }

    void RenderScene::AddSkyAtmosphere(SkyAtmosphereRenderObject* skyAtmosphere)
    {
        assert(skyAtmosphere != nullptr);
        assert(std::ranges::find(skyAtmospheres, skyAtmosphere) == skyAtmospheres.end());

        skyAtmospheres.push_back(skyAtmosphere);

        activeSkyAtmosphere = skyAtmospheres.back();
    }

    void RenderScene::RemoveSkyAtmosphere(SkyAtmosphereRenderObject* skyAtmosphere)
    {
        assert(skyAtmosphere != nullptr);
        assert(std::ranges::find(skyAtmospheres, skyAtmosphere) != skyAtmospheres.end());

        skyAtmospheres.erase(std::ranges::remove(skyAtmospheres, skyAtmosphere).begin(), skyAtmospheres.end());

        activeSkyAtmosphere = skyAtmospheres.empty() ? nullptr : skyAtmospheres.back();
    }

    bool RenderScene::HasAnyLocalVolumetricFog() const
    {
        return !localVolumetricFogs.empty();
    }

    void RenderScene::AddLocalVolumetricFog(LocalVolumetricFogRenderObject* localVolumetricFog)
    {
        assert(localVolumetricFog != nullptr);
        assert(std::ranges::find(localVolumetricFogs, localVolumetricFog) == localVolumetricFogs.end());

        localVolumetricFogs.push_back(localVolumetricFog);
    }

    void RenderScene::RemoveLocalVolumetricFog(LocalVolumetricFogRenderObject* localVolumetricFog)
    {
        assert(localVolumetricFog != nullptr);
        assert(std::ranges::find(localVolumetricFogs, localVolumetricFog) != localVolumetricFogs.end());

        // Avoid the overhead of moving the items as the order does not matter.
        auto iter = std::ranges::find(localVolumetricFogs, localVolumetricFog);
        //if (iter != localFogVolumes.end())
        {
            std::swap(*iter, localVolumetricFogs.back());
            localVolumetricFogs.pop_back();
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