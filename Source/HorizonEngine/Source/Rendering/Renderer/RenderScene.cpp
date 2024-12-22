#include "RenderScene.h"

#include <optick.h>


#include "ImageBasedLighting.h"
#include "ShaderLibrary.h"
#include "Engine/Classes/RenderSystem.h"

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
        , shadowCascadeTransitionScale(description.shadowCascadeTransitionScale)
        , maxShadowDistance(description.maxShadowDistance)
        , shadowFadeOutFactor(description.shadowFadeOutFactor)
        , shadowMapDepthBiasConstantFactor(description.shadowMapDepthBiasConstantFactor)
        , shadowMapDepthBiasSlopeFactor(description.shadowMapDepthBiasSlopeFactor)
        , usedAsAtmosphericLight(description.usedAsAtmosphericLight)
        , halfApexAngleInRadians(description.halfApexAngleInRadians)
        , atmosphericLightDiskColorFactor(description.atmosphericLightDiskColorFactor)
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
        : transform(IdentityMatrix4x4)
        , emission(Vector3(0.0f, 0.0f, 0.0f))
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

    RenderScene::RenderScene(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary)
        : renderBackend(renderBackend)
        , shaderLibrary(shaderLibrary)
        , atmosphericLight(nullptr)
        , activeSkyAtmosphere(nullptr)
    {
        gpuScene = new GPUScene();
        rayTracingScene = new RayTracingScene();
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

    void RenderScene::AddSkyLight(SkyLightRenderObject* skyLight)
    {
        assert(skyLight != nullptr);
        assert(std::ranges::find(skyLights, skyLight) == skyLights.end());

        skyLights.push_back(skyLight);
    }

    void RenderScene::RemoveSkyLight(SkyLightRenderObject* skyLight)
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

    void SetupDistantLightShaderParameters(DistantLightRenderData& outParameters, const LightRenderObject* light)
    {
        outParameters.direction = light->direction;
        outParameters.tangent = light->tangent;
        outParameters.color = light->color;
    }

    void RenderScene::UpdateGPUScene(RenderBackendCommandList* commandList)
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
            geometry.previousVertexBuffer0 = -1;
            geometry.indexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->indexBuffer);
            geometry.materialBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->materialBuffer);
            geometry.materialIndexBuffer = renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(mesh->materialIndexBuffer);
            geometry.vertexCount = mesh->vertexCount;
            geometry.indexCount = mesh->indexCount;
            //geometry.boundsMin = mesh.boundsMin;
            //geometry.boundsMax = mesh.boundsMax;
            gpuScene->geometryData.emplace_back(geometry);

            // TODO
            GPUSceneGeometryInstanceData geometryInstance;
            geometryInstance.localToWorldMatrix = glm::scale(mesh->localToWorldMatrix, Vector3(0.01f, 0.01f, 0.01f));
            geometryInstance.worldToLocalMatrix = mesh->worldToLocalMatrix;
            geometryInstance.previousLocalToWorldMatrix = geometryInstance.localToWorldMatrix;
            geometryInstance.previousWorldToLocalMatrix = geometryInstance.worldToLocalMatrix;
            geometryInstance.geometryID = geometryID;
            gpuScene->geometryInstanceData.emplace_back(geometryInstance);
        }

        uint32 newGeometryDataBufferSize = geometryCount * sizeof(GPUSceneGeometryData);
        if (gpuScene->geometryDataBuffer == RenderBackendBufferHandle::Null && newGeometryDataBufferSize > 0)
        {
            RenderBackendBufferDesc geometryUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(newGeometryDataBufferSize);
            gpuScene->geometryDataUploadBuffer = renderBackend->CreateBuffer(&geometryUploadBufferDesc, nullptr, "GeometryDataUploadBuffer");
            RenderBackendBufferDesc geometryBufferDesc = RenderBackendBufferDesc::CreateByteAddress(newGeometryDataBufferSize);
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
            //     commandList->Transitions(barrier, 1);
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
            //     commandList->Transitions(barrier, 1);
            // }
        }

        uint32 geometryInstanceCount = geometryCount;
        uint32 newGeometryInstanceDataBufferSize = geometryInstanceCount * sizeof(GPUSceneGeometryInstanceData);
        if (gpuScene->geometryInstanceDataBuffer == RenderBackendBufferHandle::Null && newGeometryInstanceDataBufferSize > 0)
        {
            RenderBackendBufferDesc geometryInstanceUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(newGeometryInstanceDataBufferSize);
            gpuScene->geometryInstanceDataUploadBuffer = renderBackend->CreateBuffer(&geometryInstanceUploadBufferDesc, nullptr, "GeometryInstanceDataUploadBuffer");
            RenderBackendBufferDesc geometryInstanceBufferDesc = RenderBackendBufferDesc::CreateByteAddress(newGeometryInstanceDataBufferSize);
            gpuScene->geometryInstanceDataBuffer = renderBackend->CreateBuffer(&geometryInstanceBufferDesc, nullptr, "GeometryInstanceDataBuffer");
            gpuScene->geometryInstanceDataBufferSize = newGeometryInstanceDataBufferSize;
        }
        else if (gpuScene->geometryInstanceDataBufferSize < newGeometryInstanceDataBufferSize)
        {
            renderBackend->ResizeBuffer(gpuScene->geometryInstanceDataUploadBuffer, newGeometryInstanceDataBufferSize);
            renderBackend->ResizeBuffer(gpuScene->geometryInstanceDataBuffer, newGeometryInstanceDataBufferSize);
            gpuScene->geometryInstanceDataBufferSize = newGeometryInstanceDataBufferSize;
        }

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
            // commandList->Transitions(barrier, 1);
        }

        DistantLightRenderData distantLightShaderParameters;
        SetupDistantLightShaderParameters(distantLightShaderParameters, atmosphericLight);

        if (!distantLightDataBuffer)
        {
            RenderBackendBufferDesc distantLightDataUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(DistantLightRenderData));
            distantLightDataUploadBuffer = renderBackend->CreateBuffer(&distantLightDataUploadBufferDesc, nullptr, "DistantLightDataUploadBuffer");
            RenderBackendBufferDesc distantLightDataBufferDesc = RenderBackendBufferDesc::CreateStructured(sizeof(DistantLightRenderData), 1);
            distantLightDataBuffer = renderBackend->CreateBuffer(&distantLightDataBufferDesc, nullptr, "DistantLightDataBuffer");
        }
        {
            renderBackend->UpdateBuffer(distantLightDataUploadBuffer, 0, &distantLightShaderParameters, sizeof(DistantLightRenderData));
            commandList->CopyBuffer(
                distantLightDataUploadBuffer,
                0,
                distantLightDataBuffer,
                0,
                sizeof(DistantLightRenderData));
            // RenderBackendBarrier barrier[] =
            // {
            //     RenderBackendBarrier(distantLightDataBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource)
            // };
            // commandList->Transitions(barrier, 1);
        }

        static int first = 0;
        SkyLightRenderObject* skyLight = skyLights[0];
        if (first == 0)
        {
            first = 1;

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

            RenderBackendBufferDesc irradianceEnvironmentMapBufferDesc = RenderBackendBufferDesc::CreateByteAddress(sizeof(float) * 27);
            irradianceEnvironmentMapBuffer = renderBackend->CreateBuffer(&irradianceEnvironmentMapBufferDesc, nullptr, "IrradianceEnvironmentMapBuffer");
            irradianceEnvironmentMapBufferFast = renderBackend->CreateBuffer(&irradianceEnvironmentMapBufferDesc, nullptr, "IrradianceEnvironmentMapBufferFast");

            PrecomputeEnvironmentMaps(
                renderBackend,
                shaderLibrary,
                *commandList,
                skyLight->cubemapSize,
                skyLight->environmentMapTexture.GetHandle(),
                convolvedEnvironmentMapTexture,
                irradianceEnvironmentMapTexture,
                irradianceEnvironmentMapBuffer,
                irradianceEnvironmentMapBufferFast);
        }

        static int first11 = 0;
        if (ShouldUpdateRayTracingScene())
        {
            static std::vector<RenderBackendRayTracingGeometryDesc> geometryDescs;
            if (first11 == 0)
            {
                rayTracingScene->transformMatrixCount = uint32(meshes.size());
                RenderBackendBufferDesc transformBufferRowMajorUploadDesc = RenderBackendBufferDesc::CreateUpload(rayTracingScene->transformMatrixCount * uint32(sizeof(float)) * 16);
                rayTracingScene->transformBufferRowMajorUpload = renderBackend->CreateBuffer(&transformBufferRowMajorUploadDesc, nullptr, "RowMajorTransformUploadBuffer");
                RenderBackendBufferDesc transformBufferRowMajorDesc = RenderBackendBufferDesc::Create(uint32(sizeof(float)) * 16, rayTracingScene->transformMatrixCount, RenderBackendBufferCreateFlags::UnorderedAccess | RenderBackendBufferCreateFlags::ShaderResource | RenderBackendBufferCreateFlags::RayTracingAccelerationStructure);
                rayTracingScene->transformBufferRowMajor = renderBackend->CreateBuffer(&transformBufferRowMajorDesc, nullptr, "RowMajorTransformBuffer");

                for (const MeshRenderObject* mesh : meshes)
                {
                    RenderBackendRayTracingGeometryDesc geometryDesc = {};
                    geometryDesc.type = RenderBackendRayTracingGeometryType::Triangles;
                    geometryDesc.flags = RenderBackendRayTracingGeometryFlags::Opaque;
                    geometryDesc.triangleDesc.indexCount = mesh->indexCount;
                    geometryDesc.triangleDesc.vertexCount = mesh->vertexCount;
                    geometryDesc.triangleDesc.vertexStride = 3 * sizeof(float);
                    geometryDesc.triangleDesc.vertexBuffer = mesh->vertexBuffers[0];
                    geometryDesc.triangleDesc.vertexOffset = 0;
                    geometryDesc.triangleDesc.indexBuffer = mesh->indexBuffer;
                    geometryDesc.triangleDesc.indexOffset = 0;//mesh->baseIndex * sizeof(uint32);
                    geometryDesc.triangleDesc.transformBuffer = rayTracingScene->transformBufferRowMajor;
                    geometryDesc.triangleDesc.transformOffset = uint32(geometryDescs.size()) * uint32(sizeof(float)) * 16;
                    geometryDescs.push_back(geometryDesc);

                    rayTracingScene->rowMajorTransforms.push_back(Math::Transpose(glm::scale(mesh->localToWorldMatrix, Vector3(0.01f, 0.01f, 0.01f))));
                }

                if (rayTracingScene->transformMatrixCount > 0)
                {
                    renderBackend->UpdateBuffer(rayTracingScene->transformBufferRowMajorUpload, 0, rayTracingScene->rowMajorTransforms.data(), transformBufferRowMajorDesc.size);

                    RenderBackendBarrier barrier1[] =
                    {
                        RenderBackendBarrier(rayTracingScene->transformBufferRowMajor, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                    };
                    commandList->Transitions(barrier1, 1);

                    commandList->CopyBuffer(
                        rayTracingScene->transformBufferRowMajorUpload,
                        0,
                        rayTracingScene->transformBufferRowMajor,
                        0,
                        transformBufferRowMajorDesc.size);

                    RenderBackendBarrier barrier2[] =
                    {
                        RenderBackendBarrier(rayTracingScene->transformBufferRowMajor, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::UnorderedAccess)
                    };
                    commandList->Transitions(barrier2, 1);
                }
            }

            if (first11 == 100)
            {
                RenderBackendRayTracingBottomLevelAccelerationStructureDesc blasDesc =
                {
                    .buildFlags = RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastTrace,
                    .geometryCount = uint32(geometryDescs.size()),
                    .geometryDescs = geometryDescs.data()
                };
                rayTracingScene->bottomLevelAccelerationStructure = renderBackend->CreateRayTracingBottomLevelAccelerationStructure(&blasDesc, "BLAS");

                RenderBackendRayTracingInstance geometryInstance =
                {
                    .transformMatrix = Matrix4x4f(1.0f),
                    .instanceID = 0,
                    .instanceMask = 0xff,
                    .instanceContributionToHitGroupIndex = 0,
                    .flags = RenderBackendRayTracingInstanceFlags::TriangleFacingCullDisable,
                    .blas = rayTracingScene->bottomLevelAccelerationStructure
                };

                RenderBackendRayTracingTopLevelAccelerationStructureDesc tlasDesc =
                {
                    .buildFlags = RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastTrace,
                    .geometryFlags = RenderBackendRayTracingGeometryFlags::Opaque,
                    .instanceCount = 1,
                    .instances = &geometryInstance
                };
                rayTracingScene->topLevelAccelerationStructure = renderBackend->CreateRayTracingTopLevelAccelerationStructure(&tlasDesc, "RayTracingSceneTLAS");
            }

            if (first11 == 200)
            {
                commandList->BuildRayTracingBottomLevelAccelerationStructure(rayTracingScene->bottomLevelAccelerationStructure);
            }

            if (first11 == 500)
            {
                commandList->BuildRayTracingTopLevelAccelerationStructure(rayTracingScene->topLevelAccelerationStructure);
            }

            first11++;
        }
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