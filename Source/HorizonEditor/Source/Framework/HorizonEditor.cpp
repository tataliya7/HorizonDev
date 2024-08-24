#include "HorizonEditor.h"

#include "USDModule.h"
#include "RenderDocPlugin.h"

#include <optick.h>

#include "Engine/Components/SkyLightComponent.h"


#include "TextureImporter.h"

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
        WindowCreateInfo windowInfo = {
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
//#if HE_ENBALE_STREAMLINE_SUPPORT
//        streamlineContext = new Streamline::StreamlineContext();
//#endif
//
        RenderDocPluginInit();
//

        InitializeEngine();

        engine = HorizonEngine::GetInstance();
        RenderSystem* renderSystem = engine->GetSubsystem<RenderSystem>();
        renderBackend = renderSystem->GetRenderBackend();
        RenderGraphResourcePool* renderGraphResourcePool = renderSystem->GetRenderGraphResourcePool();

        InitializeImGuiContext();

        std::string usdPluginsPath = executablePath.append("usd").string();
        USDInit(usdPluginsPath);

        RenderBackendSwapChainDesc swapChainDesc = {
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
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        targetTexture = renderGraphResourcePool->AllocateTexture(targetTextureDesc, "SceneViewTexture");

        const uint32 environmentMapTextureSize = 128;
        const uint32 environmentMapTextureMipLevelCount = Math::MaxNumMipLevels(environmentMapTextureSize);
        RenderBackendTextureHandle environmentMapTextureLatLong = LoadTextureFromHDRFile(renderBackend, "../../../Assets/HDRIs/HDR_029_Sky_Cloudy_Ref.hdr");
        RenderBackendTextureDesc environmentMapTextureDesc = RenderBackendTextureDesc::CreateCube(
            environmentMapTextureSize,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            environmentMapTextureMipLevelCount);
        RenderBackendTextureHandle environmentMapTexture = renderBackend->CreateTexture(&environmentMapTextureDesc, nullptr, "EnvironmentMapTexture");

        RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);
        RenderBackendBarrier transitions[] =
        {
            RenderBackendBarrier(targetTexture->GetHandle(), RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::Undefined, RenderBackendResourceState::ShaderResource),
        };
        commandList->Transitions(transitions, 1);

        ConvertLatLongToCubemap(renderBackend, renderSystem->GetShaderLibrary(), *commandList, environmentMapTextureLatLong, environmentMapTexture, environmentMapTextureSize);
        GenerateCubemapMips(renderBackend, renderSystem->GetShaderLibrary(), *commandList, environmentMapTexture, environmentMapTextureMipLevelCount);

        renderBackend->SubmitCommandLists(&commandList, 1, RenderBackendSwapChainHandle::Null);

        renderer = renderSystem->CreateRenderer();

        editorSceneManager = new EditorSceneManager();
        Scene* scene = editorSceneManager->CreateScene("DefaultScene");
        {
            editorSceneManager->SetActiveScene(scene);

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
                lightComponent.castShadows = true;
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
                lightComponent.castShadows = true;
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
                lightComponent.castShadows = true;
                lightComponent.CreateRenderObject(scene->GetRenderScene());
            }

            EntityHandle skyDome = scene->CreateEntity("SkyDome");
            {
                SkyLightComponent& skyLightComponent = scene->GetEntityManager()->AddComponent<SkyLightComponent>(skyDome);
                skyLightComponent.environmentMapTexture = RenderGraphPersistentTexture("EnvironmentMapTexture", environmentMapTextureDesc, environmentMapTexture);
                skyLightComponent.CreateRenderObject(scene->GetRenderScene());
            }

            EntityHandle skyAtmosphere = scene->CreateEntity("SkyAtmosphere");
            {
                SkyAtmosphereComponent& skyAtmosphereComponent = scene->GetEntityManager()->AddComponent<SkyAtmosphereComponent>(skyAtmosphere);
                skyAtmosphereComponent.CreateRenderObject(scene->GetRenderScene());
            }

            EntityHandle localVolumetricFog = scene->CreateEntity("LocalVolumetricFog");
            {
                LocalVolumetricFogComponent& localVolumetricFogComponent = scene->GetEntityManager()->AddComponent<LocalVolumetricFogComponent>(localVolumetricFog);
                localVolumetricFogComponent.CreateRenderObject(scene->GetRenderScene());
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
        editorCamera.fieldOfView = 60.0f;
        //editorCamera.aspectRatio = (float)swapChainWidth / (float)swapChainHeight;
        editorCamera.aspectRatio = 16.0f / 9.0f;
        editorCamera.nearClippingPlane = 0.1f;
        //editorCamera.farClippingPlane = std::numeric_limits<float>::max();
        editorCamera.farClippingPlane = 100.0f;
        editorCamera.cameraSpeed = 1.0f;
        editorCamera.overrideAspectRatio = false;

        renderSettings.indirectLightingIntensity = 1000.0f;
//
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

//#if HE_ENBALE_STREAMLINE_SUPPORT
//        streamlineContext->ReflexSetMarkerSimulationStart();
//#endif
//
//        OnUpdate(deltaTime);
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//        streamlineContext->ReflexSetMarkerSimulationEnd();
//#endif
//
//        GRenderBackend->Tick();
//
//        renderEngine->BeginDrawUI();
//
        engine->GetSubsystem<RenderSystem>()->BeginDrawUI(imguiContext);

        OnDrawUI();

        engine->GetSubsystem<RenderSystem>()->EndDrawUI();

        editorCamera.Update(deltaTimeInSeconds);

        engine->Tick(deltaTimeInSeconds);

        editorSceneManager->GetActiveScene()->Tick(deltaTimeInSeconds);

        //RenderBackendCommandList* commandList = renderBackend->AllocateCommandList();
        //
        // commandList->BeginDebugLabel();
        // commandList->BeginTimingQuery();
        //
        // RenderBackendRenderPassInfo renderPass = {
        //     .renderTargets = { {.texture = output, .mipLevel = 0, .arrayLayer = 0, .loadOp = RenderBackendRenderPassBeginningAccessType::Clear, .storeOp = RenderBackendRenderPassEndingAccessType::Preserve } },
        // };
        // commandList->BeginRenderPass(renderPass);
        //
        // commandList->EndRenderPass();
        //
        // commandList->EndTimingQuery();
        // commandList->EndDebugLabel();

        Quaternion cameraOrientation = Math::QuaternionFromEulerAngles(Math::DegreesToRadians(editorCamera.GetRotation()));

        Vector3 cameraRightVector   = Math::Normalize(cameraOrientation * Vector3(1.0f, 0.0f, 0.0f));
        Vector3 cameraForwardVector = Math::Normalize(cameraOrientation * Vector3(0.0f, 1.0f, 0.0f));
        Vector3 cameraUpVector      = Math::Normalize(cameraOrientation * Vector3(0.0f, 0.0f, 1.0f));

        SceneView sceneView;
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
        sceneView.fieldOfView = editorCamera.fieldOfView;
        sceneView.aspectRatio = editorCamera.aspectRatio;
        sceneView.nearClippingPlane = editorCamera.nearClippingPlane;
        sceneView.farClippingPlane = editorCamera.farClippingPlane;
        sceneView.backgroundColor = Vector3(0.0f, 0.0f, 0.0f);
        sceneView.targetWidth = swapChainWidth;
        sceneView.targetHeight = swapChainHeight;
        sceneView.targetTexture = targetTexture;
        sceneView.displayWidth = swapChainWidth;
        sceneView.displayHeight = swapChainHeight;

        sceneView.transformations.Update(sceneView.cameraPosition, sceneView.cameraRotation, sceneView.fieldOfView, sceneView.aspectRatio, sceneView.nearClippingPlane, sceneView.farClippingPlane);

        viewMatrix_deprecated = sceneView.transformations.worldToViewMatrix;
        projectionMatrix_deprecated = sceneView.transformations.viewToClipMatrix;

        engine->GetSubsystem<RenderSystem>()->RenderSceneView(renderer, &sceneView);

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

//
//        renderEngine->EndDrawUI();
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//        streamlineContext->ReflexSetMarkerRenderSubmitStart();
//#endif
//
//        OnRender(deltaTime);
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//        streamlineContext->ReflexSetMarkerRenderSubmitEnd();
//#endif

        //
        //#if HE_ENBALE_STREAMLINE_SUPPORT
        //            streamlineContext->ReflexSetMarkerPresentStart();
        //#endif
        //
        renderBackend->PresentSwapChain(swapChain);
        //
        //#if HE_ENBALE_STREAMLINE_SUPPORT
        //            streamlineContext->ReflexSetMarkerPresentEnd();
        //#endif
        //
        //            GArena->Reset();
        //
        //            frameCounter++;

        frameIndex++;
    }

    int HorizonEditor::Run()
    {
        while (!IsExitRequested())
        {
            OPTICK_FRAME("MainThread");
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//            streamlineContext->GetNewFrameToken();
//            streamlineContext->ReflexSleep();
//            streamlineContext->ReflexSetMarkerInputSample();
//#endif
            window->ProcessEvents();
//
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