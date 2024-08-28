#include "HorizonEditor.h"
#include "FirstPersonCameraController.h"
#include "Streamline.h"

#include "Framework/FileBrowserWindow.h"
#include "Framework/SceneViewportWindow.h"

#include "Plugins/USD/USD.h"
#include "Plugins/RenderDoc/RenderDocPlugin.h"

#define SPDLOG_USE_STD_FORMAT
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#define HE_JOB_SYSTEM_NUM_FIBIERS 128
#define HE_JOB_SYSTEM_FIBER_STACK_SIZE (HE_JOB_SYSTEM_NUM_FIBIERS * 1024)

namespace Horizon
{
    HorizonEditor* HorizonEditor::Instance = nullptr;

    bool CreateConsoleLogger();

    HorizonEditor::HorizonEditor()
        : applicationName("Horizon Editor")
    {
        ASSERT(!Instance);
        Instance = this;
        gizmoOperationType = ImGuizmo::OPERATION::TRANSLATE;
    }

    HorizonEditor::~HorizonEditor()
    {
        ASSERT(Instance);
        Instance = nullptr;
    }

    bool HorizonEditor::Init(int argc, char** argv)
    {
        // Initialize path to executable
        executablePath = argv[0];
        executableDirectory = executablePath.parent_path();

        // Initialize logging
        CreateConsoleLogger_Deprecated();
        //CreateConsoleLogger();

        JobSystemInit(HE::GetNumberOfProcessors(), HE_JOB_SYSTEM_NUM_FIBIERS, HE_JOB_SYSTEM_FIBER_STACK_SIZE);

        GLFWInit();

        //WindowCreateFlags windowFlags = WindowCreateFlags(HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_RESIZABLE);
        WindowCreateFlags windowFlags = WindowCreateFlags(HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_RESIZABLE | HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_MAXIMIZED);

        // Create main window
        WindowCreateInfo windowInfo = {
            .width = initialWidth,
            .height = initialHeight,
            .title = applicationName.c_str(),
            .icon = "../../../Assets/Icons/horizon.png",
            .flags = windowFlags
        };
        window = new Window(&windowInfo);
        window->keyPressEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyPressedEvent);
        window->keyReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnKeyReleasedEvent);
        window->mouseButtonPressEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonPressedEvent);
        window->mouseButtonReleaseEventCallback = BIND_FUNCTION(HorizonEditor::OnMouseButtonReleasedEvent);

        Input::SetCurrentContext(window->GetGLFWHandle());

        PhysXInit();
        Audio::AudioEngineInit();

#if HE_ENBALE_STREAMLINE_SUPPORT
        streamlineContext = new Streamline::StreamlineContext();
