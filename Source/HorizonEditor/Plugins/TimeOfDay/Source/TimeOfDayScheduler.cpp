#include "TimeOfDayScheduler.h"

namespace Horizon
{
    TimeOfDayScheduler::TimeOfDayScheduler(Scene* scene)
        : scene(scene)
    {
        entityHandle = scene->CreateEntity("TimeOfDayScheduler");

        transform = scene->GetEntityManager()->TryGetComponent<TransformComponent>(entityHandle);
        transform->rotation = Vector3f(11.0f, 6.0f, 0.0f);

        {
            LightComponent& component = scene->GetEntityManager()->AddComponent<LightComponent>(entityHandle);
            component.type = LightComponent::Type::Distant;
            component.direction = DefaultLightDirection; //
            component.direction = Math::Normalize(Vector3f(-0.102607988f, 0.190808982f, -0.976249754f));
            component.color = Vector3f(1.0f, 1.0f, 1.0f);
            component.luminousIntensity = 120000.0f;
            component.apexAngleInDegrees = 0.5357f;
            component.castDynamicShadows = true;
            component.useColorTemperature = true;
            component.colorTemperature = 6500.0f;
            component.usedAsAtmosphericLight = true;
            //component.shadowMapSize = 4096;
            component.shadowCascadeSplitLambda = 0.8f;
            component.CreateRenderObject(scene->GetRenderScene());
        }
        sunLight = scene->GetEntityManager()->TryGetComponent<LightComponent>(entityHandle);

        {
            RenderBackend* renderBackend = scene->GetRenderScene()->renderBackend;
            RenderSystem* renderSystem = HorizonEngine::GetInstance()->GetSubsystem<RenderSystem>();

            const uint32 environmentMapTextureSize = 128;
            const uint32 environmentMapTextureMipLevelCount = Math::MaxMipLevelCount(environmentMapTextureSize);
            RenderBackendTextureHandle environmentMapTextureLatLong = LoadTextureFromHDRFile(renderBackend, "../../../Assets/HDRIs/HDR_029_Sky_Cloudy_Ref.hdr");
            RenderBackendTextureDesc environmentMapTextureDesc = RenderBackendTextureDesc::CreateCube(
                environmentMapTextureSize,
                RenderBackendTextureFormat::R11G11B10Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
                environmentMapTextureMipLevelCount);
            RenderBackendTextureHandle environmentMapTexture = renderBackend->CreateTexture(&environmentMapTextureDesc, nullptr, "EnvironmentMapTexture");

            RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);
            // RenderBackendBarrier transitions[] =
            // {
            //     RenderBackendBarrier(targetTexture->GetHandle(), RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::Undefined, RenderBackendResourceState::ShaderResource),
            // };
            // commandList->Transitions(transitions, 1);

            ConvertLatLongToCubemap(renderBackend, renderSystem->GetShaderLibrary(), *commandList, environmentMapTextureLatLong, environmentMapTexture, environmentMapTextureSize);
            GenerateCubemapMips(renderBackend, renderSystem->GetShaderLibrary(), *commandList, environmentMapTexture, environmentMapTextureMipLevelCount);

            renderBackend->SubmitCommandLists(&commandList, 1, RenderBackendSwapChainHandle::Null);

            SkyLightComponent& component = scene->GetEntityManager()->AddComponent<SkyLightComponent>(entityHandle);
            component.cubemapSize = environmentMapTextureSize;
            component.environmentMapTexture = RenderGraphPersistentTexture("EnvironmentMapTexture", environmentMapTextureDesc, environmentMapTexture);
            component.CreateRenderObject(scene->GetRenderScene());
        }
        skyLight = scene->GetEntityManager()->TryGetComponent<SkyLightComponent>(entityHandle);

        {
            SkyAtmosphereComponent& component = scene->GetEntityManager()->AddComponent<SkyAtmosphereComponent>(entityHandle);
            component.CreateRenderObject(scene->GetRenderScene());
        }
        skyAtmosphere = scene->GetEntityManager()->TryGetComponent<SkyAtmosphereComponent>(entityHandle);

        {
            GlobalFogComponent& component = scene->GetEntityManager()->AddComponent<GlobalFogComponent>(entityHandle);
        }
        globalFog = scene->GetEntityManager()->TryGetComponent<GlobalFogComponent>(entityHandle);

        {
            VolumetricCloudComponent& component = scene->GetEntityManager()->AddComponent<VolumetricCloudComponent>(entityHandle);
            //component.CreateRenderObject(scene->GetRenderScene());
        }
        volumetricCloud = scene->GetEntityManager()->TryGetComponent<VolumetricCloudComponent>(entityHandle);
    }

    TimeOfDayScheduler::~TimeOfDayScheduler()
    {

    }

    void TimeOfDayScheduler::Tick(float deltaTime)
    {
        transform = scene->GetEntityManager()->TryGetComponent<TransformComponent>(entityHandle);
        transform->rotation.x += deltaTime / loopTime * 360.0f;
        transform->rotation.x = std::fmod(transform->rotation.x, 360.0f);
    }
}