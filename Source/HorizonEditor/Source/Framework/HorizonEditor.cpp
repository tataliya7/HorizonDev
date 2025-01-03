#include "HorizonEditor.h"

#include "USDModule.h"
#include "RenderDocPlugin.h"
#include "TimeOfDayPlugin.h"

#include <optick.h>

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

//        PhysXInit();
//        Audio::AudioEngineInit();

        //RenderDocPluginInit();

        // UUID unit tests.
        {
            const UUID nil_uuid = UUID::NilUUID();
            const UUID zeroes_uuid{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            const UUID default_constructed{};
            assert(nil_uuid.IsNil());
            assert(zeroes_uuid.IsNil());
            assert(default_constructed.IsNil());
        }
        {
            UUID uuid = UUID::FromString("0C886E65-49E9-40B3-9E00-05649FF49C06");
            std::string uuidString = UUID::ToString(uuid);
            assert(uuidString == "0c886e65-49e9-40b3-9e00-05649ff49c06");
        }
        {
            UUID uuid = UUID::FromString("d30a0e60-14a2-11ec-8b99-f7736944db8b1");
            assert(uuid.IsNil());
        }
        {
            UUID uuid = UUID::FromString("d30a0e60-14a2-11ec-8b99-f7736944db8b");
            std::string uuidString = UUID::ToString(uuid);
            assert(uuidString == "d30a0e60-14a2-11ec-8b99-f7736944db8b");
        }
        {
            std::stringstream ss;
            const UUID uuid1 = { 3540651616, 5282, 4588, 139, 153, 0xf7, 0x73, 0x69, 0x44, 0xdb, 0x8b };
            //ss << uuid1;
            assert(UUID::ToString(uuid1) == "d30a0e60-14a2-11ec-8b99-f7736944db8b");
            UUID uuid2 = UUID::FromString("d30a0e60-14a2-11ec-8b99-f7736944db8b");
            assert(uuid1 == uuid2);
        }

        assetDatabase = new AssetDatabase();

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


        renderer = renderSystem->CreateRenderer();
        previewRenderer = renderSystem->CreateRenderer();

        editorSceneManager = new EditorSceneManager();
        Scene* scene = editorSceneManager->CreateScene("DefaultScene");
        {
            editorSceneManager->SetActiveScene(scene);

            // EntityHandle sunLight = scene->CreateEntity("SunLight");
            // {
            //     TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(sunLight);
            //     transformComponent.rotation = Vector3f(11.0f, 6.0f, 0.0f);
            //     //transformComponent.rotation = Vector3f(0.0f, 0.0f, 0.0f);
            //
            //     LightComponent& lightComponent = scene->GetEntityManager()->AddComponent<LightComponent>(sunLight);
            //     lightComponent.type = LightComponent::Type::Distant;
            //     lightComponent.direction = DefaultLightDirection; //
            //     lightComponent.direction = Math::Normalize(Vector3f(-0.102607988f, 0.190808982f, -0.976249754f));
            //     lightComponent.color = Vector3f(1.0f, 1.0f, 1.0f);
            //     lightComponent.luminousIntensity = 120000.0f;
            //     lightComponent.apexAngleInDegrees = 0.5357f;
            //     lightComponent.castDynamicShadows = true;
            //     lightComponent.useColorTemperature = true;
            //     lightComponent.colorTemperature = 6500.0f;
            //     lightComponent.usedAsAtmosphericLight = true;
            //     //lightComponent.shadowMapSize = 4096;
            //     lightComponent.shadowCascadeSplitLambda = 0.8f;
            //     lightComponent.CreateRenderObject(scene->GetRenderScene());
            // }
            //
            // EntityHandle skyDome = scene->CreateEntity("SkyDome");
            // {
            //     SkyLightComponent& skyLightComponent = scene->GetEntityManager()->AddComponent<SkyLightComponent>(skyDome);
            //     skyLightComponent.cubemapSize = environmentMapTextureSize;
            //     skyLightComponent.environmentMapTexture = RenderGraphPersistentTexture("EnvironmentMapTexture", environmentMapTextureDesc, environmentMapTexture);
            //     skyLightComponent.CreateRenderObject(scene->GetRenderScene());
            // }
            //
            // EntityHandle skyAtmosphere = scene->CreateEntity("SkyAtmosphere");
            // {
            //     SkyAtmosphereComponent& skyAtmosphereComponent = scene->GetEntityManager()->AddComponent<SkyAtmosphereComponent>(skyAtmosphere);
            //     skyAtmosphereComponent.CreateRenderObject(scene->GetRenderScene());
            // }

            // Create point light 0
            EntityHandle pointLight0 = scene->CreateEntity("PointLight0");
            {
                TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(pointLight0);
                transformComponent.position = Vector3f(9.0f, -3.0f, 1.5f);

                LightComponent& lightComponent = scene->GetEntityManager()->AddComponent<LightComponent>(pointLight0);
                lightComponent.type = LightComponent::Type::Point;
                lightComponent.color = Vector4f(255.0f / 255.0f, 41.0f / 255.0f, 0.0f / 255.0f, 1.0f);
                lightComponent.luminousIntensity = 500.0f;
                lightComponent.radius = 3.0f;
                lightComponent.castDynamicShadows = true;
                lightComponent.CreateRenderObject(scene->GetRenderScene());
            }

            // Create point light 1
            EntityHandle pointLight1 = scene->CreateEntity("PointLight1");
            {
                TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(pointLight1);
                transformComponent.position = Vector3f(-9.5f, 3.5f, 1.5f);

                LightComponent& lightComponent = scene->GetEntityManager()->AddComponent<LightComponent>(pointLight1);
                lightComponent.type = LightComponent::Type::Point;
                lightComponent.color = Vector4f(0.0f, 7.0f / 255.0f, 255.0f / 255.0f, 1.0f);
                lightComponent.luminousIntensity = 500.0f;
                lightComponent.radius = 3.0f;
                lightComponent.castDynamicShadows = true;
                lightComponent.CreateRenderObject(scene->GetRenderScene());
            }

            EntityHandle localFogVolume = scene->CreateEntity("LocalFogVolume");
            {
                LocalFogVolumeComponent& localFogVolumeComponent = scene->GetEntityManager()->AddComponent<LocalFogVolumeComponent>(localFogVolume);
                localFogVolumeComponent.CreateRenderObject(scene->GetRenderScene());
            }

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
        }

        timeOfDayScheduler = new TimeOfDayScheduler(scene);

        //std::filesystem::path assetPath = "../../../Assets/UsdSkelExamples/HumanFemale/HumanFemale.walk.usd";
        std::filesystem::path assetPath = "../../../Assets/Test/Sponza/sponza.usdc";
        assetDatabase->ImportAsset(assetPath, scene);

        editorCamera.position = Vector3f(0.0f, 0.0f, 5.0f);
        editorCamera.rotation = Vector3f(0.0f, 0.0f, 0.0f);
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

        renderSettings.globalIlluminationSettings.indirectLightingIntensity = 1.0f;
        renderSettings.shadowsTechnique = ShadowsTechnique::ShadowMap;
        renderSettings.reflectionsTechnique = ReflectionsTechnique::ScreenSpaceReflections;
        renderSettings.superSamplingSettings.superSamplingTechnique = SuperSamplingTechnique::None;
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

        timeOfDayScheduler->Tick(deltaTimeInSeconds); // TODO
        editorSceneManager->GetActiveScene()->Tick(deltaTimeInSeconds);

        Quaternion cameraOrientation = Math::QuaternionFromEulerAngles(Math::DegreesToRadians(editorCamera.GetRotation()));

        Vector3f cameraRightVector   = Math::Normalize(cameraOrientation * Vector3f(1.0f, 0.0f, 0.0f));
        Vector3f cameraForwardVector = Math::Normalize(cameraOrientation * Vector3f(0.0f, 1.0f, 0.0f));
        Vector3f cameraUpVector      = Math::Normalize(cameraOrientation * Vector3f(0.0f, 0.0f, 1.0f));

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
        sceneView.backgroundColor = Vector3f(0.0f, 0.0f, 0.0f);
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

                Vector3f preivewCameraUpVector = Math::Normalize(previewCameraOrientation * Vector3f(0.0f, 0.0f, 1.0f));
                Vector3f preivewCameraRightVector = Math::Normalize(previewCameraOrientation * Vector3f(1.0f, 0.0f, 0.0f));
                Vector3f preivewCameraForwardVector = Math::Normalize(previewCameraOrientation * Vector3f(0.0f, 1.0f, 0.0f));

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
                previewSceneView.backgroundColor = Vector3f(0.0f, 0.0f, 0.0f);
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
            Vector3f uNear = uLenNear * previewSceneView.cameraUpVector;
            Vector3f rNear = rLenNear * previewSceneView.cameraRightVector;
            Vector3f uFar = uLenFar * previewSceneView.cameraUpVector;
            Vector3f rFar = rLenFar * previewSceneView.cameraRightVector;
            Vector3f nearCenterPoint = previewSceneView.cameraPosition + distanceNear * previewSceneView.cameraForwardVector;
            Vector3f farCenterPoint = previewSceneView.cameraPosition + distanceFar * previewSceneView.cameraForwardVector;

            Vector3f corners[8];
            corners[0] = nearCenterPoint - uNear - rNear; // left-bottom
            corners[1] = nearCenterPoint - uNear + rNear; // right-bottom
            corners[2] = nearCenterPoint + uNear - rNear; // left-up
            corners[3] = nearCenterPoint + uNear + rNear; // right-up
            corners[4] = farCenterPoint - uFar - rFar; // left-bottom
            corners[5] = farCenterPoint - uFar + rFar; // right-bottom
            corners[6] = farCenterPoint + uFar - rFar; // left-up
            corners[7] = farCenterPoint + uFar + rFar; // right-up

            renderer->debugDrawLinesVertices.clear();
            renderer->DrawLine(corners[0], corners[1], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[1], corners[3], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2], corners[3], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2], corners[0], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[0+4], corners[1+4], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[1+4], corners[3+4], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2+4], corners[3+4], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2+4], corners[0+4], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[0], corners[4], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[1], corners[5], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[2], corners[6], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
            renderer->DrawLine(corners[3], corners[7], Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);

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