#endif

        RenderDocPluginInit();

        bool enableHardwareRayTracing = false;

        // Initialize render backend
        {
            RenderBackendType renderBackendType = RenderBackendType::Vulkan;
            if (renderBackendType == RenderBackendType::Vulkan)
            {
#if HE_ENBALE_STREAMLINE_SUPPORT
                int flags = VULKAN_RENDER_BACKEND_CREATE_FLAGS_SURFACE; // Disable validation layers when using NSight and Reflex.
#else
                int flags = VULKAN_RENDER_BACKEND_CREATE_FLAGS_VALIDATION_LAYERS | VULKAN_RENDER_BACKEND_CREATE_FLAGS_SURFACE;

                if (enableHardwareRayTracing)
                {
                    flags |= VULKAN_RENDER_BACKEND_CREATE_FLAGS_RAY_TRACING;
                }
#endif
                GRenderBackend = RenderBackendCreateVulkan(flags);
            }
            else if (renderBackendType == RenderBackendType::D3D12)
            {
#if 1
                D3D12RenderBackendDesc d3d12RenderBackendDesc = {
                    .useDebugLayers = false,
                    .useGPUBasedValidation = false,
                };
#else
                D3D12RenderBackendDesc d3d12RenderBackendDesc = {
                    .useDebugLayers = true,
                    .useGPUBasedValidation = true,
                };
#endif
                GRenderBackend = RenderBackendCreateD3D12(&d3d12RenderBackendDesc);
            }
            else
            {
                LogError(GLogger, std::format("Unknown RenderBackendType!"));
            }

            uint32 primaryDeviceMask = 0;
            uint32 physicalDeviceID = 0;
            GRenderBackend->CreateRenderDevices(&physicalDeviceID, 1, &primaryDeviceMask);
        }

        renderEngine = new RenderSystem();
        renderEngine->hardwareRayTracingEnabled = enableHardwareRayTracing;
        renderEngine->Init(window->GetGLFWHandle());

        RenderBackendSwapChainDesc swapChainDesc = {
            .width = window->GetWidth(),
            .height = window->GetHeight(),
            .windowHandle = (uint64)window->GetNativeHandle(),
            .numBuffers = 3,
            .vsync = false,
            .format = RenderBackendTextureFormat::RGB10A2Unorm, //RenderBackendTextureFormat::BGRA8Unorm,
            .presentMode = RenderBackendSwapChainPresentMode::Immediate,
        };
        swapChain = GRenderBackend->CreateSwapChain(~0u, &swapChainDesc);
        swapChainWidth = window->GetWidth();
        swapChainHeight = window->GetHeight();

        ShaderGraphSystemInit();

        selectionManager = new SelectionManager();

        Setup();
        return true;
    }

    void HorizonEditor::Exit()
    {
        Clear();

        renderEngine->Exit();
        delete renderEngine;

        delete window;

        VulkanRenderBackendDestroyBackend(GRenderBackend);

        Audio::AudioEngineExit();
        PhysXExit();

        GLFWExit();
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

    /*EntityHandle HorizonEditor::GetPickingResult()
    {
        Buffer::Create(4, Buffer::Usage::StagingBuffer);

        RenderBackendCommandList commandList;

        commandList.CopyTextureToBuffer();
    }*/

    void HorizonEditor::OnKeyPressedEvent(KeyCode key, bool repeat)
    {
        bool shiftDown = Input::GetKeyDown(KeyCode::LeftShift) || Input::GetKeyDown(KeyCode::RightShift);
        if (key == KeyCode::Tab)
        {
            showConsoleWindow = !showConsoleWindow;
            return;
        }
        if ((key == KeyCode::Enter) && shiftDown)
        {
            window->SetFullscreen(!toggleFullScreen);
            toggleFullScreen = !toggleFullScreen;
        }
        OnKeyPressed(key, repeat);
    }

    void HorizonEditor::OnKeyReleasedEvent(KeyCode key)
    {
        OnKeyReleased(key);
    }

    void HorizonEditor::OnMouseButtonPressedEvent(MouseButtonID id)
    {
        ImGui::ClearActiveID();

        struct SelectionData
        {
            EntityHandle entity;
            float distance;
        };
        std::vector<SelectionData> selectionData;

        ImVec2 mousePos = ImGui::GetMousePos();

        float viewportWidth = viewportPos.z - viewportPos.x;
        float viewportHeight = viewportPos.w - viewportPos.y;

        float mouseX = ((mousePos.x - viewportPos.x) / viewportWidth) * 2.0f - 1.0f;
        float mouseY = -(((mousePos.y - viewportPos.y) / viewportHeight) * 2.0f - 1.0f);

        if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
        {
            //EntityHandle pickedEntity = GetPickingResult();

            bool ctrlDown = Input::GetKeyDown(KeyCode::LeftControl) || Input::GetKeyDown(KeyCode::RightControl);
            bool shiftDown = Input::GetKeyDown(KeyCode::LeftShift) || Input::GetKeyDown(KeyCode::RightShift);
            if (!ctrlDown)
            {
                selectionManager->DeselectAll();
            }

            if (!selectionData.empty())
            {
                EntityHandle entity = selectionData.front().entity;
                if (selectionManager->IsSelected(entity) && ctrlDown)
                {
                    selectionManager->Deselect(entity);
                }
                else
                {
                    selectionManager->Select(entity);
                }
            }
        }
    }

    void HorizonEditor::OnMouseButtonReleasedEvent(MouseButtonID id)
    {

    }

    void HorizonEditor::Tick()
    {
        OPTICK_EVENT();

        deltaTime = CalculateDeltaTime();

#if HE_ENBALE_STREAMLINE_SUPPORT
        streamlineContext->ReflexSetMarkerSimulationStart();
#endif

        OnUpdate(deltaTime);

#if HE_ENBALE_STREAMLINE_SUPPORT
        streamlineContext->ReflexSetMarkerSimulationEnd();
#endif

        GRenderBackend->Tick();

        renderEngine->BeginDrawUI();

        OnDrawUI();

        renderEngine->EndDrawUI();

#if HE_ENBALE_STREAMLINE_SUPPORT
        streamlineContext->ReflexSetMarkerRenderSubmitStart();
#endif

        OnRender(deltaTime);

#if HE_ENBALE_STREAMLINE_SUPPORT
        streamlineContext->ReflexSetMarkerRenderSubmitEnd();
#endif
    }

    int HorizonEditor::Run()
    {
        while (!IsExitRequested())
        {
            OPTICK_FRAME("MainThread");

#if HE_ENBALE_STREAMLINE_SUPPORT
            streamlineContext->GetNewFrameToken();
            streamlineContext->ReflexSleep();
            streamlineContext->ReflexSetMarkerInputSample();
#endif
            window->ProcessEvents();

            if (window->ShouldClose())
            {
                SetExitRequest(true);
            }

            WindowState state = window->GetState();

            if (state == WindowState::Minimized)
            {
                continue;
            }

            //if (!window->IsFocused())
            //{
            //    OSSuspendCurrentThread(0.05f);
            //}

            //static std::chrono::steady_clock::time_point previousTimePoint1{ std::chrono::steady_clock::now() };
            //std::chrono::steady_clock::time_point timePoint = std::chrono::steady_clock::now();
            //std::chrono::duration<float> timeDuration = std::chrono::duration_cast<std::chrono::duration<float>>(timePoint - previousTimePoint1);
            //float deltaTimeT = timeDuration.count();
            ////if (deltaTimeT < 0.033f)
            //if (deltaTimeT < 0.01666667f)
            ////if (deltaTimeT < 0.01111111f)
            //{
            //    continue;
            //}
            //previousTimePoint1 = timePoint;

            uint32 width = window->GetWidth();
            uint32 height = window->GetHeight();
            if (width != swapChainWidth || height != swapChainHeight)
            {
                // Vulkan does not support swap chains with width and height set to zero
                if (width != 0 && height != 0)
                {
                    GRenderBackend->ResizeSwapChain(swapChain, &width, &height);
                    swapChainWidth = width;
                    swapChainHeight = height;
                }
            }

            Tick();

#if HE_ENBALE_STREAMLINE_SUPPORT
            streamlineContext->ReflexSetMarkerPresentStart();
#endif

            GRenderBackend->PresentSwapChain(swapChain);

#if HE_ENBALE_STREAMLINE_SUPPORT
            streamlineContext->ReflexSetMarkerPresentEnd();
#endif

            GArena->Reset();

            frameCounter++;
        }

        return 0;
    }

    void HorizonEditor::Setup()
    {
        fileIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/file.png", false, false);
        directoryIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/folder-512.png", false, false);
        backwardButtonIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/backward-button-32.png", false, false);
        forwardButtonIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/forward-button-32.png", false, false);
        parentButtonIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/parent-button-32.png", false, false);
        searchButtonIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/search-button-32.png", false, false);
        refreshButtonIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/refresh-button-32.png", false, false);

        playButtonIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/play-button-128.png", false, false);
        stopButtonIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/stop-button-128.png", false, false);
        pauseButtonIcon = LoadTextureFromFile(GRenderBackend, "../../../Assets/Icons/pause-button-128.png", false, false);

        // Create scene
        scene = SceneManager::CreateScene("ExampleScene");
        SceneManager::SetActiveScene(scene);

        scene->SetShouldSimulate(true);
        scene->SetShouldUpdateScripts(true);

        // Create main camera
        mainCamera = scene->CreateEntity("MainCamera");
        {
            // When an entity is added to the scene, TransformComponent and SceneHierarchyComponent are automatically created
            auto& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(mainCamera);
            transformComponent.position = Vector3(0.0f, 0.0f, 5.0f);
            transformComponent.rotation = Vector3(0.0f, 0.0f, 0.0f);

            CameraComponent cameraComponent;
            cameraComponent.projectionMode = CameraProjectionMode::Perspective;
            cameraComponent.nearClippingPlane = 0.1f;
            cameraComponent.farClippingPlane = 100.0f;
            cameraComponent.fieldOfView = 60.0f;
            cameraComponent.aspectRatio = 16.0f / 9.0f;
            cameraComponent.overrideAspectRatio = false;
            scene->GetEntityManager()->AddComponent<CameraComponent>(mainCamera, cameraComponent);

            // Attach camera controller to main camera
            scene->GetEntityManager()->AddComponent<ScriptComponent>(mainCamera).Bind<FirstPersonCameraController>();
        }

        EntityHandle testCamera = scene->CreateEntity("TestCamera");
        {
            // When an entity is added to the scene, TransformComponent and SceneHierarchyComponent are automatically created
            auto& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(testCamera);
            transformComponent.position = Vector3(0.0f, 0.0f, 5.0f);
            transformComponent.rotation = Vector3(0.0f, 0.0f, 0.0f);

            CameraComponent cameraComponent;
            cameraComponent.projectionMode = CameraProjectionMode::Perspective;
            cameraComponent.nearClippingPlane = 0.1f;
            cameraComponent.farClippingPlane = 10.0f;
            cameraComponent.fieldOfView = 60.0f;
            cameraComponent.aspectRatio = 16.0f / 9.0f;
            cameraComponent.overrideAspectRatio = false;
            scene->GetEntityManager()->AddComponent<CameraComponent>(testCamera, cameraComponent);
        }

        // Create main light
        EntityHandle directionalLight = scene->CreateEntity("DirectionalLight");
        {
            auto& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(directionalLight);
            //transformComponent.rotation = Vector3(11.0f, -15.0f, 0.0f);
            transformComponent.rotation = Vector3(11.0f, 6.0f, 0.0f);

            LightComponent lightComponent;
            lightComponent.type = LightComponent::LightType::Directional;
            lightComponent.color = Vector4(1.0f);
            lightComponent.luminousIntensity = 120000.0f;
            lightComponent.apexAngle = 0.5357f;
            lightComponent.castShadows = true;
            lightComponent.useColorTemperature = true;
            lightComponent.colorTemperature = 6500.0f;
            scene->GetEntityManager()->AddComponent<LightComponent>(directionalLight, lightComponent);
        }

        // Create point light 0
        EntityHandle pointLight0 = scene->CreateEntity("PointLight0");
        {
            auto& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(pointLight0);
            transformComponent.position = Vector3(9.0f, -3.0f, 1.5f);
            LightComponent lightComponent;
            lightComponent.type = LightComponent::LightType::Point;
            lightComponent.color = Vector4(255.0f / 255.0f, 41.0f / 255.0f, 0.0f / 255.0f, 1.0f);
            lightComponent.luminousIntensity = 500.0f;
            lightComponent.radius = 3.0f;
            lightComponent.castShadows = true;
            scene->GetEntityManager()->AddComponent<LightComponent>(pointLight0, lightComponent);
        }

        // Create point light 1
        EntityHandle pointLight1 = scene->CreateEntity("PointLight1");
        {
            auto& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(pointLight1);
            transformComponent.position = Vector3(-9.5f, 3.5f, 1.5f);
            LightComponent lightComponent;
            lightComponent.type = LightComponent::LightType::Point;
            lightComponent.color = Vector4(0.0f, 7.0f / 255.0f, 255.0f / 255.0f, 1.0f);
            lightComponent.luminousIntensity = 500.0f;
            lightComponent.radius = 3.0f;
            lightComponent.castShadows = true;
            scene->GetEntityManager()->AddComponent<LightComponent>(pointLight1, lightComponent);
        }

        // Create environment light
        EntityHandle skyLight = scene->CreateEntity("EnvironmentLight");
        {
            EnvironmentLightComponent environmentLightComponent;
            environmentLightComponent.cubemapResolution = 1024;
            environmentLightComponent.SetCubemap("../../../Assets/HDRIs/HDR_029_Sky_Cloudy_Ref.hdr");
            scene->GetEntityManager()->AddComponent<EnvironmentLightComponent>(skyLight, environmentLightComponent);
        }

        // Create sky atmosphere
        EntityHandle skyAtmosphere = scene->CreateEntity("SkyAtmosphere");
        {
            SkyAtmosphereComponent skyAtmosphereComponent;
            SetupEarthAtmosphere(&skyAtmosphereComponent);
            scene->GetEntityManager()->AddComponent<SkyAtmosphereComponent>(skyAtmosphere, skyAtmosphereComponent);
        }

        std::string usdPluginsPath = executablePath.append("usd").string();
        USDInit_DEPRECATED(usdPluginsPath);

        // TODO: Test New Sponaza
        USDImportSettings settings = {};
        settings.importMeshes = true;
        settings.importMaterials = true;
        //USDImport("../../../Assets/NewSponza/NewSponza.usdc", &settings, false);
        USDImport("../../../Assets/Test/Sponza/sponza.usdc", &settings, false);

        auto& renderPipelineSettings = ((RenderSystem*)renderEngine)->GetRealTimeRendererSettings_Deprecated();
        renderPipelineSettings.indirectLightingIntensity = 1000.0f;
        renderPipelineSettings.gtaoSettings.radius = 0.5f;
        renderPipelineSettings.gtaoSettings.factor = 3.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureExposureCompensation = 0.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureMinExposureValue = -5.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureMaxExposureValue = 20.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureHistogramLowerPercentage = 0.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureHistogramHigherPercentage = 1.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureHistogramMinEV100 = -10.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureHistogramMaxEV100 = 20.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureSpeedDarkToBright = 3.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureSpeedBrightToDark = 1.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureUseTargetExposure = 0.0f;
        renderPipelineSettings.superResolutionTechnique = SuperResolutionTechnique::FSR2;
        renderPipelineSettings.fsr2Settings.qualityMode = FSR2QualityMode::Balanced;
        renderPipelineSettings.toneMappingOperator = ToneMappingOperatorType::ACES;

        settings.ambientOcclusionTechnique = AmbientOcclusionTechnique::GroundTruthAmbientOcclusion;
        settings.antialiasingTechnique = AntialiasingTechnique::TemporalAA;

        settings.postProcessingSettings.fixedExposureValue = 0.0f;

        settings.postProcessingSettings.dofScale = 0.0f;
        settings.postProcessingSettings.dofFocalDistance = 5.0f;
        settings.postProcessingSettings.dofFocalRegion = 1.0f;
        settings.postProcessingSettings.dofNearTransitionRegion = 0.0f;
        settings.postProcessingSettings.dofFarTransitionRegion = 0.0f;
        settings.postProcessingSettings.dofNearRegionBlurSize = 1.0f;
        settings.postProcessingSettings.dofFarRegionBlurSize = 1.0f;

        settings.postProcessingSettings.localExposureShadows = 1.5f;
        settings.postProcessingSettings.localExposureHighlights = 2.0f;
        settings.postProcessingSettings.localExposureCoarsestMipLevel = 2;
        settings.postProcessingSettings.localExposureDisplayMipLevel = 1;
        settings.postProcessingSettings.localExposurePreferenceSigma = 5.0f;

        settings.postProcessingSettings.bloomIntensity = 1.0f;
        settings.postProcessingSettings.bloomRadius = 0.5f;
        settings.postProcessingSettings.lensDirtIntensity = 0.0f;
        settings.postProcessingSettings.lensDirtTint = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

        settings.postProcessingSettings.lensFlaresIntensity = 0.0f;

        settings.postProcessingSettings.colorGradingWhiteBalanceColorTemperature = 6500.0f;

        settings.postProcessingSettings.chromaticAberrationIntensity = 0.0f;
        settings.postProcessingSettings.chromaticAberrationOffset = 0.0f;

        settings.postProcessingSettings.localExposureEnabled = false;

        settings.postProcessingSettings.colorCorrectionSaturation = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        settings.postProcessingSettings.colorCorrectionContrast = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        settings.postProcessingSettings.colorCorrectionGamma = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        settings.postProcessingSettings.colorCorrectionGain = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        settings.postProcessingSettings.colorCorrectionOffset = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

        ImNodes::CreateContext();

        ImNodesStyle& style = ImNodes::GetStyle();
        //style.Colors[ImNodesCol_TitleBar] = IM_COL32(43, 101, 43, 255);
        //style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(43, 101, 43, 255);
        //style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(43, 101, 43, 255);
        style.Colors[ImNodesCol_NodeBackground] = IM_COL32(65, 65, 65, 255);
        style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(65, 65, 65, 255);
        style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(65, 65, 65, 255);
        style.Colors[ImNodesCol_NodeOutline] = IM_COL32(0, 0, 0, 200);
        style.Colors[ImNodesCol_NodeOutlineHovered] = IM_COL32(0, 0, 0, 200);
        style.Colors[ImNodesCol_NodeOutlineSelected] = IM_COL32(235, 163, 0, 255);

        style.NodeCornerRounding = 10.0f;
        style.NodePadding = ImVec2(20.0f, 8.0f);
        style.NodeBorderThickness = 2.0f;

        style.Colors[ImNodesCol_Pin] = IM_COL32(195, 195, 195, 255);
        style.Colors[ImNodesCol_PinHovered] = IM_COL32(195, 195, 195, 255);

        style.PinCircleRadius = 8.0f;
        style.PinLineThickness = 1.0f;
    }

    void HorizonEditor::Clear()
    {
        SceneManager::DestroyScene(scene);
        ImNodes::DestroyContext();
    }

    void HorizonEditor::OnUpdate(float deltaTime)
    {
        OPTICK_EVENT();

        scene->Update(deltaTime);
    }

    void HorizonEditor::OnRender(float deltaTime)
    {
        OPTICK_EVENT();

        SceneView view;
        view.scene = scene;
        view.renderEngine = renderEngine;
        view.camera = scene->GetEntityManager()->GetComponent<CameraComponent>(mainCamera);
        view.deltaTime = deltaTime;
        view.frameIndex = GetFrameCounter();
        view.targetWidth = swapChainWidth;
        view.targetHeight = swapChainHeight;
        view.swapChain = swapChain;
        view.target = GRenderBackend->GetActiveSwapChainBuffer(swapChain);
        view.targetDesc = RenderBackendTextureDesc::Create2D(swapChainWidth, swapChainHeight, RenderBackendTextureFormat::BGRA8Unorm, RenderBackendTextureCreateFlags::Present);
        view.debugViewMode = viewMode;

        // TODO: calculate projection matrix

        RenderSceneView(&view);
    }
}
