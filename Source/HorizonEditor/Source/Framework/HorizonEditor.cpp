#include "HorizonEditor.h"

#include <optick.h>

#define BIND_FUNCTION(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)> (args)...); }

namespace Horizon
{
    HorizonEditor* HorizonEditor::Instance = nullptr;

    HorizonEditor::HorizonEditor()
    {
        assert(!Instance);
        Instance = this;

        applicationName = HORIZON_EDITOR_APPLICATION_NAME;
    }

    HorizonEditor::~HorizonEditor()
    {
        assert(Instance);
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

        //window->keyPressEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyPressedEvent);
        //window->keyReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyReleasedEvent);
        //window->mouseButtonPressEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonPressedEvent);
        //window->mouseButtonReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonReleasedEvent);
//
//        Input::SetCurrentContext(window->GetGLFWHandle());
//
//        PhysXInit();
//        Audio::AudioEngineInit();
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//        streamlineContext = new Streamline::StreamlineContext();
//#endif
//
//        RenderDocPluginInit();
//

        // Initialize render backend
        {
            bool enableDebugLayers = true;
            bool enableHardwareRayTracing = false;

            if (renderBackendType == RenderBackendType::Vulkan)
            {
                int flags = VULKAN_RENDER_BACKEND_CREATE_FLAGS_SURFACE;
                if (enableDebugLayers)
                {
                    flags |= VULKAN_RENDER_BACKEND_CREATE_FLAGS_VALIDATION_LAYERS;
                }
                if (enableHardwareRayTracing)
                {
                    flags |= VULKAN_RENDER_BACKEND_CREATE_FLAGS_RAY_TRACING;
                }
                renderBackend = RenderBackendCreateVulkan(flags);
            }
            else if (renderBackendType == RenderBackendType::D3D12)
            {
                D3D12RenderBackendDesc d3d12RenderBackendDesc = {
                    .useDebugLayers = enableDebugLayers,
                    .useGPUBasedValidation = enableDebugLayers,
                };
                renderBackend = RenderBackendCreateD3D12(&d3d12RenderBackendDesc);
            }
            else
            {
                LogError(GLogger, std::format("Unknown RenderBackendType!"));
            }

            uint32 primaryDeviceMask = 0;
            uint32 physicalDeviceID = 0;
            renderBackend->CreateRenderDevices(&physicalDeviceID, 1, &primaryDeviceMask);
        }
//
//        renderEngine = new RenderSystem();
//        renderEngine->hardwareRayTracingEnabled = enableHardwareRayTracing;
//        renderEngine->Init(window->GetGLFWHandle());
//
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

        shaderLibrary = new ShaderLibrary();
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
        if (renderBackendType == RenderBackendType::Vulkan)
        {
            RenderBackendDestroyVulkan(renderBackend);
        }
        else if (renderBackendType == RenderBackendType::D3D12)
        {
            RenderBackendDestroyD3D12(renderBackend);
        }

        if (window) delete window;

        GLFWExit();
    }

    void HorizonEditor::Tick()
    {
        OPTICK_EVENT();
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
        OnDrawUI();

        editorCamera.Update(deltaTimeInSeconds);

        sceneView->SetRenderSettings(renderSettings);

        shaderLibrary->HotReload();

        // RenderBackendCommandList* commandList = renderBackend->AllocateCommandList();
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

            WindowState state = window->GetState();

            if (state == WindowState::Minimized)
            {
                continue;
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

            Tick();
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//            streamlineContext->ReflexSetMarkerPresentStart();
//#endif
//
            //renderBackend->PresentSwapChain(swapChain);
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//            streamlineContext->ReflexSetMarkerPresentEnd();
//#endif
//
//            GArena->Reset();
//
//            frameCounter++;
        }

        return 0;
    }

    //void HorizonEditor::OnUpdate(float deltaTime)
    //{
        //OPTICK_EVENT();

        //scene->Update(deltaTime);
    //}
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