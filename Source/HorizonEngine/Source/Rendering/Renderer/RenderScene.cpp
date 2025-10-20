#include "RenderScene.h"
#include "ImageBasedLighting.h"
#include "ShaderRepository.h"
#include "Engine/Core/RenderSystem.h"

namespace Horizon
{
    LightRenderObject::LightRenderObject(const LightRenderObjectDescription& description)
        : lightType(description.lightType)
        , color(description.color)
        , position(description.position)
        , direction(description.direction)
        , radius(description.radius)
        , castDynamicShadows(description.castDynamicShadows)
        , shadowMapSize(description.shadowMapSize)
        , shadowCascadeCount(description.shadowCascadeCount)
        , shadowCascadeSplitLambda(description.shadowCascadeSplitLambda)
        , shadowCascadeBlendScale(description.shadowCascadeTransitionScale)
        , maxShadowDistance(description.maxShadowDistance)
        , shadowFadeOutFactor(description.shadowFadeOutFactor)
        , shadowMapDepthBiasConstantFactor(description.shadowMapDepthBiasConstantFactor)
        , shadowMapDepthBiasSlopeFactor(description.shadowMapDepthBiasSlopeFactor)
        , enableScreenSpaceShadows(description.enableScreenSpaceShadows)
        , screenSpaceShadowsSurfaceThickness(description.screenSpaceShadowsSurfaceThickness)
        , screenSpaceShadowsShadowContrast(description.screenSpaceShadowsShadowContrast)
        , usedAsAtmosphericLight(description.usedAsAtmosphericLight)
        , halfApexAngleInRadians(description.halfApexAngleInRadians)
        , atmosphericLightDiskColorFactor(description.atmosphericLightDiskColorFactor)
        , enableLightShafts(description.enableLightShafts)
        , lightShaftsIntensity(description.lightShaftsIntensity)
        , lightShaftsColor(description.lightShaftsColor)
    {

    }

    LightRenderObject::~LightRenderObject()
    {

    }

    SkyLightRenderObject::SkyLightRenderObject()
    {
    }

    SkyLightRenderObject::~SkyLightRenderObject()
    {
    }

    LocalFogVolumeRenderObject::LocalFogVolumeRenderObject()
        : transform(IdentityMatrix4x4f)
        , emission(Vector3f(0.0f, 0.0f, 0.0f))
    {

    }

    LocalFogVolumeRenderObject::~LocalFogVolumeRenderObject()
    {

    }

    SkyAtmosphereRenderObject::SkyAtmosphereRenderObject()
        : atmosphereParameters()
    {
    }

    SkyAtmosphereRenderObject::~SkyAtmosphereRenderObject()
    {

    }

    GlobalFogRenderObject::GlobalFogRenderObject()
    {
    }

    GlobalFogRenderObject::~GlobalFogRenderObject()
    {
    }

    RenderScene::RenderScene(RenderBackend* renderBackend, ShaderRepository* shaderRepository)
        : renderBackend(renderBackend)
        , shaderRepository(shaderRepository)
        , atmosphericLight(nullptr)
        , activeSkyAtmosphere(nullptr)
        , activeGlobalFog(nullptr)
    {
        gpuScene = new GPUScene();

        rayTracingScene = nullptr;
    }

    RenderScene::~RenderScene()
    {

    }

    void RenderScene::Release()
    {

    }