#if 1
                float halfCascadeFrustumNearPlaneExtentX = cascadeStartDistance * tanHalfVerticalFOV * aspectRatio;
                float halfCascadeFrustumNearPlaneExtentY = cascadeStartDistance * tanHalfVerticalFOV;

                float halfCascadeFrustumFarPlaneExtentX = cascadeEndDistance * tanHalfVerticalFOV * aspectRatio;
                float halfCascadeFrustumFarPlaneExtentY = cascadeEndDistance * tanHalfVerticalFOV;

                Vector4f cascadeFrustumCorners[8] =
                {
                    Vector4f( halfCascadeFrustumNearPlaneExtentX,  halfCascadeFrustumNearPlaneExtentY, -cascadeStartDistance, 1.0f), // top right
                    Vector4f( halfCascadeFrustumNearPlaneExtentX, -halfCascadeFrustumNearPlaneExtentY, -cascadeStartDistance, 1.0f), // bottom right
                    Vector4f(-halfCascadeFrustumNearPlaneExtentX,  halfCascadeFrustumNearPlaneExtentY, -cascadeStartDistance, 1.0f), // top left
                    Vector4f(-halfCascadeFrustumNearPlaneExtentX, -halfCascadeFrustumNearPlaneExtentY, -cascadeStartDistance, 1.0f), // bottom left
                    Vector4f( halfCascadeFrustumFarPlaneExtentX,  halfCascadeFrustumFarPlaneExtentY, -cascadeEndDistance, 1.0f), // top right
                    Vector4f( halfCascadeFrustumFarPlaneExtentX, -halfCascadeFrustumFarPlaneExtentY, -cascadeEndDistance, 1.0f), // bottom right
                    Vector4f(-halfCascadeFrustumFarPlaneExtentX,  halfCascadeFrustumFarPlaneExtentY, -cascadeEndDistance, 1.0f), // top left
                    Vector4f(-halfCascadeFrustumFarPlaneExtentX, -halfCascadeFrustumFarPlaneExtentY, -cascadeEndDistance, 1.0f)  // bottom left
                };

                Vector3f boundingSphereCenter = Vector3f(0.0f, 0.0f, 0.0f);
                for (uint32 i = 0; i < 8; i++)
                {
                    cascadeFrustumCorners[i] = inverseViewMatrix * cascadeFrustumCorners[i];
                    boundingSphereCenter += Vector3f(cascadeFrustumCorners[i].x, cascadeFrustumCorners[i].y, cascadeFrustumCorners[i].z);
                }
                boundingSphereCenter /= 8.0f;

                float boundingSphereRadius = 0.0f;
                for (uint32 i = 0; i < 8; i++)
                {
                    float distance = glm::length(Vector3f(cascadeFrustumCorners[i].x, cascadeFrustumCorners[i].y, cascadeFrustumCorners[i].z) - boundingSphereCenter);
                    boundingSphereRadius = glm::max(boundingSphereRadius, distance);
                }
                boundingSphereRadius = std::ceil(boundingSphereRadius); // Use the ceilling function to increase stability.

                Vector4f boundingSphere = Vector4f(boundingSphereCenter.x, boundingSphereCenter.y, boundingSphereCenter.z, boundingSphereRadius);
