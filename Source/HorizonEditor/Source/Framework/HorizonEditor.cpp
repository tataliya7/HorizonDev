#include "HorizonEditor.h"

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

    bool HorizonEditor::Init(int argc, char** argv)
    {
//        // Initialize path to executable
//        executablePath = argv[0];
//        executableDirectory = executablePath.parent_path();
//
//        // Initialize logging
//        CreateConsoleLogger_Deprecated();
//        //CreateConsoleLogger();
//
//        JobSystemInit(HE::GetNumberOfProcessors(), HE_JOB_SYSTEM_NUM_FIBIERS, HE_JOB_SYSTEM_FIBER_STACK_SIZE);
//
//        GLFWInit();
//
//        //WindowCreateFlags windowFlags = WindowCreateFlags(HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_RESIZABLE);
//        WindowCreateFlags windowFlags = WindowCreateFlags(HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_RESIZABLE | HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_MAXIMIZED);
//
//        // Create main window
//        WindowCreateInfo windowInfo = {
//            .width = initialWidth,
//            .height = initialHeight,
//            .title = applicationName.c_str(),
//            .icon = "../../../Assets/Icons/horizon.png",
//            .flags = windowFlags
//        };
//        window = new Window(&windowInfo);
//        window->keyPressEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyPressedEvent);
//        window->keyReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyReleasedEvent);
//        window->mouseButtonPressEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonPressedEvent);
//        window->mouseButtonReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonReleasedEvent);
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
//        bool enableHardwareRayTracing = false;
//
//        // Initialize render backend
//        {
//            RenderBackendType renderBackendType = RenderBackendType::Vulkan;
//            if (renderBackendType == RenderBackendType::Vulkan)
//            {
//#if HE_ENBALE_STREAMLINE_SUPPORT
//                int flags = VULKAN_RENDER_BACKEND_CREATE_FLAGS_SURFACE; // Disable validation layers when using NSight and Reflex.
//#else
//                int flags = VULKAN_RENDER_BACKEND_CREATE_FLAGS_VALIDATION_LAYERS | VULKAN_RENDER_BACKEND_CREATE_FLAGS_SURFACE;
//
//                if (enableHardwareRayTracing)
//                {
//                    flags |= VULKAN_RENDER_BACKEND_CREATE_FLAGS_RAY_TRACING;
//                }
//#endif
//                GRenderBackend = RenderBackendCreateVulkan(flags);
//            }
//            else if (renderBackendType == RenderBackendType::D3D12)
//            {
//#if 1
//                D3D12RenderBackendDesc d3d12RenderBackendDesc = {
//                    .useDebugLayers = false,
//                    .useGPUBasedValidation = false,
//                };
//#else
//                D3D12RenderBackendDesc d3d12RenderBackendDesc = {
//                    .useDebugLayers = true,
//                    .useGPUBasedValidation = true,
//                };
//#endif
//                GRenderBackend = RenderBackendCreateD3D12(&d3d12RenderBackendDesc);
//            }
//            else
//            {
//                LogError(GLogger, std::format("Unknown RenderBackendType!"));
//            }
//
//            uint32 primaryDeviceMask = 0;
//            uint32 physicalDeviceID = 0;
//            GRenderBackend->CreateRenderDevices(&physicalDeviceID, 1, &primaryDeviceMask);
//        }
//
//        renderEngine = new RenderSystem();
//        renderEngine->hardwareRayTracingEnabled = enableHardwareRayTracing;
//        renderEngine->Init(window->GetGLFWHandle());
//
//        RenderBackendSwapChainDesc swapChainDesc = {
//            .width = window->GetWidth(),
//            .height = window->GetHeight(),
//            .windowHandle = (uint64)window->GetNativeHandle(),
//            .numBuffers = 3,
//            .vsync = false,
//            .format = RenderBackendTextureFormat::RGB10A2Unorm, //RenderBackendTextureFormat::BGRA8Unorm,
//            .presentMode = RenderBackendSwapChainPresentMode::Immediate,
//        };
//        swapChain = GRenderBackend->CreateSwapChain(~0u, &swapChainDesc);
//        swapChainWidth = window->GetWidth();
//        swapChainHeight = window->GetHeight();
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
        
    }

    void HorizonEditor::Tick()
    {
//        OPTICK_EVENT();
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
//        OnDrawUI();
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
//        while (!IsExitRequested())
//        {
//            OPTICK_FRAME("MainThread");
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//            streamlineContext->GetNewFrameToken();
//            streamlineContext->ReflexSleep();
//            streamlineContext->ReflexSetMarkerInputSample();
//#endif
//            window->ProcessEvents();
//
//            if (window->ShouldClose())
//            {
//                SetExitRequest(true);
//            }
//
//            WindowState state = window->GetState();
//
//            if (state == WindowState::Minimized)
//            {
//                continue;
//            }
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
//            uint32 width = window->GetWidth();
//            uint32 height = window->GetHeight();
//            if (width != swapChainWidth || height != swapChainHeight)
//            {
//                // Vulkan does not support swap chains with width and height set to zero
//                if (width != 0 && height != 0)
//                {
//                    GRenderBackend->ResizeSwapChain(swapChain, &width, &height);
//                    swapChainWidth = width;
//                    swapChainHeight = height;
//                }
//            }
//
//            Tick();
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//            streamlineContext->ReflexSetMarkerPresentStart();
//#endif
//
//            GRenderBackend->PresentSwapChain(swapChain);
//
//#if HE_ENBALE_STREAMLINE_SUPPORT
//            streamlineContext->ReflexSetMarkerPresentEnd();
//#endif
//
//            GArena->Reset();
//
//            frameCounter++;
//        }

        return 0;
    }

    //void HorizonEditor::OnUpdate(float deltaTime)
    //{
        //OPTICK_EVENT();

        //scene->Update(deltaTime);
    //}
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