#include "HorizonEditor.h"

#include "USDModule.h"
#include "RenderDocPlugin.h"

#include <optick.h>

#include "TextureImporter.h"
#include "../Plugins/Streamline/Source/StreamlineModule.h"

// TODO: delete this
#define BIND_FUNCTION(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)> (args)...); }

namespace Horizon
{
    HorizonEditor* HorizonEditor::Instance = nullptr;

    HorizonEditor::HorizonEditor()
    {
        assert(Instance == nullptr);
        Instance = this;

        applicationName = HORIZON_EDITOR_APPLICATION_NAME;
    }

    HorizonEditor::~HorizonEditor()
    {
        assert(Instance != nullptr);
        Instance = nullptr;
    }

    bool HorizonEditor::Init(int argc, char** argv)
    {
        // Initialize path to executable
        executablePath = argv[0];

        // Initialize logging system
        CreateConsoleLogger_Deprecated();
//
//        JobSystemInit(HE::GetNumberOfProcessors(), HE_JOB_SYSTEM_NUM_FIBIERS, HE_JOB_SYSTEM_FIBER_STACK_SIZE);
//
        GLFWInit();

        // Hard coded initial window size.
        // TODO: Figure out best practice for first-time boot up of editor.
        uint32 initialWidth = 1920;
        uint32 initialHeight = 1080;

        WindowCreateFlags windowFlags = HORIZON_WINDOW_CREATE_FLAG_BIT_RESIZABLE | HORIZON_WINDOW_CREATE_FLAG_BIT_MAXIMIZED;

        // Create main window
        WindowCreateInfo windowInfo =
        {
            .width = initialWidth,
            .height = initialHeight,
            .title = applicationName.c_str(),
            .icon = "../../../Assets/Icons/horizon.png",
            .flags = windowFlags
        };
        window = new Window(&windowInfo);

        Input::SetCurrentContext(window->GetGLFWwindow());

        //window->keyPressEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyPressedEvent);
        //window->keyReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyReleasedEvent);
        //window->mouseButtonPressEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonPressedEvent);
        //window->mouseButtonReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonReleasedEvent);
//
//        PhysXInit();
//        Audio::AudioEngineInit();
//
        RenderDocPluginInit();
//

        InitializeEngine();
        streamlineContext = HorizonEngine::GetInstance()->streamlineContext;

        engine = HorizonEngine::GetInstance();
        RenderSystem* renderSystem = engine->GetSubsystem<RenderSystem>();
        renderBackend = renderSystem->GetRenderBackend();
        RenderGraphResourcePool* renderGraphResourcePool = renderSystem->GetRenderGraphResourcePool();

        InitializeImGuiContext();

        std::string usdPluginsPath = executablePath.append("usd").string();
        USDInit(usdPluginsPath);

        RenderBackendSwapChainDesc swapChainDesc =
        {
            .width = window->GetWidth(),
            .height = window->GetHeight(),
            .windowHandle = (uint64)window->GetNativeHandle(),
            .numBuffers = 3,
            .vsync = false,
            .format = RenderBackendTextureFormat::R10G10B10A2Unorm,
            .presentMode = RenderBackendSwapChainPresentMode::Immediate,
        };
        swapChain = renderBackend->CreateSwapChain(&swapChainDesc);
        swapChainWidth = window->GetWidth();
        swapChainHeight = window->GetHeight();

        RenderBackendTextureFormat targetTextureFormat = RenderBackendTextureFormat::R10G10B10A2Unorm;
        RenderGraphTextureDesc targetTextureDesc = RenderGraphTextureDesc::Create2D(
            swapChainWidth,
            swapChainHeight,
            targetTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
            RenderBackendTextureClearValue::Black,
            1,
            1,
            RenderBackendResourceState::ShaderResource); // TODO: handle transition
        targetTexture = renderGraphResourcePool->AllocateTexture(targetTextureDesc, "SceneViewTexture");

        RenderGraphTextureDesc previewTextureDesc = RenderGraphTextureDesc::Create2D(
            previewTextureWidth,
            previewTextureHeight,
            targetTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
            RenderBackendTextureClearValue::Black,
            1,
            1,
            RenderBackendResourceState::ShaderResource); // TODO: handle transition
        previewTexture = renderGraphResourcePool->AllocateTexture(previewTextureDesc, "SceneViewPreviewTexture");

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

        renderer = renderSystem->CreateRenderer();
        previewRenderer = renderSystem->CreateRenderer();

        editorSceneManager = new EditorSceneManager();
        Scene* scene = editorSceneManager->CreateScene("DefaultScene");
        {
            editorSceneManager->SetActiveScene(scene);

            EntityHandle camera01 = scene->CreateEntity("Camera01");
            {
                CameraComponent& cameraComponent = scene->GetEntityManager()->AddComponent<CameraComponent>(camera01);
                cameraComponent.fieldOfViewAxis = FieldOfViewAxis::Horizontal;
                cameraComponent.fieldOfView = Math::DegreesToRadians(90.0f);
                cameraComponent.nearClippingPlane = 0.1f;
                cameraComponent.farClippingPlane = 1000.0f;
                cameraComponent.overrideAspectRatio = false;
                cameraComponent.aspectRatio = 16.0f / 9.0f;
            }

            EntityHandle sunLight = scene->CreateEntity("SunLight");
            {
                TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(sunLight);
                transformComponent.rotation = Vector3(11.0f, 6.0f, 0.0f);
                //transformComponent.rotation = Vector3(0.0f, 0.0f, 0.0f);

                LightComponent& lightComponent = scene->GetEntityManager()->AddComponent<LightComponent>(sunLight);
                lightComponent.type = LightComponent::Type::Distant;
                lightComponent.direction = DefaultLightDirection; //
                lightComponent.direction = Math::Normalize(Vector3(-0.102607988f, 0.190808982f, -0.976249754f));
                lightComponent.color = Vector3(1.0f, 1.0f, 1.0f);
                lightComponent.luminousIntensity = 120000.0f;
                lightComponent.apexAngleInDegrees = 0.5357f;
                lightComponent.castDynamicShadows = true;
                lightComponent.useColorTemperature = true;
                lightComponent.colorTemperature = 6500.0f;
                lightComponent.usedAsAtmosphericLight = true;
                lightComponent.CreateRenderObject(scene->GetRenderScene());
            }

            // Create point light 0
            EntityHandle pointLight0 = scene->CreateEntity("PointLight0");
            {
                TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(pointLight0);
                transformComponent.position = Vector3(9.0f, -3.0f, 1.5f);

                LightComponent& lightComponent = scene->GetEntityManager()->AddComponent<LightComponent>(pointLight0);
                lightComponent.type = LightComponent::Type::Point;
                lightComponent.color = Vector4(255.0f / 255.0f, 41.0f / 255.0f, 0.0f / 255.0f, 1.0f);
                lightComponent.luminousIntensity = 500.0f;
                lightComponent.radius = 3.0f;
                lightComponent.castDynamicShadows = true;
                lightComponent.CreateRenderObject(scene->GetRenderScene());
            }

            // Create point light 1
            EntityHandle pointLight1 = scene->CreateEntity("PointLight1");
            {
                TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(pointLight1);
                transformComponent.position = Vector3(-9.5f, 3.5f, 1.5f);

                LightComponent& lightComponent = scene->GetEntityManager()->AddComponent<LightComponent>(pointLight1);
                lightComponent.type = LightComponent::Type::Point;
                lightComponent.color = Vector4(0.0f, 7.0f / 255.0f, 255.0f / 255.0f, 1.0f);
                lightComponent.luminousIntensity = 500.0f;
                lightComponent.radius = 3.0f;
                lightComponent.castDynamicShadows = true;
                lightComponent.CreateRenderObject(scene->GetRenderScene());
            }

            EntityHandle skyDome = scene->CreateEntity("SkyDome");
            {
                SkyLightComponent& skyLightComponent = scene->GetEntityManager()->AddComponent<SkyLightComponent>(skyDome);
                skyLightComponent.cubemapSize = environmentMapTextureSize;
                skyLightComponent.environmentMapTexture = RenderGraphPersistentTexture("EnvironmentMapTexture", environmentMapTextureDesc, environmentMapTexture);
                skyLightComponent.CreateRenderObject(scene->GetRenderScene());
            }

            EntityHandle skyAtmosphere = scene->CreateEntity("SkyAtmosphere");
            {
                SkyAtmosphereComponent& skyAtmosphereComponent = scene->GetEntityManager()->AddComponent<SkyAtmosphereComponent>(skyAtmosphere);
                skyAtmosphereComponent.CreateRenderObject(scene->GetRenderScene());
            }

            EntityHandle localFogVolume = scene->CreateEntity("LocalFogVolume");
            {
                LocalFogVolumeComponent& localFogVolumeComponent = scene->GetEntityManager()->AddComponent<LocalFogVolumeComponent>(localFogVolume);
                localFogVolumeComponent.CreateRenderObject(scene->GetRenderScene());
            }

            // DistantLightRenderObject* distantLight = new DistantLightRenderObject();
            // distantLight->usedAsAtmosphericLight = true;
            // renderScene->AddLight(distantLight);
            //
            // SkyAtmosphereRenderObject* skyAtmosphere = new SkyAtmosphereRenderObject();
            // renderScene->AddSkyAtmosphere(skyAtmosphere);
        }

        // TODO: Test New Sponaza
        USDImportSettings settings = {};
        settings.importMeshes = true;
        settings.importMaterials = true;
        //USDImport("../../../Assets/Test/NewSponza/NewSponza.usdc", &settings, false);
        USDImport(scene, "../../../Assets/Test/Sponza/sponza.usdc", &settings, false);

        editorCamera.position = Vector3(0.0f, 0.0f, 5.0f);
        editorCamera.rotation = Vector3(0.0f, 0.0f, 0.0f);
        editorCamera.fieldOfViewAxis = FieldOfViewAxis::Horizontal;
        editorCamera.fieldOfView = Math::DegreesToRadians(90.0f);
        editorCamera.aspectRatio = (float)swapChainWidth / (float)swapChainHeight;
        //editorCamera.aspectRatio = 16.0f / 9.0f;
        editorCamera.nearClippingPlane = 0.1f;
#if HORIZON_EXPERIMENTAL_INFINITE_PERSPECTIVE
        editorCamera.farClippingPlane = std::numeric_limits<float>::max();
#else
        editorCamera.farClippingPlane = 100.0f;
#endif
        editorCamera.cameraSpeed = 1.0f;
        editorCamera.overrideAspectRatio = false;

        renderSettings.indirectLightingIntensity = 1.0f;
        renderSettings.shadowsTechnique = ShadowsTechnique::ShadowMap;
        renderSettings.reflectionsTechnique = ReflectionsTechnique::ScreenSpaceReflections;
        renderSettings.superSamplingSettings.superSamplingTechnique = SuperSamplingTechnique::FSR;
        renderSettings.superSamplingSettings.qualityMode = 5;
        renderSettings.superSamplingSettings.desiredRenderResolutionPercentage = 1.0f;

//        ShaderGraphSystemInit();
//
//        selectionManager = new SelectionManager();
//
//        Setup();

        return true;
    }

