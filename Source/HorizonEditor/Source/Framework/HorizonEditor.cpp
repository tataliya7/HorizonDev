#include "HorizonEditor.h"

#include "USDModule.h"
#include "RenderDocPlugin.h"
#include "TimeOfDayPlugin.h"

#include <thread>

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

        RenderDocPluginInit();

        // Initialize logging system
        CreateConsoleLogger_Deprecated();

//
//        JobSystemInit(HE::GetNumberOfProcessors(), HE_JOB_SYSTEM_NUM_FIBIERS, HE_JOB_SYSTEM_FIBER_STACK_SIZE);
//
        WindowSystemInit();

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

        Input::SetCurrentContext(window);

        //window->keyPressEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyPressedEvent);
        //window->keyReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyReleasedEvent);
        //window->mouseButtonPressEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonPressedEvent);
        //window->mouseButtonReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonReleasedEvent);

//        PhysXInit();
//        Audio::AudioEngineInit();

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

        JobSystemInit(16);

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

        // RenderBackendTextureFormat targetTextureFormat = RenderBackendTextureFormat::R10G10B10A2Unorm;
        // RenderGraphTextureDescription targetTextureDesc = RenderGraphTextureDescription::Create2D(
        //     swapChainWidth,
        //     swapChainHeight,
        //     targetTextureFormat,
        //     RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
        //     RenderBackendTextureClearValue::Black,
        //     1,
        //     1,
        //     RenderBackendResourceState::ShaderResource); // TODO: handle transition
        // targetTexture = renderGraphResourcePool->AllocateTexture(targetTextureDesc, "SceneViewTexture");
        // displayTexture = renderGraphResourcePool->AllocateTexture(targetTextureDesc, "DisplayTexture");

        // RenderGraphTextureDescription previewTextureDesc = RenderGraphTextureDescription::Create2D(
        //     previewTextureWidth,
        //     previewTextureHeight,
        //     targetTextureFormat,
        //     RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
        //     RenderBackendTextureClearValue::Black,
        //     1,
        //     1,
        //     RenderBackendResourceState::ShaderResource); // TODO: handle transition
        // previewTexture = renderGraphResourcePool->AllocateTexture(previewTextureDesc, "SceneViewPreviewTexture");

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

        renderer = new RasterizationRenderer(renderBackend, renderGraphResourcePool, renderSystem->shaderRepository, renderSystem->rendererDefaultResources);
        previewRenderer = new RasterizationRenderer(renderBackend, renderGraphResourcePool, renderSystem->shaderRepository, renderSystem->rendererDefaultResources);
        renderSettings.renderMode = RenderMode::RasterRendering;
        renderSettings.rasterRenderingSettings.debugVisualizationMode = RasterizationRendererDebugVisualizationMode::Lighting;
        renderSettings.rasterRenderingSettings.globalIlluminationSettings.indirectLightingIntensity = 1.0f;
        renderSettings.rasterRenderingSettings.shadowsTechnique = RasterizationRendererShadowsTechnique::VirtualShadowMaps;
        renderSettings.rasterRenderingSettings.ambientOcclusionTechnique = RasterizationRendererAmbientOcclusionTechnique::GroundTruthAmbientOcclusion;
        renderSettings.rasterRenderingSettings.reflectionsTechnique = RasterizationRendererReflectionsTechnique::ScreenSpaceReflections;
        renderSettings.rasterRenderingSettings.superSamplingSettings.superSamplingTechnique = SuperSamplingTechnique::FSR2;
        renderSettings.rasterRenderingSettings.superSamplingSettings.qualityMode = 5;
        renderSettings.rasterRenderingSettings.superSamplingSettings.desiredRenderResolutionPercentage = 1.0f;
        renderSettings.rasterRenderingSettings.postProcessingSettings.localToneMappingMethod = LocalToneMappingMethod::BilateralGrid;
        renderSettings.rasterRenderingSettings.postProcessingSettings.bilateralGridLocalToneMappingShadows = 0.8f;

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

        WindowSystemExit();

        JobSystemExit();
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

        engine->Tick(deltaTimeInSeconds);

        timeOfDayScheduler->Tick(deltaTimeInSeconds); // TODO
        editorSceneManager->GetActiveScene()->Tick(deltaTimeInSeconds);

        Quaternion cameraOrientation = Math::QuaternionFromEulerAngles(Math::DegreesToRadians(editorCamera.GetRotation()));

        Vector3f cameraRightVector   = Math::Normalize(cameraOrientation * Vector3f(1.0f, 0.0f, 0.0f));
        Vector3f cameraForwardVector = Math::Normalize(cameraOrientation * Vector3f(0.0f, 1.0f, 0.0f));
        Vector3f cameraUpVector      = Math::Normalize(cameraOrientation * Vector3f(0.0f, 0.0f, 1.0f));

        RenderScene* renderScene = editorSceneManager->GetActiveScene()->GetRenderScene();

        float verticalFOV = editorCamera.fieldOfView;
        if (editorCamera.fieldOfViewAxis == FieldOfViewAxis::Horizontal)
        {
            verticalFOV = HorizontalFOVToVerticalFOV(editorCamera.fieldOfView, editorCamera.aspectRatio);
        }

        uint32 targetWidth = static_cast<uint32>(viewportSize.x);
        uint32 targetHeight = static_cast<uint32>(viewportSize.y);

        uint32 displayWidth = swapChainWidth;
        uint32 displayHeight = swapChainHeight;

        if  (targetTexture == nullptr || displayTexture == nullptr)
        {
            RenderSystem* renderSystem = engine->GetSubsystem<RenderSystem>();
            RenderGraphResourcePool* renderGraphResourcePool = renderSystem->GetRenderGraphResourcePool();

            RenderBackendTextureFormat targetTextureFormat = RenderBackendTextureFormat::R10G10B10A2Unorm;
            RenderGraphTextureDescription displayTextureDescription = RenderGraphTextureDescription::Create2D(
                displayWidth,
                displayHeight,
                targetTextureFormat,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
                RenderBackendTextureClearValue::Black,
                1,
                1,
                RenderBackendResourceState::ShaderResource); // TODO: handle transition

            displayTexture = renderGraphResourcePool->AllocateTexture(displayTextureDescription, "DisplayTexture");

            //renderBackend->ResizeTexture();
        }

        SceneViewDescription sceneViewDescription = {};
        sceneViewDescription.scene = renderScene;
        sceneViewDescription.frameIndex = frameIndex;
        sceneViewDescription.deltaTimeInSeconds = deltaTimeInSeconds;
        sceneViewDescription.renderSettings = renderSettings;
        sceneViewDescription.reset = false;
        sceneViewDescription.cameraPosition = editorCamera.GetPosition();
        sceneViewDescription.cameraRotation = editorCamera.GetRotation();
        sceneViewDescription.cameraUpVector = cameraUpVector;
        sceneViewDescription.cameraRightVector = cameraRightVector;
        sceneViewDescription.cameraForwardVector = cameraForwardVector;
        sceneViewDescription.verticalFOV = verticalFOV;
        sceneViewDescription.aspectRatio = editorCamera.aspectRatio;
        sceneViewDescription.nearClippingPlane = std::max(editorCamera.nearClippingPlane, NearClippingPlaneMinDistance);
        sceneViewDescription.farClippingPlane = editorCamera.farClippingPlane;
        sceneViewDescription.backgroundColor = Vector3f(0.0f, 0.0f, 0.0f);
        sceneViewDescription.targetWidth = targetWidth;
        sceneViewDescription.targetHeight = targetHeight;
        sceneViewDescription.targetTexture = targetTexture;
        sceneViewDescription.displayWidth = displayWidth;
        sceneViewDescription.displayHeight = displayHeight;
        sceneViewDescription.displayTexture = displayTexture;

        SceneView sceneView(sceneViewDescription);

        if (currentRenderMode != renderSettings.renderMode)
        {
            RenderSystem* renderSystem = engine->GetSubsystem<RenderSystem>();
            renderer = renderSystem->CreateSceneRenderer(&sceneView);
            previewRenderer = renderSystem->CreateSceneRenderer(&sceneView);
            currentRenderMode = renderSettings.renderMode;
        }

        renderer->Tick(deltaTimeInSeconds);

        viewMatrix_deprecated = sceneView.GetWorldToViewMatrix();
        projectionMatrix_deprecated = sceneView.GetViewToClipMatrix();

        RenderSystem* renderSystem = engine->GetSubsystem<RenderSystem>();
        RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);
        RenderGraph renderGraph(GArena, renderSystem->renderGraphResourcePool, nullptr);

        // @todo
        if (Input::GetKeyDown(KeyCode::F8))
        {
            renderSystem->shaderRepository->shouldRecompileShaders = true;
        }
        else
        {
            renderSystem->shaderRepository->shouldRecompileShaders = false;
        }

        renderSystem->UpdateImGuiData(commandList);
        renderScene->UpdateGPUScene(renderGraph);

        renderGraph.Execute(*commandList);

        renderBackend->SubmitCommandLists(&commandList, 1, RenderBackendSwapChainHandle::Null);
        delete commandList;

        renderSystem->RenderSceneView(renderer, &sceneView);

        {
            RenderBackendCommandList* commandList2 = new RenderBackendCommandList(GArena);

            RenderBackendTextureHandle swapChainTexture = renderBackend->GetActiveSwapChainBuffer(swapChain);

            if (true)
            {
                {
                    RenderBackendBarrier transitions[] =
                    {
                        RenderBackendBarrier(displayTexture->GetHandle(), RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::CopySrc),
                        RenderBackendBarrier(swapChainTexture, RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                    };
                    commandList2->Barriers(transitions, 2);
                }

                commandList2->CopyTexture2D(
                    displayTexture->GetHandle(),
                    Offset2D(0, 0),
                    0,
                    swapChainTexture,
                    Offset2D(0, 0),
                    0,
                    Extent2D(swapChainWidth, swapChainHeight));

                {
                    RenderBackendBarrier transitions[] =
                    {
                        RenderBackendBarrier(displayTexture->GetHandle(), RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::CopySrc, RenderBackendResourceState::ShaderResource),
                        RenderBackendBarrier(swapChainTexture, RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::CopyDst, RenderBackendResourceState::Present)
                    };
                    commandList2->Barriers(transitions, 2);
                }
            }

            renderBackend->SubmitCommandLists(&commandList2, 1, swapChain);

            delete commandList2;
        }

        renderBackend->PresentSwapChain(swapChain);

        frameIndex++;
    }

    int HorizonEditor::Run()
    {
        while (!IsExitRequested())
        {
            OPTICK_FRAME("MainThread");

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