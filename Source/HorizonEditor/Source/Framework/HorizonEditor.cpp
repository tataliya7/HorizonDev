#include "HorizonEditor.h"

#include "RenderDocPlugin.h"

#include <optick.h>

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

    bool HorizonEditor::Init()
    {
//        // Initialize path to executable
//        executablePath = argv[0];
//        executableDirectory = executablePath.parent_path();
//
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

        RenderBackendSwapChainDesc swapChainDesc = {
            .width = window->GetWidth(),
            .height = window->GetHeight(),
            .windowHandle = (uint64)window->GetNativeHandle(),
            .numBuffers = 3,
            .vsync = false,
            .format = RenderBackendTextureFormat::RGB10A2Unorm,
            .presentMode = RenderBackendSwapChainPresentMode::Immediate,
        };
        swapChain = renderBackend->CreateSwapChain(&swapChainDesc);
        swapChainWidth = window->GetWidth();
        swapChainHeight = window->GetHeight();

        RenderBackendTextureFormat targetTextureFormat = RenderBackendTextureFormat::RGB10A2Unorm;
        RenderGraphTextureDesc targetTextureDesc = RenderGraphTextureDesc::Create2D(
            swapChainWidth,
            swapChainHeight,
            targetTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        targetTexture = renderGraphResourcePool->AllocateTexture(targetTextureDesc, "SceneViewTexture");

        RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);
        RenderBackendBarrier transitions[] =
        {
            RenderBackendBarrier(targetTexture->GetHandle(), RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::Undefined, RenderBackendResourceState::ShaderResource),
        };
        commandList->Transitions(transitions, 1);
        renderBackend->SubmitCommandLists(&commandList, 1, RenderBackendSwapChainHandle::Null);

        renderer = new RealTimeRenderer(renderBackend, renderGraphResourcePool, shaderLibrary, rendererDefaultResources);

        editorSceneManager = new EditorSceneManager();
        {
            Scene* scene = editorSceneManager->CreateScene("DefaultScene");
            editorSceneManager->SetActiveScene(scene);

            RenderScene* renderScene = scene->GetRenderScene();

            DistantLightRenderProxy* distantLight = new DistantLightRenderProxy();
            distantLight->usedAsAtmosphericLight = true;
            renderScene->AddLight(distantLight);

            SkyAtmosphereRenderProxy* skyAtmosphere = new SkyAtmosphereRenderProxy();
            renderScene->AddSkyAtmosphere(skyAtmosphere);
        }

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

    void HorizonEditor::Tick()
    {
        OPTICK_EVENT();

        WindowState state = window->GetState();

        if (state == WindowState::Minimized)
        {
            return;
        }

        //
        //            //if (!window->IsFocused())
        //            //{
        //            //    OSSuspendCurrentThread(0.05f);
        //            //}
        //
        //            //static std::chrono::steady_clock::time_point previousTimePoint1{ std::chrono::steady_clock::now() };
        //            //std::chrono::steady_clock::time_point timePoint = std::chrono::steady_clock::now();
        //            //std::chrono::duration<float> timeDuration = std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - previousTimePoint1);
        //            //float deltaTimeT = timeDuration.count();
        //            ////if (deltaTimeT < 0.033f)
        //            //if (deltaTimeT < 0.01666667f)
        //            ////if (deltaTimeT < 0.01111111f)
        //            //{
        //            //    continue;
        //            //}
        //            //previousTimePoint1 = timePoint;
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

//
//        deltaTime = CalculateDeltaTime();
//
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
        //OnDrawUI();

        editorCamera.Update(deltaTimeInSeconds);

        engine->Tick(deltaTimeInSeconds);

        //RenderBackendCommandList* commandList = renderBackend->AllocateCommandList();
        RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);
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

        Vector3 cameraRightVector   = Math::Normalize(editorCamera.GetRotation() * Vector3(1.0f, 0.0f, 0.0f));
        Vector3 cameraForwardVector = Math::Normalize(editorCamera.GetRotation() * Vector3(0.0f, 1.0f, 0.0f));
        Vector3 cameraUpVector      = Math::Normalize(editorCamera.GetRotation() * Vector3(0.0f, 0.0f, 1.0f));

        SceneView sceneView;
        sceneView.scene = editorSceneManager->GetActiveScene()->GetRenderScene();
        sceneView.renderSettings = renderSettings;
        sceneView.debugVisualizationMode = SceneViewDebugVisualizationMode::Lighting;//currentDebugVisualizationMode;
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

        gpuProfiler->BeginFrame(commandList);
        uint32 frameTimingQueryRegion = gpuProfiler->BeginRegion(commandList, "GPU Frametime");

        renderer->OnRenderBegin(&sceneView);

        RenderGraph renderGraph(GArena, renderGraphResourcePool, gpuProfiler);

        renderer->Render(renderGraph);

        renderGraph.Execute(*commandList);

        gpuProfiler->EndRegion(frameTimingQueryRegion, commandList);
        gpuProfiler->EndFrame(commandList);

        //renderer->OnRenderEnd();

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

int HorizonEditorMain()
{
    int exitCode = EXIT_SUCCESS;
    Horizon::HorizonEditor* editor = new Horizon::HorizonEditor();
    bool result = editor->Init();
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