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

    GPUScene::GPUScene(RenderScene* renderScene)
        : renderScene(renderScene)
    {

    }

    static void SetupGPUSceneGeometryData(GPUSceneGeometryData& geometryData, const MeshRenderObject& mesh)
    {
        auto renderBackend = mesh.renderScene->renderBackend;

        geometryData.vertexBuffer0 = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.vertexBuffers[0]);
        geometryData.vertexBuffer1 = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.vertexBuffers[1]);
        geometryData.vertexBuffer2 = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.vertexBuffers[2]);
        geometryData.vertexBuffer3 = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.vertexBuffers[3]);
        geometryData.vertexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.vertexBuffer);
        geometryData.indexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.indexBuffer);
        geometryData.meshletBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.meshletBuffer);
        geometryData.meshletGroupBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.meshletGroupBuffer);
        geometryData.meshletVertexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.meshletVertexBuffer);
        geometryData.meshletTriangleBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.meshletTriangleBuffer);
        geometryData.materialBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.materialBuffer);
        geometryData.materialIndexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.materialIndexBuffer);
        geometryData.jointIndexBuffer = mesh.jointIndexBuffer ? renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.jointIndexBuffer) : -1;
        geometryData.jointWeightBuffer = mesh.jointWeightBuffer ? renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.jointWeightBuffer) : -1;
        geometryData.jointTransformBuffer = mesh.jointTransformBuffer ? renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.jointTransformBuffer) : -1;
        geometryData.previousJointTransformBuffer = mesh.previousJointTransformBuffer ? renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh.previousJointTransformBuffer) : -1;
        geometryData.vertexCount = mesh.vertexCount;
        geometryData.indexCount = mesh.indexCount;
        //geometryData.boundsMin = mesh.boundsMin;
        //geometryData.boundsMax = mesh.boundsMax;
        geometryData.meshletCount = mesh.meshletCount;
        geometryData.meshletGroupCount = mesh.meshletGroupCount;
        geometryData.relevantJointCountPerVertex = mesh.relevantJointCountPerVertex;
        geometryData.flags = 0;
        if (mesh.UseGPUSkinning())
        {
            geometryData.flags |= GPU_SCENE_GEOMETRY_DATA_FLAG_USE_GPU_SKINNING;
        }
    }

    static void SetupGPUSceneDistantLightData(GPUSceneDistantLightData& distanceLightData, const LightRenderObject& light)
    {
        distanceLightData.direction = light.direction;
        distanceLightData.tangent = light.tangent;
        distanceLightData.color = light.color;
    }

    static void SetupGPUSceneLocalLightData(GPUSceneLocalLightData& localLightData, const LightRenderObject& light)
    {
        localLightData.data0 = Vector4f(light.position, light.radius);
        localLightData.data1 = Vector4f(light.direction, 0.0f);
        localLightData.data2 = Vector4f(light.tangent, 0.0f);
        localLightData.data3 = Vector4f(light.color, 0.0f);
        localLightData.data4 = Vector4f(0.0f);
    }

    void GPUScene::UploadGeometries(RenderGraph& renderGraph)
    {
        std::vector<GPUSceneGeometryData> geometryDataToUpload;
        std::vector<GPUSceneGeometryInstanceData> geometryInstanceDataToUpload;

        // @todo
        geometryCount = static_cast<uint32>(renderScene->meshes.size());
        geometryInstanceCount = static_cast<uint32>(renderScene->meshes.size());
        for (uint32 index = 0; index < geometryCount; index++)
        {
            const uint32 geometryID = index;
            const MeshRenderObject& mesh = *renderScene->meshes[index];

            GPUSceneGeometryData& geometryData = geometryDataToUpload.emplace_back();
            SetupGPUSceneGeometryData(geometryData, mesh);

            GPUSceneGeometryInstanceData& geometryInstanceData = geometryInstanceDataToUpload.emplace_back();
            geometryInstanceData.localToWorldMatrix = mesh.localToWorldMatrix;
            geometryInstanceData.worldToLocalMatrix = mesh.worldToLocalMatrix;
            geometryInstanceData.previousLocalToWorldMatrix = mesh.localToWorldMatrix;
            geometryInstanceData.previousWorldToLocalMatrix = mesh.worldToLocalMatrix;
            geometryInstanceData.geometryID = geometryID;
        }

        uint32 geometryDataBufferElementCount = std::max(1u, geometryCount);
        uint64 geometryDataBufferSize = geometryDataBufferElementCount * sizeof(GPUSceneGeometryData);
        RenderGraphBufferDescription geometryDataBufferDescription = RenderGraphBufferDescription::CreateByteAddress(sizeof(GPUSceneGeometryData) * geometryDataBufferElementCount);
        RenderGraphBufferHandle geometryDataBuffer = renderGraph.CreateBuffer(geometryDataBufferDescription, "GPUSceneGeometryDataBuffer");
        if (geometryCount > 0)
        {
            renderGraph.UploadBufferDeferred(geometryDataBuffer, geometryDataToUpload.data(), geometryDataBufferSize, RenderGraphSourceDataLifetimeHint::OnlyValidNow);
        }
        renderGraph.ExportBufferDeferred(geometryDataBuffer, &persistentGeometryDataBuffer);

        uint32 geometryInstanceDataBufferElementCount = std::max(1u, geometryInstanceCount);
        uint64 geometryInstanceDataBufferSize = geometryInstanceDataBufferElementCount * sizeof(GPUSceneGeometryInstanceData);
        RenderGraphBufferDescription geometryInstanceDataBufferDescription = RenderGraphBufferDescription::CreateByteAddress(sizeof(GPUSceneGeometryInstanceData) * geometryInstanceDataBufferElementCount);
        RenderGraphBufferHandle geometryInstanceDataBuffer = renderGraph.CreateBuffer(geometryInstanceDataBufferDescription, "GPUSceneGeometryInstanceDataBuffer");
        if (geometryInstanceCount > 0)
        {
            renderGraph.UploadBufferDeferred(geometryInstanceDataBuffer, geometryInstanceDataToUpload.data(), geometryInstanceDataBufferSize, RenderGraphSourceDataLifetimeHint::OnlyValidNow);
        }
        renderGraph.ExportBufferDeferred(geometryInstanceDataBuffer, &persistentGeometryInstanceDataBuffer);
    }

    void GPUScene::UploadLights(RenderGraph& renderGraph)
    {
        distantLightCount = 0;
        std::vector<GPUSceneDistantLightData> distantLightDataToUpload;

        localLightCount = 0;
        std::vector<GPUSceneLocalLightData> localLightDataToUpload;

        uint32 lightCount = static_cast<uint32>(renderScene->lights.size());
        for (uint32 i = 0; i < lightCount; i++)
        {
            const LightRenderObject& light = *renderScene->lights[i];

            if (light.IsDistantLight())
            {
                GPUSceneDistantLightData& distantLightData = distantLightDataToUpload.emplace_back();
                SetupGPUSceneDistantLightData(distantLightData, light);
                distantLightCount++;
            }
            else if (light.IsLocalLight())
            {
                GPUSceneLocalLightData& localLightData = localLightDataToUpload.emplace_back();
                SetupGPUSceneLocalLightData(localLightData, light);
                localLightCount++;
            }
        }

        uint32 distantLightDataBufferElementCount = std::max(1u, distantLightCount);
        uint64 distantLightDataBufferSize = distantLightDataBufferElementCount * sizeof(GPUSceneDistantLightData);
        RenderGraphBufferDescription distantLightDataBufferDescription = RenderGraphBufferDescription::CreateStructured(sizeof(GPUSceneDistantLightData), distantLightDataBufferElementCount);
        RenderGraphBufferHandle distantLightDataBuffer = renderGraph.CreateBuffer(distantLightDataBufferDescription, "GPUSceneDistantLightDataBuffer");
        if (distantLightCount > 0)
        {
            renderGraph.UploadBufferDeferred(distantLightDataBuffer, distantLightDataToUpload.data(), distantLightDataBufferSize, RenderGraphSourceDataLifetimeHint::OnlyValidNow);
        }
        renderGraph.ExportBufferDeferred(distantLightDataBuffer, &persistentDistantLightDataBuffer);

        uint32 localLightDataBufferElementCount = std::max(1u, localLightCount);
        uint64 localLightDataBufferSize = localLightDataBufferElementCount * sizeof(GPUSceneLocalLightData);
        RenderGraphBufferDescription localLightDataBufferDescription = RenderGraphBufferDescription::CreateStructured(sizeof(GPUSceneLocalLightData), localLightDataBufferElementCount);
        RenderGraphBufferHandle localLightDataBuffer = renderGraph.CreateBuffer(localLightDataBufferDescription, "GPUSceneLocalLightDataBuffer");
        if (localLightCount > 0)
        {
            renderGraph.UploadBufferDeferred(localLightDataBuffer, localLightDataToUpload.data(), localLightDataBufferSize, RenderGraphSourceDataLifetimeHint::OnlyValidNow);
        }
        renderGraph.ExportBufferDeferred(localLightDataBuffer, &persistentLocalLightDataBuffer);
    }

    RenderScene::RenderScene(RenderBackend* renderBackend, ShaderRepository* shaderRepository)
        : renderBackend(renderBackend)
        , shaderRepository(shaderRepository)
        , atmosphericLight(nullptr)
        , activeSkyAtmosphere(nullptr)
        , activeGlobalFog(nullptr)
    {
        gpuScene = new GPUScene(this);

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

    void SetupDistantLightShaderParameters(GPUSceneDistantLightData& outParameters, const LightRenderObject* light)
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
        gpuScene->UploadGeometries(renderGraph);

        gpuScene->UploadLights(renderGraph);

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