    void HorizonEditor::Exit()
    {
        if (window) delete window;

        GLFWExit();
    }

    float HorizonEditor::CalculateDeltaTime()
    {
        static std::chrono::steady_clock::time_point previousTimePoint{ std::chrono::steady_clock::now() };
        std::chrono::steady_clock::time_point timePoint = std::chrono::steady_clock::now();
        std::chrono::duration<float> timeDuration = std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - previousTimePoint);
        float deltaTime = timeDuration.count();
        previousTimePoint = timePoint;

        return deltaTime;
    }

    extern float CascadedShadowMapPracticalSplitScheme(uint32 cascadeIndex, uint32 cascadeCount, float nearPlane, float farPlane, float lambda);
    extern Vector4f ComputeViewSpaceShadowCascadeMinimumBoundingSphere(float n, float f, float tanHalfVerticalFOV, float aspectRatio);

    void HorizonEditor::Tick()
    {
        OPTICK_EVENT();

        WindowState state = window->GetState();

        if (state == WindowState::Minimized)
        {
            return;
        }

#if HORIZON_EXPERIMENTAL_STREAMLINE
        streamlineContext->ReflexSetMarkerRenderSubmitStart(frameIndex);
#endif

        deltaTimeInSeconds = CalculateDeltaTime();

        //
        //            //if (!window->IsFocused())
        //            //{
        //            //    OSSuspendCurrentThread(0.05f);
        //            //}
        //

        uint32 width = window->GetWidth();
        uint32 height = window->GetHeight();
        if (width != swapChainWidth || height != swapChainHeight)
        {
            // Vulkan does not support swap chains with width and height set to zero
            if (width != 0 && height != 0)
            {
                renderBackend->ResizeSwapChain(swapChain, &width, &height);
                swapChainWidth = width;
                swapChainHeight = height;
            }
        }

        engine->GetSubsystem<RenderSystem>()->BeginDrawUI(imguiContext);

        OnDrawUI();

        engine->GetSubsystem<RenderSystem>()->EndDrawUI();

        editorCamera.Update(deltaTimeInSeconds);

#if HORIZON_EXPERIMENTAL_STREAMLINE
        streamlineContext->ReflexSetMarkerSimulationStart(frameIndex);
#endif

        engine->Tick(deltaTimeInSeconds);

#if HORIZON_EXPERIMENTAL_STREAMLINE
        streamlineContext->ReflexSetMarkerSimulationEnd(frameIndex);
#endif

        editorSceneManager->GetActiveScene()->Tick(deltaTimeInSeconds);


        Quaternion cameraOrientation = Math::QuaternionFromEulerAngles(Math::DegreesToRadians(editorCamera.GetRotation()));

        Vector3 cameraRightVector   = Math::Normalize(cameraOrientation * Vector3(1.0f, 0.0f, 0.0f));
        Vector3 cameraForwardVector = Math::Normalize(cameraOrientation * Vector3(0.0f, 1.0f, 0.0f));
        Vector3 cameraUpVector      = Math::Normalize(cameraOrientation * Vector3(0.0f, 0.0f, 1.0f));

        SceneView sceneView = {};
        sceneView.frameIndex = frameIndex;
        sceneView.deltaTimeInSeconds = deltaTimeInSeconds;
        sceneView.scene = editorSceneManager->GetActiveScene()->GetRenderScene();
        sceneView.renderSettings = renderSettings;
        sceneView.debugVisualizationMode = currentDebugVisualizationMode;
        sceneView.reset = false;
        sceneView.cameraPosition = editorCamera.GetPosition();
        sceneView.cameraRotation = editorCamera.GetRotation();
        sceneView.cameraUpVector = cameraUpVector;
        sceneView.cameraRightVector = cameraRightVector;
        sceneView.cameraForwardVector = cameraForwardVector;
        sceneView.verticalFOV = editorCamera.fieldOfView;
        if (editorCamera.fieldOfViewAxis == FieldOfViewAxis::Horizontal)
        {
            sceneView.verticalFOV = HorizontalFOVToVerticalFOV(editorCamera.fieldOfView, editorCamera.aspectRatio);
        }
        sceneView.aspectRatio = editorCamera.aspectRatio;
        sceneView.tanHalfVerticalFOV = std::tan(editorCamera.fieldOfView * 0.5f);
        sceneView.nearClippingPlane = std::max(editorCamera.nearClippingPlane, MinNearClippingPlane);
        sceneView.farClippingPlane = editorCamera.farClippingPlane;
        sceneView.backgroundColor = Vector3(0.0f, 0.0f, 0.0f);
        sceneView.targetWidth = swapChainWidth;
        sceneView.targetHeight = swapChainHeight;
        sceneView.targetTexture = targetTexture;
        sceneView.displayWidth = swapChainWidth;
        sceneView.displayHeight = swapChainHeight;

        sceneView.transformations.Update(sceneView.cameraPosition, sceneView.cameraRotation, sceneView.verticalFOV, sceneView.aspectRatio, sceneView.nearClippingPlane, sceneView.farClippingPlane);

        viewMatrix_deprecated = sceneView.transformations.worldToViewMatrix;
        projectionMatrix_deprecated = sceneView.transformations.viewToClipMatrix;

        RenderBackendCommandList* commandListUpload = new RenderBackendCommandList(GArena);
        engine->GetSubsystem<RenderSystem>()->UpdateImGuiData(commandListUpload);
        sceneView.scene->UpdateGPUScene(commandListUpload);
        renderBackend->SubmitCommandLists(&commandListUpload, 1, RenderBackendSwapChainHandle::Null);
        delete commandListUpload;


        SceneView previewSceneView = {};

        bool renderPreview = false;
        if (preview)
        {
            if (editorSceneManager->GetActiveScene()->GetEntityManager()->HasEntity(selectedEntity) && editorSceneManager->GetActiveScene()->GetEntityManager()->HasComponent<CameraComponent>(selectedEntity))
            {
                CameraComponent& previewCamera = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<CameraComponent>(selectedEntity);
                TransformComponent& transform = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<TransformComponent>(selectedEntity);

                Quaternion previewCameraOrientation = Math::QuaternionFromEulerAngles(Math::DegreesToRadians(transform.rotation));

                Vector3 preivewCameraUpVector = Math::Normalize(previewCameraOrientation * Vector3(0.0f, 0.0f, 1.0f));
                Vector3 preivewCameraRightVector = Math::Normalize(previewCameraOrientation * Vector3(1.0f, 0.0f, 0.0f));
                Vector3 preivewCameraForwardVector = Math::Normalize(previewCameraOrientation * Vector3(0.0f, 1.0f, 0.0f));

                previewSceneView.frameIndex = frameIndex;
                previewSceneView.deltaTimeInSeconds = deltaTimeInSeconds;
                previewSceneView.scene = editorSceneManager->GetActiveScene()->GetRenderScene();
                previewSceneView.renderSettings = renderSettings;
                previewSceneView.debugVisualizationMode = currentDebugVisualizationMode;
                previewSceneView.reset = false;
                previewSceneView.cameraPosition = transform.position;
                previewSceneView.cameraRotation = transform.rotation;
                previewSceneView.cameraUpVector = preivewCameraUpVector;
                previewSceneView.cameraRightVector = preivewCameraRightVector;
                previewSceneView.cameraForwardVector = preivewCameraForwardVector;
                previewSceneView.verticalFOV = previewCamera.fieldOfView;
                if (previewCamera.fieldOfViewAxis == FieldOfViewAxis::Horizontal)
                {
                    previewSceneView.verticalFOV = HorizontalFOVToVerticalFOV(previewCamera.fieldOfView, previewCamera.aspectRatio);
                }
                previewSceneView.aspectRatio = previewCamera.aspectRatio;
                previewSceneView.tanHalfVerticalFOV = std::tan(previewCamera.fieldOfView * 0.5f);
                previewSceneView.nearClippingPlane = std::max(previewCamera.nearClippingPlane, MinNearClippingPlane);
                previewSceneView.farClippingPlane = previewCamera.farClippingPlane;
                previewSceneView.backgroundColor = Vector3(0.0f, 0.0f, 0.0f);
                previewSceneView.targetWidth = previewTextureWidth;
                previewSceneView.targetHeight = previewTextureHeight;
                previewSceneView.targetTexture = previewTexture;
                previewSceneView.displayWidth = previewTextureWidth;
                previewSceneView.displayHeight = previewTextureHeight;
                previewSceneView.transformations.Update(previewSceneView.cameraPosition, previewSceneView.cameraRotation, previewSceneView.verticalFOV, previewSceneView.aspectRatio, previewSceneView.nearClippingPlane, previewSceneView.farClippingPlane);

                renderPreview = true;
            }
            else
            {
                // Clear preview texture
            }
        }

        if (renderPreview)
        {
            float distanceNear = previewSceneView.nearClippingPlane;
            float distanceFar = previewSceneView.farClippingPlane;
            float uLenNear = distanceNear * previewSceneView.tanHalfVerticalFOV;
            float rLenNear = uLenNear * previewSceneView.aspectRatio;
            float uLenFar = distanceFar * previewSceneView.tanHalfVerticalFOV;
            float rLenFar = uLenFar * previewSceneView.aspectRatio;
            Vector3 uNear = uLenNear * previewSceneView.cameraUpVector;
            Vector3 rNear = rLenNear * previewSceneView.cameraRightVector;
            Vector3 uFar = uLenFar * previewSceneView.cameraUpVector;
            Vector3 rFar = rLenFar * previewSceneView.cameraRightVector;
            Vector3 nearCenterPoint = previewSceneView.cameraPosition + distanceNear * previewSceneView.cameraForwardVector;
            Vector3 farCenterPoint = previewSceneView.cameraPosition + distanceFar * previewSceneView.cameraForwardVector;

            Vector3 corners[8];
            corners[0] = nearCenterPoint - uNear - rNear; // left-bottom
            corners[1] = nearCenterPoint - uNear + rNear; // right-bottom
            corners[2] = nearCenterPoint + uNear - rNear; // left-up
            corners[3] = nearCenterPoint + uNear + rNear; // right-up
            corners[4] = farCenterPoint - uFar - rFar; // left-bottom
            corners[5] = farCenterPoint - uFar + rFar; // right-bottom
            corners[6] = farCenterPoint + uFar - rFar; // left-up
            corners[7] = farCenterPoint + uFar + rFar; // right-up

            renderer->debugDrawLinesVertices.clear();
            renderer->DrawLine(corners[0], corners[1], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[1], corners[3], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2], corners[3], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2], corners[0], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[0+4], corners[1+4], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[1+4], corners[3+4], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2+4], corners[3+4], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2+4], corners[0+4], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[0], corners[4], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[1], corners[5], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2], corners[6], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[3], corners[7], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);

            previewSceneView.transformations.Finalize();

            LightRenderObject* sunLight = editorSceneManager->GetActiveScene()->GetRenderScene()->GetAtmosphericLight();
            uint32 shadowCascadeCount = sunLight->shadowCascadeCount;
            float shadowCascadeSplitLambda = sunLight->shadowCascadeSplitLambda;
            float cameraNearClippingPlane = previewSceneView.nearClippingPlane;
            float cameraFarClippingPlane = previewSceneView.farClippingPlane;
            float tanHalfVerticalFOV = previewSceneView.tanHalfVerticalFOV;
            float aspectRatio = previewSceneView.aspectRatio;
            Matrix4x4f inverseViewMatrix = previewSceneView.transformations.viewToWorldMatrix;
            Vector3f lightDirection = sunLight->GetDirection();
            const float maxShadowDistance = std::min(sunLight->maxShadowDistance, cameraFarClippingPlane);

            for (uint32 cascadeIndex = 0; cascadeIndex < shadowCascadeCount; cascadeIndex++)
            {
                float cascadeStartDistance = CascadedShadowMapPracticalSplitScheme(cascadeIndex, shadowCascadeCount, cameraNearClippingPlane, maxShadowDistance, shadowCascadeSplitLambda);
                float cascadeEndDistance = CascadedShadowMapPracticalSplitScheme(cascadeIndex + 1, shadowCascadeCount, cameraNearClippingPlane, maxShadowDistance, shadowCascadeSplitLambda);

                Vector4f viewSpaceBoundingSphere = ComputeViewSpaceShadowCascadeMinimumBoundingSphere(cascadeStartDistance, cascadeEndDistance, tanHalfVerticalFOV, aspectRatio);
                float boundingSphereRadius = std::ceil(viewSpaceBoundingSphere.w); // Use the ceilling function to increase stability.

                Vector4f viewSpaceBoundingSphereCenter = Vector4f(viewSpaceBoundingSphere.x, viewSpaceBoundingSphere.y, viewSpaceBoundingSphere.z, 1.0f);

                Vector4f boundingSphere = inverseViewMatrix * viewSpaceBoundingSphereCenter;
                boundingSphere.w = boundingSphereRadius;

                Vector3f boundingSphereCenter = Vector3f(boundingSphere.x, boundingSphere.y, boundingSphere.z);

                // Scene Independent Projection
                // GPU Gems 3. Chapter 10. Parallel-Split Shadow Maps on Programmable GPUs
                // {
                    //Vector4f viewSpaceBoundingSphereCenter = ;

                    // To avoid shimmering caused by camera movements, create a "stable" projection using the method described in the article "Stable Cascaded Shadow Maps" from ShaderX6.
                    // 1. Using a bounding sphere instead of a bounding box to guarantee the projection is rotation-invariant.
                    // 2. Moving the shadow caster camera in texel-sized increments.

                    //float snapX = std::fmodf(, 2.0f / shadowMapSize);
                    //float snapY = std::fmodf(, 2.0f / shadowMapSize);
                // }

                float minZ = -100.0f;//-boundingSphereRadius;
                float maxZ = boundingSphereRadius;

                Matrix4x4f viewMatrix = glm::lookAt(boundingSphereCenter, boundingSphereCenter + lightDirection, Vector3f(0.0f, 1.0f, 0.0f));
                Matrix4x4f projectionMatrix = Math::OrthographicProjection_ReverseZ_ZO(-boundingSphereRadius, boundingSphereRadius, -boundingSphereRadius, boundingSphereRadius, minZ, maxZ);

                renderer->DrawSphere(boundingSphereCenter, boundingSphereRadius, Vector4(1.0f, 0.0f, 0.0f, 1.0f));
            }
        }

        engine->GetSubsystem<RenderSystem>()->RenderSceneView(renderer, &sceneView);

        if (renderPreview)
        {
            engine->GetSubsystem<RenderSystem>()->RenderSceneView(previewRenderer, &previewSceneView);
        }

        RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);

        RenderBackendTextureHandle swapChainTexture = renderBackend->GetActiveSwapChainBuffer(swapChain);

        {
            RenderBackendBarrier transitions[] =
            {
                RenderBackendBarrier(targetTexture->GetHandle(), RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::CopySrc),
                RenderBackendBarrier(swapChainTexture, RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
            };
            commandList->Transitions(transitions, 2);
        }

        commandList->CopyTexture2D(
            targetTexture->GetHandle(),
            Offset2D(0, 0),
            0,
            swapChainTexture,
            Offset2D(0, 0),
            0,
            Extent2D(swapChainWidth, swapChainHeight));

        {
            RenderBackendBarrier transitions[] =
            {
                RenderBackendBarrier(targetTexture->GetHandle(), RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::CopySrc, RenderBackendResourceState::ShaderResource),
                RenderBackendBarrier(swapChainTexture, RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::CopyDst, RenderBackendResourceState::Present)
            };
            commandList->Transitions(transitions, 2);
        }

        renderBackend->SubmitCommandLists(&commandList, 1, swapChain);

        delete commandList;

#if HORIZON_EXPERIMENTAL_STREAMLINE
        streamlineContext->ReflexSetMarkerRenderSubmitEnd(frameIndex);
#endif

#if HORIZON_EXPERIMENTAL_STREAMLINE
        streamlineContext->ReflexSetMarkerPresentStart(frameIndex);
#endif

        renderBackend->PresentSwapChain(swapChain);

#if HORIZON_EXPERIMENTAL_STREAMLINE
        streamlineContext->ReflexSetMarkerPresentEnd(frameIndex);
#endif

        frameIndex++;
    }

    int HorizonEditor::Run()
    {
        while (!IsExitRequested())
        {
            OPTICK_FRAME("MainThread");

#if HORIZON_EXPERIMENTAL_STREAMLINE
            //streamlineContext->ReflexSleep(frameIndex);
            streamlineContext->ReflexSetMarkerControllerInputSample(frameIndex);
#endif

            window->ProcessEvents();

            if (window->ShouldClose())
            {
                SetExitRequest(true);
            }

            Tick();
        }

        return 0;
    }
}

int HorizonEditorMain(int argc, char** argv)
{
    int exitCode = EXIT_SUCCESS;
    Horizon::HorizonEditor* editor = new Horizon::HorizonEditor();
    bool result = editor->Init(argc, argv);
    if (result)
    {
        exitCode = editor->Run();
    }
    else
    {
        exitCode = EXIT_FAILURE;
    }
    editor->Exit();
    delete editor;
    return exitCode;
}