#else
                Vector4f viewSpaceBoundingSphere = ComputeViewSpaceShadowCascadeMinimumBoundingSphere(cascadeStartDistance, cascadeEndDistance, tanHalfVerticalFOV, aspectRatio);
                float boundingSphereRadius = std::ceil(viewSpaceBoundingSphere.w); // Use the ceilling function to increase stability.

                Vector4f viewSpaceBoundingSphereCenter = Vector4f(viewSpaceBoundingSphere.x, viewSpaceBoundingSphere.y, viewSpaceBoundingSphere.z, 1.0f);

                Vector4f boundingSphere = inverseViewMatrix * viewSpaceBoundingSphereCenter;
                boundingSphere.w = boundingSphereRadius;

                Vector3f boundingSphereCenter = Vector3f(boundingSphere.x, boundingSphere.y, boundingSphere.z);
#endif
                float minZ = -100.0f;//-boundingSphereRadius;
                float maxZ = boundingSphereRadius;

                Matrix4x4f viewMatrix = glm::lookAt(boundingSphereCenter, boundingSphereCenter + lightDirection, Vector3f(0.0f, 1.0f, 0.0f));
                Matrix4x4f projectionMatrix = Math::OrthographicProjection_ReverseZ_ZO(-boundingSphereRadius, boundingSphereRadius, -boundingSphereRadius, boundingSphereRadius, minZ, maxZ);

                renderer->DrawSphere(boundingSphereCenter, boundingSphereRadius, Vector4f(1.0f, 0.0f, 0.0f, 1.0f));

                Vector3f fff = previewSceneView.cameraPosition + cascadeEndDistance * previewSceneView.cameraForwardVector;
                renderer->DrawLine(previewSceneView.cameraPosition + float(cascadeIndex) * previewSceneView.cameraRightVector, fff + float(cascadeIndex) * previewSceneView.cameraRightVector, Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
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