    void RenderScene::AddMesh(MeshRenderObject* mesh)
    {
        assert(mesh != nullptr);
        assert(std::ranges::find(meshes, mesh) == meshes.end());

        meshes.push_back(mesh);
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

    SkyLightRenderObject* RenderScene::GetActiveSkyLight() const
    {
        if (skyLights.empty())
        {
            return nullptr;
        }

        return skyLights[0];
    }

    void RenderScene::AddSkyLight(SkyLightRenderObject* skyLight)
    {
        assert(skyLight != nullptr);
        assert(std::ranges::find(skyLights, skyLight) == skyLights.end());

        skyLights.push_back(skyLight);
    }

    void RenderScene::RemoveSkyLight(SkyLightRenderObject* skyLight)
    {

    }

    bool RenderScene::HasSkyLight() const
    {
        return !skyLights.empty();
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

    bool RenderScene::HasActiveGlobalFog() const
    {
        return activeGlobalFog != nullptr;
    }

    GlobalFogRenderObject* RenderScene::GetActiveGlobalFog() const
    {
        return activeGlobalFog;
    }

    void RenderScene::AddGlobalFog(GlobalFogRenderObject* globalFog)
    {
        assert(globalFog != nullptr);
        assert(std::ranges::find(globalFogs, globalFog) == globalFogs.end());

        globalFogs.push_back(globalFog);

        activeGlobalFog = globalFogs.back();
    }

    void RenderScene::RemoveGlobalFog(GlobalFogRenderObject* globalFog)
    {
        assert(globalFog != nullptr);
        assert(std::ranges::find(globalFogs, globalFog) != globalFogs.end());

        globalFogs.erase(std::ranges::remove(globalFogs, globalFog).begin(), globalFogs.end());

        activeGlobalFog = globalFogs.empty() ? nullptr : globalFogs.back();
    }

    bool RenderScene::HasAnyLocalFogVolume() const
    {
        return !localFogVolumes.empty();
    }

    void RenderScene::AddLocalFogVolume(LocalFogVolumeRenderObject* localFogVolume)
    {
        assert(localFogVolume != nullptr);
        assert(std::ranges::find(localFogVolumes, localFogVolume) == localFogVolumes.end());

        localFogVolumes.push_back(localFogVolume);
    }

    void RenderScene::RemoveLocalFogVolume(LocalFogVolumeRenderObject* localFogVolume)
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

    void SetupDistantLightShaderParameters(DistantLightShaderParameters& outParameters, const LightRenderObject* light)
    {
        if (light)
        {
            outParameters.direction = light->direction;
            outParameters.tangent = light->tangent;
            outParameters.color = light->color;
        }
    }

    RayTracingScene* RenderScene::CreateRayTracingScene()
    {
        rayTracingScene = new RayTracingScene();
        return rayTracingScene;
    }

    void RenderScene::UpdateGPUScene(RenderGraph& renderGraph, RenderBackendCommandList* commandList)
    {
        gpuScene->geometryData.clear();
        gpuScene->geometryInstanceData.clear();

        uint32 geometryCount = uint32(meshes.size());
        for (uint32 index  = 0; index < meshes.size(); index++)
        {
            uint32 geometryID = index;
            MeshRenderObject* mesh = meshes[index];

            GPUSceneGeometryData geometry;
            geometry.vertexBuffer0 = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->vertexBuffers[0]);
            geometry.vertexBuffer1 = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->vertexBuffers[1]);
            geometry.vertexBuffer2 = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->vertexBuffers[2]);
            geometry.vertexBuffer3 = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->vertexBuffers[3]);
            geometry.vertexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->vertexBuffer);
            geometry.previousVertexBuffer0 = -1;
            geometry.indexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->indexBuffer);
            geometry.meshletBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->meshletBuffer);
            geometry.meshletGroupBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->meshletGroupBuffer);
            geometry.meshletVertexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->meshletVertexBuffer);
            geometry.meshletTriangleBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->meshletTriangleBuffer);
            geometry.materialBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->materialBuffer);
            geometry.materialIndexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->materialIndexBuffer);
            geometry.jointIndexBuffer = mesh->jointIndexBuffer ? renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->jointIndexBuffer) : -1;
            geometry.jointWeightBuffer = mesh->jointWeightBuffer ? renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->jointWeightBuffer) : -1;
            geometry.jointTransformBuffer = mesh->jointTransformBuffer ? renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->jointTransformBuffer) : -1;
            geometry.previousJointTransformBuffer = mesh->previousJointTransformBuffer ? renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->previousJointTransformBuffer) : -1;
            geometry.vertexCount = mesh->vertexCount;
            geometry.indexCount = mesh->indexCount;
            //geometry.boundsMin = mesh.boundsMin;
            //geometry.boundsMax = mesh.boundsMax;
            geometry.meshletCount = mesh->meshletCount;
            geometry.meshletGroupCount = mesh->meshletGroupCount;
            geometry.relevantJointCountPerVertex = mesh->relevantJointCountPerVertex;
            geometry.flags = 0;
            if (mesh->UseGPUSkinning())
            {
                geometry.flags |= GPU_SCENE_GEOMETRY_DATA_FLAG_USE_GPU_SKINNING;
            }
            gpuScene->geometryData.emplace_back(geometry);

            // TODO
            GPUSceneGeometryInstanceData geometryInstance;
            geometryInstance.localToWorldMatrix = mesh->localToWorldMatrix;
            geometryInstance.worldToLocalMatrix = mesh->worldToLocalMatrix;
            geometryInstance.previousLocalToWorldMatrix = geometryInstance.localToWorldMatrix;
            geometryInstance.previousWorldToLocalMatrix = geometryInstance.worldToLocalMatrix;
            geometryInstance.geometryID = geometryID;
            gpuScene->geometryInstanceData.emplace_back(geometryInstance);
        }

        uint32 newGeometryDataBufferSize = geometryCount * sizeof(GPUSceneGeometryData);
        if (gpuScene->geometryDataBuffer == RenderBackendBufferHandle::Null && newGeometryDataBufferSize > 0)
        {
            RenderBackendBufferDescription geometryUploadBufferDesc = RenderBackendBufferDescription::CreateUpload(newGeometryDataBufferSize);
            gpuScene->geometryDataUploadBuffer = renderBackend->CreateBuffer(&geometryUploadBufferDesc, nullptr, "GeometryDataUploadBuffer");
            RenderBackendBufferDescription geometryBufferDesc = RenderBackendBufferDescription::CreateByteAddress(newGeometryDataBufferSize);
            gpuScene->geometryDataBuffer = renderBackend->CreateBuffer(&geometryBufferDesc, nullptr, "GeometryDataBuffer");
            gpuScene->geometryDataBufferSize = newGeometryDataBufferSize;
        }
        else if (gpuScene->geometryDataBufferSize < newGeometryDataBufferSize)
        {
            renderBackend->ResizeBuffer(gpuScene->geometryDataUploadBuffer, newGeometryDataBufferSize);
            renderBackend->ResizeBuffer(gpuScene->geometryDataBuffer, newGeometryDataBufferSize);
            gpuScene->geometryDataBufferSize = newGeometryDataBufferSize;
        }

        if (gpuScene->geometryDataBuffer && geometryCount > 0)
        {
            renderBackend->UpdateBuffer(gpuScene->geometryDataUploadBuffer, 0, gpuScene->geometryData.data(), gpuScene->geometryDataBufferSize);

            // {
            //     RenderBackendBarrier barrier[] =
            //     {
            //         RenderBackendBarrier(gpuScene->geometryDataBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
            //     };
            //     commandList->Barriers(barrier, 1);
            // }
            commandList->CopyBuffer(
                gpuScene->geometryDataUploadBuffer,
                0,
                gpuScene->geometryDataBuffer,
                0,
                gpuScene->geometryDataBufferSize);
            // {
            //     RenderBackendBarrier barrier[] =
            //     {
            //         RenderBackendBarrier(gpuScene->geometryDataBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource)
            //     };
            //     commandList->Barriers(barrier, 1);
            // }
        }

        uint32 geometryInstanceCount = geometryCount;
        uint32 newGeometryInstanceDataBufferSize = geometryInstanceCount * sizeof(GPUSceneGeometryInstanceData);
        if (gpuScene->geometryInstanceDataBuffer == RenderBackendBufferHandle::Null && newGeometryInstanceDataBufferSize > 0)
        {
            RenderBackendBufferDescription geometryInstanceUploadBufferDesc = RenderBackendBufferDescription::CreateUpload(newGeometryInstanceDataBufferSize);
            gpuScene->geometryInstanceDataUploadBuffer = renderBackend->CreateBuffer(&geometryInstanceUploadBufferDesc, nullptr, "GeometryInstanceDataUploadBuffer");
            RenderBackendBufferDescription geometryInstanceBufferDesc = RenderBackendBufferDescription::CreateByteAddress(newGeometryInstanceDataBufferSize);
            gpuScene->geometryInstanceDataBuffer = renderBackend->CreateBuffer(&geometryInstanceBufferDesc, nullptr, "GeometryInstanceDataBuffer");
            gpuScene->geometryInstanceDataBufferSize = newGeometryInstanceDataBufferSize;
        }
        else if (gpuScene->geometryInstanceDataBufferSize < newGeometryInstanceDataBufferSize)
        {
            renderBackend->ResizeBuffer(gpuScene->geometryInstanceDataUploadBuffer, newGeometryInstanceDataBufferSize);
            renderBackend->ResizeBuffer(gpuScene->geometryInstanceDataBuffer, newGeometryInstanceDataBufferSize);
            gpuScene->geometryInstanceDataBufferSize = newGeometryInstanceDataBufferSize;
        }

        gpuScene->geometryInstanceCount = geometryInstanceCount;

        if (gpuScene->geometryInstanceDataBuffer && geometryInstanceCount > 0)
        {
            renderBackend->UpdateBuffer(gpuScene->geometryInstanceDataUploadBuffer, 0, gpuScene->geometryInstanceData.data(), gpuScene->geometryInstanceDataBufferSize);
            commandList->CopyBuffer(
                gpuScene->geometryInstanceDataUploadBuffer,
                0,
                gpuScene->geometryInstanceDataBuffer,
                0,
                gpuScene->geometryInstanceDataBufferSize);
            // RenderBackendBarrier barrier[] =
            // {
            //     RenderBackendBarrier(gpuScene->geometryInstanceDataBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource)
            // };
            // commandList->Barriers(barrier, 1);
        }

        DistantLightShaderParameters distantLightShaderParameters;
        SetupDistantLightShaderParameters(distantLightShaderParameters, atmosphericLight);

        if (!distantLightDataBuffer)
        {
            RenderBackendBufferDescription distantLightDataUploadBufferDesc = RenderBackendBufferDescription::CreateUpload(sizeof(DistantLightShaderParameters));
            distantLightDataUploadBuffer = renderBackend->CreateBuffer(&distantLightDataUploadBufferDesc, nullptr, "DistantLightDataUploadBuffer");
            RenderBackendBufferDescription distantLightDataBufferDesc = RenderBackendBufferDescription::CreateStructured(sizeof(DistantLightShaderParameters), 1);
            distantLightDataBuffer = renderBackend->CreateBuffer(&distantLightDataBufferDesc, nullptr, "DistantLightDataBuffer");
        }
        {
            renderBackend->UpdateBuffer(distantLightDataUploadBuffer, 0, &distantLightShaderParameters, sizeof(DistantLightShaderParameters));
            commandList->CopyBuffer(
                distantLightDataUploadBuffer,
                0,
                distantLightDataBuffer,
                0,
                sizeof(DistantLightShaderParameters));
            // RenderBackendBarrier barrier[] =
            // {
            //     RenderBackendBarrier(distantLightDataBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource)
            // };
            // commandList->Barriers(barrier, 1);
        }

        static int first = 0;
        if (first == 0)
        {
            first = 1;

            SkyLightRenderObject* skyLight = skyLights[0];
            if (skyLight)
            {
                uint32 cubemapSize = skyLight->cubemapSize;

                RenderBackendTextureDesc convolvedEnvironmentMapTextureDesc = RenderBackendTextureDesc::CreateCube(
                    cubemapSize,
                    RenderBackendTextureFormat::R16G16B16A16Float,
                    RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource,
                    Math::MaxMipLevelCount(cubemapSize));
                convolvedEnvironmentMapTexture = renderBackend->CreateTexture(&convolvedEnvironmentMapTextureDesc, nullptr, "ConvolvedEnvironmentMapTexture");

                RenderBackendTextureDesc irradianceEnvironmentMapTextureDesc = RenderBackendTextureDesc::CreateCube(
                    GIrradianceEnvironmentMapSize,
                    RenderBackendTextureFormat::R16G16B16A16Float,
                    RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
                irradianceEnvironmentMapTexture = renderBackend->CreateTexture(&irradianceEnvironmentMapTextureDesc, nullptr, "IrradianceEnvironmentMapTexture");

                RenderBackendBufferDescription irradianceEnvironmentMapBufferDesc = RenderBackendBufferDescription::CreateByteAddress(sizeof(float) * 27);
                irradianceEnvironmentMapBuffer = renderBackend->CreateBuffer(&irradianceEnvironmentMapBufferDesc, nullptr, "IrradianceEnvironmentMapBuffer");
                irradianceEnvironmentMapBufferFast = renderBackend->CreateBuffer(&irradianceEnvironmentMapBufferDesc, nullptr, "IrradianceEnvironmentMapBufferFast");

                PrecomputeEnvironmentMaps(
                    renderBackend,
                    shaderRepository,
                    *commandList,
                    skyLight->cubemapSize,
                    skyLight->environmentMapTexture->GetHandle(),
                    convolvedEnvironmentMapTexture,
                    irradianceEnvironmentMapTexture,
                    irradianceEnvironmentMapBuffer,
                    irradianceEnvironmentMapBufferFast);
            }
        }

        if (ShouldUpdateRayTracingScene())
        {
            RayTracing::GatherRayTracingInstances(this);

            rayTracingScene->BuildRayTracingBLASes(renderGraph);

            rayTracingScene->CreateRayTracingTLAS(renderGraph);

            // Rebuilding the TLAS every frame.
            rayTracingScene->BuildRayTracingTLAS(renderGraph);
        }
    }
}