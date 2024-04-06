#include "HorizonEditor.h"
#include "FirstPersonCameraController.h"
#include "Streamline.h"

#include "Framework/FileBrowserWindow.h"
#include "Framework/SceneViewportWindow.h"

#include "Plugins/USD/USD.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <ImGuizmo.h>

#include "imnodes.h"
#include "imnodes_internal.h"

#define SPDLOG_USE_STD_FORMAT
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#define HE_JOB_SYSTEM_NUM_FIBIERS 128
#define HE_JOB_SYSTEM_FIBER_STACK_SIZE (HE_JOB_SYSTEM_NUM_FIBIERS * 1024)

#define BIND_FUNCTION(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args) > (args)...); }

namespace HE
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

        renderEngine = new Renderer();
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
        //USDImport("../../../Assets/Main.1_Sponza/NewSponza_Main_USD_Yup_002.usda", &settings, false);
        USDImport("../../../Assets/Test/Sponza/sponza.usdc", &settings, false);

        auto& renderPipelineSettings = ((Renderer*)renderEngine)->GetRealTimeRendererSettings_Deprecated();
        renderPipelineSettings.indirectLightingIntensity = 1000.0f;
        renderPipelineSettings.gtaoSettings.radius = 0.5f;
        renderPipelineSettings.gtaoSettings.factor = 3.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureExposureCompensation = 0.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureMinExposureValue = -5.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureMaxExposureValue = 20.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureHistogramLowPercent = 0.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureHistogramHighPercent = 1.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureHistogramMinEV100 = -10.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureHistogramMaxEV100 = 20.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureSpeedDarkToBright = 3.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureSpeedBrightToDark = 1.0f;
        renderPipelineSettings.postProcessingSettings.autoExposureUseTargetExposure = 0;
        renderPipelineSettings.superResolutionTechnique = SuperResolutionTechnique::FSR2;
        renderPipelineSettings.fsr2Settings.qualityMode = FSR2QualityMode::Balanced;
        renderPipelineSettings.toneMappingOperator = ToneMappingOperatorType::ACES;

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

    void HorizonEditor::OnDrawUIEx()
    {
        // Draw gizmos
        {
            auto windowPos = ImGui::GetWindowPos();
            //auto viewportSize = ImGui::GetContentRegionAvail();
            Vector2 viewportSize = { swapChainWidth, swapChainHeight };

            if (selectedEntity && gizmoOperationType != -1)
            {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(windowPos.x, windowPos.y, viewportSize.x, viewportSize.y);

                bool snap = Input::GetKeyDown(KeyCode::LeftControl);
                float snapValue = GetSnapValue();
                float snapValues[3] = { snapValue, snapValue, snapValue };

                // Editor camera
                CameraComponent editorCamera = SceneManager::GetActiveScene()->GetEntityManager()->GetComponent<CameraComponent>(mainCamera);
                Matrix4x4 cameraProjection = editorCamera.projectionMatrix;
                Matrix4x4 cameraView = editorCamera.viewMatrix;

                // Entity transform
                static Matrix4x4 transformMatrix = Matrix4x4(1.0f);
                auto& transformComponent = SceneManager::GetActiveScene()->GetEntityManager()->GetComponent<TransformComponent>(selectedEntity);
                transformMatrix = transformComponent.matrix;

                //float deltaMatrix[16];
                ImGuizmo::Manipulate(glm::value_ptr(cameraView),
                    glm::value_ptr(cameraProjection),
                    (ImGuizmo::OPERATION)gizmoOperationType,
                    ImGuizmo::LOCAL,
                    glm::value_ptr(transformMatrix),
                    nullptr,
                    snap ? snapValues : nullptr);

                /*static Quaternion zUpQuat = glm::rotate(glm::quat(), Math::DegreesToRadians(90.0), Vector3(1.0, 0.0, 0.0));
                static Matrix4x4 preTransform = Math::Compose(Vector3(0.0f, 0.0f, 0.0f), zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
                ImGuizmo::DrawGrid(glm::value_ptr(cameraView),
                    glm::value_ptr(cameraProjection),
                    glm::value_ptr(preTransform),
                    10.0f);*/

                    /*if (ImGuizmo::IsUsing())
                    {
                        auto parent = selectedEntity->GetCreator()->GetEntityByHandle(selectedEntity->GetComponent<SceneHierarchyComponent>().parent);
                        if (parent)
                        {
                            transformMatrix = glm::inverse(parent->GetComponent<TransformComponent>().localToWorldMatrix) * transformMatrix;
                        }
                        SceneManager::GetActiveScene()->GetEntityManager()->ReplaceComponent<TransformComponent>(selectedEntity, transformComponent);
                    }*/
            }
        }
    }
}

namespace HE
{
    struct ConsoleLog
    {
        std::string message;
    };

    struct Console
    {
        char                  inputBuffer[256];
        ImVector<char*>       Items;
        ImVector<const char*> Commands;
        ImVector<char*>       History;
        int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
        ImGuiTextFilter       Filter;
        bool                  AutoScroll;
        bool                  ScrollToBottom;

        // Portable helpers
        static int   Stricmp(const char* s1, const char* s2) { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
        static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
        static char* Strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
        static void  Strtrim(char* s) { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

        void    ClearLog()
        {
            for (int i = 0; i < Items.Size; i++)
                free(Items[i]);
            Items.clear();
        }

        void AddLog(const char* fmt, ...) IM_FMTARGS(2)
        {
            // FIXME-OPT
            char buf[1024];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
            buf[IM_ARRAYSIZE(buf) - 1] = 0;
            va_end(args);
            Items.push_back(Strdup(buf));
        }

        int TextEditCallback(ImGuiInputTextCallbackData* data)
        {
            //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
            switch (data->EventFlag)
            {
            case ImGuiInputTextFlags_CallbackCompletion:
            {
                // Example of TEXT COMPLETION

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                ImVector<const char*> candidates;
                for (int i = 0; i < Commands.Size; i++)
                    if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
                        candidates.push_back(Commands[i]);

                if (candidates.Size == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
                }
                else if (candidates.Size == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
                    data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can..
                    // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
                    int match_len = (int)(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c == 0 || c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (int i = 0; i < candidates.Size; i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
            case ImGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                const int prev_history_pos = HistoryPos;
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (HistoryPos == -1)
                        HistoryPos = History.Size - 1;
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= History.Size)
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history_str);
                }
            }
            }
            return 0;
        }

        void ExecCommand(const char* command_line)
        {
            AddLog("# %s\n", command_line);

            // Insert into history. First find match and delete it so it can be pushed to the back.
            // This isn't trying to be smart or optimal.
            HistoryPos = -1;
            for (int i = History.Size - 1; i >= 0; i--)
                if (Stricmp(History[i], command_line) == 0)
                {
                    free(History[i]);
                    History.erase(History.begin() + i);
                    break;
                }
            History.push_back(Strdup(command_line));

            // Process command
            if (Stricmp(command_line, "CLEAR") == 0)
            {
                ClearLog();
            }
            else if (Stricmp(command_line, "HELP") == 0)
            {
                AddLog("Commands:");
                for (int i = 0; i < Commands.Size; i++)
                    AddLog("- %s", Commands[i]);
            }
            else if (Stricmp(command_line, "HISTORY") == 0)
            {
                int first = History.Size - 10;
                for (int i = first > 0 ? first : 0; i < History.Size; i++)
                    AddLog("%3d: %s\n", i, History[i]);
            }
            else
            {
                AddLog("Unknown command: '%s'\n", command_line);
            }

            // On command input, we scroll to bottom even if AutoScroll==false
            ScrollToBottom = true;
        }

        std::shared_ptr<spdlog::logger> spdLogger = nullptr;
    };

    Console console;

    class ConsoleSink : public spdlog::sinks::base_sink<std::mutex>
    {
    public:
        ConsoleSink(Console& console) : console(console) {};
        virtual ~ConsoleSink() {};

        ConsoleSink(ConsoleSink&) = delete;
        ConsoleSink(const ConsoleSink&) = delete;
        ConsoleSink& operator=(ConsoleSink&) = delete;
        ConsoleSink& operator=(const ConsoleSink&) = delete;

    protected:

        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            ConsoleLog log = {};
            log.message = msg.payload;

            console.AddLog(log.message.c_str());

            //flush_();
        }

        void flush_() override
        {
            //console.AddLog(message);
        }

    private:

        Console& console;
    };

    bool CreateConsoleLogger()
    {
        std::string logsDirectory = "Logs";
        if (!std::filesystem::exists(logsDirectory))
        {
            std::filesystem::create_directories(logsDirectory);
        }

        std::vector<spdlog::sink_ptr> sinks =
        {
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
            std::make_shared<ConsoleSink>(console),
        };

        sinks[0]->set_pattern("%^[%Y-%m-%d %T][%n][%l]%v%$");
        sinks[1]->set_pattern("..");

        auto colorSink = static_cast<spdlog::sinks::stdout_color_sink_mt*>(sinks[0].get());
        colorSink->set_color(spdlog::level::trace, FOREGROUND_BLUE);
        colorSink->set_color(spdlog::level::info, std::numeric_limits<uint16_t>::max());
        colorSink->set_color(spdlog::level::warn, FOREGROUND_RED | FOREGROUND_GREEN);
        colorSink->set_color(spdlog::level::err, FOREGROUND_RED);

        console.spdLogger = std::make_shared<spdlog::logger>("Console", sinks.begin(), sinks.end());
        console.spdLogger->set_level(spdlog::level::trace);
        spdlog::register_logger(console.spdLogger);

        GLogger = (Logger*)console.spdLogger.get();

        return true;
    }

    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        Console* console = (Console*)data->UserData;
        return console->TextEditCallback(data);
    }

    void HorizonEditor::DrawConsoleWindow(bool* open)
    {
        if (!ImGui::Begin("Console", open))
        {
            ImGui::End();
            return;
        }

        if (ImGui::SmallButton("Clear"))
        {
            console.ClearLog();
        }
        ImGui::SameLine();

        ImGui::Separator();

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) console.ClearLog();
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

        for (int i = 0; i < console.Items.Size; i++)
        {
            const char* item = console.Items[i];
            if (!console.Filter.PassFilter(item))
                continue;

            // Normally you would store more information in your item than just a string.
            // (e.g. make Items[] an array of structure, store color/type etc.)
            ImVec4 color;
            bool has_color = false;
            if (strstr(item, "[error]")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
            else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
            if (has_color)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(item);
            if (has_color)
                ImGui::PopStyleColor();
        }

        if (console.ScrollToBottom || (console.AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        console.ScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        // Command-line
        bool reclaim_focus = false;
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("Input", console.inputBuffer, IM_ARRAYSIZE(console.inputBuffer), input_text_flags, &TextEditCallbackStub, (void*)&console))
        {
            char* s = console.inputBuffer;
            console.Strtrim(s);
            if (s[0])
            {
                console.ExecCommand(s);
            }
            strcpy(s, "");
            reclaim_focus = true;
        }
        // Auto-focus on window apparition
        ImGui::SetItemDefaultFocus();

        if (reclaim_focus)
        {
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
        }
        ImGui::End();
    }

    std::unordered_map<std::string, std::function<void(const char*, const char* name, void*)>> uiCreator;
    std::vector<std::pair<std::string, bool>> g_editor_node_state_array;
    int                                       g_node_depth = -1;
    bool inited = false;

    float HorizonEditor::GetSnapValue()
    {
        switch (gizmoOperationType)
        {
        case ImGuizmo::OPERATION::TRANSLATE: return 5.0f; break;
        case ImGuizmo::OPERATION::ROTATE: return 10.0f; break;
        case ImGuizmo::OPERATION::SCALE: return 0.1f; break;
        }
        return 0.0f;
    }

    void UIInit()
    {
        using namespace HE;

        uiCreator["bool"] = [](const char* lable, const char* name, void* value)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(name);
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Checkbox(lable, static_cast<bool*>(value));
            ImGui::PopItemWidth();
            ImGui::NextColumn();
        };

        uiCreator["int"] = [](const char* lable, const char* name, void* value)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(name);
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragInt(lable, static_cast<int*>(value));
            ImGui::PopItemWidth();
            ImGui::NextColumn();
        };

        uiCreator["unsigned int"] = [](const char* lable, const char* name, void* value)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(name);
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragInt(lable, static_cast<int*>(value));
            ImGui::PopItemWidth();
            ImGui::NextColumn();
        };

        uiCreator["float"] = [](const char* lable, const char* name, void* value)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(name);
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat(lable, static_cast<float*>(value));
            ImGui::PopItemWidth();
            ImGui::NextColumn();
        };

        uiCreator["struct glm::vec<3,float,0>"] = [](const char* lable, const char* name, void* value)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(name);
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat3(lable, static_cast<float*>(value));
            ImGui::PopItemWidth();
            ImGui::NextColumn();
        };

        uiCreator["class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >"] = [](const char* lable, const char* name, void* value)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(name);
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text(lable, static_cast<std::string*>(value)->c_str());
            /*if (ImGui::InputText(lable, static_cast<std::string*>(value)->c_str(), 256))
            {

            }*/
            ImGui::PopItemWidth();
            ImGui::NextColumn();
        };
    }

    void BeginDockSpace()
    {
        static bool dockSpaceOpen = true;

        // Imgui dock node flags.
        static ImGuiDockNodeFlags dockNodeflags = ImGuiDockNodeFlags_PassthruCentralNode;

        // Imgui window flags.
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

        static bool isFullscreenPersistant = true;
        bool isFullscreen = isFullscreenPersistant;
        if (isFullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        windowFlags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        ImGui::Begin("Dockspace", &dockSpaceOpen, windowFlags);

        ImGui::PopStyleVar();

        if (isFullscreen)
        {
            ImGui::PopStyleVar(2);
        }

        // Set min width
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 300.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockSpaceID = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockSpaceID, ImVec2(0.0f, 0.0f), dockNodeflags);
        }
        style.WindowMinSize.x = minWinSizeX;
    }

    void EndDockSpace()
    {
        ImGui::End();
    }

    void DrawEntityNodeUI(EntityHandle entity)
    {
        EntityHandle selectedEntity = HorizonEditor::GetInstance()->GetSelectedEntity();

        auto entityManager = SceneManager::GetActiveScene()->GetEntityManager();
        const auto& hierarchy = entityManager->GetComponent<SceneHierarchyComponent>(entity);
        ImGuiTreeNodeFlags flags = ((selectedEntity != EntityHandle::Null && selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (hierarchy.numChildren == 0)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        bool opened = ImGui::TreeNodeEx((void*)(uint64)entity, flags, entityManager->GetComponent<NameComponent>(entity).name.c_str());
        if (ImGui::IsItemClicked())
        {
            HorizonEditor::GetInstance()->SetSelectedEntity(entity);
        }

        if (opened)
        {
            auto currentEntity = hierarchy.firstChild;
            for (uint32 i = 0; i < hierarchy.numChildren; i++)
            {
                DrawEntityNodeUI(currentEntity);
                currentEntity = entityManager->GetComponent<SceneHierarchyComponent>(currentEntity).next;
            }
            ImGui::TreePop();
        }

        bool deleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Entity"))
            {
                deleted = true;
            }
            ImGui::EndPopup();
        }

        if (deleted)
        {
            entityManager->DestroyEntity(entity);
            if (selectedEntity == entity)
            {
                selectedEntity = EntityHandle::Null;
            }
        }
    }

    void HorizonEditor::DrawSceneHierarchyWindow(bool* open)
    {
        if (ImGui::Begin("Scene Hierarchy", open))
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
            if (ImGui::TreeNodeEx((void*)(uint64)564788, flags, SceneManager::GetActiveScene()->GetName().c_str()))
            {
                EntityHandle selectedEntity = HorizonEditor::GetInstance()->GetSelectedEntity();
                auto entityManager = SceneManager::GetActiveScene()->GetEntityManager();
                entityManager->Get()->each([&](auto entity)
                    {
                        if (entityManager->GetComponent<SceneHierarchyComponent>(entity).parent == EntityHandle::Null)
                        {
                            DrawEntityNodeUI(entity);
                        }
                    });
                ImGui::TreePop();
            }

            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            {
                HorizonEditor::GetInstance()->SetSelectedEntity(EntityHandle::Null);
            }

            // Right-click on blank space
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Create Empty Entity"))
                {
                    auto newEntity = SceneManager::GetActiveScene()->CreateEntity("Empty Entity");
                    SceneManager::GetActiveScene()->GetEntityManager()->AddComponent<TransformComponent>(newEntity);
                    SceneManager::GetActiveScene()->GetEntityManager()->AddComponent<SceneHierarchyComponent>(newEntity);
                }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    bool DrawTransformComponentUI(const char* lable, TransformComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Position");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##Position", static_cast<float*>(&component.position.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Rotation");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##Rotation", static_cast<float*>(&component.rotation.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Scale");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##Scale", static_cast<float*>(&component.scale.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    bool DrawCameraComponentUI(const char* lable, CameraComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Projection Mode");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            static const char* projectionModeNames[] = { "Perspective", "Orthographic" };
            int projectionMode = (int)component.projectionMode;
            ImGui::Combo("##CameraProjectionMode", &projectionMode, projectionModeNames, IM_ARRAYSIZE(projectionModeNames));
            component.projectionMode = (CameraProjectionMode)projectionMode;

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Near Clipping Plane");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##NearClippingPlane", &component.nearClippingPlane))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Far Clipping Plane");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##FarClippingPlane", &component.farClippingPlane))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Field of View");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##FieldOfView", &component.fieldOfView))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Override Aspect Ratio");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Checkbox("##OverrideAspectRatio", &component.overrideAspectRatio))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();


            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Aspect Ratio");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##AspectRatio", &component.aspectRatio))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    bool DrawLightComponentUI(const char* lable, LightComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            //if (component.type == LightComponent::LightType::Directional)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Color");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::ColorEdit3("##Color", &component.color.x))
                {
                    dirty = true;
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();
            }
            //else if (component.type == LightComponent::LightType::Point)
            //{
            //    ImGui::AlignTextToFramePadding();
            //    ImGui::TextUnformatted("Luminance");
            //    ImGui::NextColumn();
            //    ImGui::PushItemWidth(-1);
            //    if (ImGui::DragFloat3("##Luminance", static_cast<float*>(&component.color.x)))
            //    {
            //        dirty = true;
            //    }
            //    ImGui::PopItemWidth();
            //    ImGui::NextColumn();
            //}

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Luminous Intensity");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##LuminousIntensity", &component.luminousIntensity, 0.001f, 0.0f, 200000.0f))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            if (component.type == LightComponent::LightType::Point)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Radius");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##Radius", &component.radius))
                {
                    dirty = true;
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Apex Angle");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##ApexAngle", &component.apexAngle))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Use Color Temperature");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Checkbox("##UseColorTemperature", &component.useColorTemperature))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Color Temperature");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##ColorTemperature", &component.colorTemperature))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Cast Shadows");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Checkbox("##CastShadows", &component.castShadows))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Use Ray Tracing Shadows");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Checkbox("##UseRayTracingShadows", &component.useRayTracingShadows))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Cascade Count");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragInt("##NumShadowCascades", &component.numShadowCascades, 1, 0, RendererMaxCascadedShadowMapCount))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Max Shadow Distance");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##maxShadowDistance", &component.maxShadowDistance, 0.01f, 0.0f, 1000.0f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Map Depth Bias Constant Factor");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##shadowMapDepthBiasConstantFactor", &component.shadowMapDepthBiasConstantFactor, 0.001f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Map Depth Bias Slope Factor");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##shadowMapDepthBiasSlopeFactor", &component.shadowMapDepthBiasSlopeFactor, 0.001f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            if (component.type == LightComponent::LightType::Directional)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Atmosphere Light Disk Color Tint");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::ColorEdit3("##atmosphereLightDiskColorTint", &component.atmosphereLightDiskColorTint.r))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();
            }

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    bool DrawSkyAtmosphereComponentUI(const char* lable, SkyAtmosphereComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Ground Radius");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##GroundRadius", &component.groundRadius))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Ground Albedo");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##GroundAlbedo", static_cast<float*>(&component.groundAlbedo.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Atmosphere Height");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##AtmosphereHeight", &component.atmosphereHeight))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Multiple Scattering Factor");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##MultipleScatteringFactor", &component.multipleScatteringFactor))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Rayleigh Scattering");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##RayleighScattering", static_cast<float*>(&component.rayleighScattering.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Rayleigh Scale Height");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##RayleighScaleHeight", &component.rayleighScaleHeight))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Scattering");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##MieScattering", static_cast<float*>(&component.mieScattering.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Extinction");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##MieExtinction", static_cast<float*>(&component.mieExtinction.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Anisotropy");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##MieAnisotropy", &component.mieAnisotropy))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Scale Height");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##MieScaleHeight", &component.mieScaleHeight))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Cos Max Sun Zenith Angle");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##CosMaxSunZenithAngle", &component.cosMaxSunZenithAngle))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Sky Luminance Tint");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##skyLuminanceTint", &component.skyLuminanceTint.x))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Sky Luminance Intensity");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##skyLuminanceIntensity", &component.skyLuminanceIntensity))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    void DrawBoneNode(ArmatureComponent& component, uint32 boneIndex)
    {
        auto& bone = component.bones[boneIndex];

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        // TODO: id
        bool opened = ImGui::TreeNodeEx((void*)(uint64)boneIndex, flags, bone.name.c_str());
        if (ImGui::IsItemClicked())
        {

        }

        if (opened)
        {
            for (const auto& childIndex : bone.children)
            {
                DrawBoneNode(component, childIndex);
            }
            ImGui::TreePop();
        }
    }

    bool DrawArmatureComponentUI(const char* lable, ArmatureComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawBoneNode(component, 0);
        }
        return dirty;
    }

    void HorizonEditor::DrawInspectorWindow(bool* open)
    {
        if (ImGui::Begin("Inspector", open))
        {
            EntityHandle selectedEntity = HorizonEditor::GetInstance()->GetSelectedEntity();
            if (selectedEntity != EntityHandle::Null)
            {
                ImGui::PushID((int)uint64(selectedEntity));

                auto& transformComponent = SceneManager::GetActiveScene()->GetEntityManager()->GetComponent<TransformComponent>(selectedEntity);
                bool dirty = DrawTransformComponentUI("Transform Component", transformComponent);
                if (dirty)
                {
                    SceneManager::GetActiveScene()->GetEntityManager()->ReplaceComponent<TransformComponent>(selectedEntity, transformComponent);
                }

                if (SceneManager::GetActiveScene()->GetEntityManager()->HasComponent<CameraComponent>(selectedEntity))
                {
                    auto& cameraComponent = SceneManager::GetActiveScene()->GetEntityManager()->GetComponent<CameraComponent>(selectedEntity);
                    bool dirty = DrawCameraComponentUI("Camera Component", cameraComponent);
                }

                if (SceneManager::GetActiveScene()->GetEntityManager()->HasComponent<LightComponent>(selectedEntity))
                {
                    auto& lightComponent = SceneManager::GetActiveScene()->GetEntityManager()->GetComponent<LightComponent>(selectedEntity);
                    bool dirty = DrawLightComponentUI("Light Component", lightComponent);
                }

                if (SceneManager::GetActiveScene()->GetEntityManager()->HasComponent<SkyAtmosphereComponent>(selectedEntity))
                {
                    auto& skyAtmosphereComponent = SceneManager::GetActiveScene()->GetEntityManager()->GetComponent<SkyAtmosphereComponent>(selectedEntity);
                    bool dirty = DrawSkyAtmosphereComponentUI("Sky Atmosphere Component", skyAtmosphereComponent);
                }

                if (SceneManager::GetActiveScene()->GetEntityManager()->HasComponent<ArmatureComponent>(selectedEntity))
                {
                    auto& armatureComponent = SceneManager::GetActiveScene()->GetEntityManager()->GetComponent<ArmatureComponent>(selectedEntity);
                    bool dirty = DrawArmatureComponentUI("Armature Component", armatureComponent);
                }

                ImGui::PopID();
            }
        }
        ImGui::End();
    }

    void HorizonEditor::DrawProfilerWindow(bool* open)
    {
        RenderBackendGPUProfiler* gpuProfiler = ((Renderer*)renderEngine)->gpuProfiler;
        if (ImGui::Begin("Profiler", open))
        {
            ImGui::Text("FPS: %.1f (%.4f ms/frame)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate));

            if (ImGui::CollapsingHeader("CPU Profiler", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("CPU Frametime: %.4f ms", 1000.0f * deltaTime);
            }

            if (ImGui::CollapsingHeader("GPU Profiler", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (uint32 regionIndex = 0; regionIndex < gpuProfiler->GetRegionCount(); regionIndex++)
                {
                    ImGui::Text("%s: %.4f ms", gpuProfiler->GetRegionName(regionIndex), gpuProfiler->GetRegionTime(regionIndex));
                }
            }
        }
        ImGui::End();
    }

    void HorizonEditor::DrawRenderSettingsWindow(bool* open)
    {
        auto& renderSettings = ((Renderer*)renderEngine)->GetRealTimeRendererSettings_Deprecated();

        if (ImGui::Begin("Render Settings", open))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Renderer");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            static const char* rendererTypeNames[] = { "Real-Time", "Path Tracing" };
            int rendererType = (int)renderSettings.rendererType;
            ImGui::Combo("##Renderer", &rendererType, rendererTypeNames, IM_ARRAYSIZE(rendererTypeNames));
            renderSettings.rendererType = (RendererType)rendererType;

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_None))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Fixed Pre-Exposure");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::Checkbox("##FixedPreExposure", &renderSettings.fixedPreExposureEnabled))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Pre-Exposure");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##PreExposure", &renderSettings.fixedPreExposure))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Indirect Lighting Tint");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::ColorEdit3("##indirectLightingTint", &renderSettings.indirectLightingTint.x))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Indirect Lighting Intensity");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##indirectLightingIntensity", &renderSettings.indirectLightingIntensity, 0.01f, 0.0f, 1000.0f))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

#if HE_ENBALE_STREAMLINE_SUPPORT
            if (ImGui::CollapsingHeader("NVIDIA Reflex", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Mode");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                const char* items[] = { "Off", "On", "On+Boost" };
                int item = (int)renderPipelineSettings.reflexMode;
                ImGui::Combo("##ReflexMode", &item, items, IM_ARRAYSIZE(items));

                if (item == 0)
                {
                    sl::ReflexOptions reflexOptions = {};
                    reflexOptions.mode = sl::ReflexMode::eOff;
                    reflexOptions.frameLimitUs = 0;
                    reflexOptions.useMarkersToOptimize = true;
                    if (SL_FAILED(result, slReflexSetOptions(reflexOptions)))
                    {
                        LogError(GLogger, std::format("slReflexSetOptions, error code: {}", (int32)result));
                    }
                    else
                    {
                        renderPipelineSettings.reflexMode = (NVIDIAReflexMode)item;
                    }
                }
                else if (item == 1)
                {
                    sl::ReflexOptions reflexOptions = {};
                    reflexOptions.mode = sl::ReflexMode::eLowLatency;
                    reflexOptions.frameLimitUs = 0;
                    reflexOptions.useMarkersToOptimize = true;
                    if (SL_FAILED(result, slReflexSetOptions(reflexOptions)))
                    {
                        LogError(GLogger, std::format("slReflexSetOptions, error code: {}", (int32)result));
                    }
                    else
                    {
                        renderPipelineSettings.reflexMode = (NVIDIAReflexMode)item;
                    }
                }
                else if (item == 2)
                {
                    sl::ReflexOptions reflexOptions = {};
                    reflexOptions.mode = sl::ReflexMode::eLowLatencyWithBoost;
                    reflexOptions.frameLimitUs = 0;
                    reflexOptions.useMarkersToOptimize = true;
                    if (SL_FAILED(result, slReflexSetOptions(reflexOptions)))
                    {
                        LogError(GLogger, std::format("slReflexSetOptions, error code: {}", (int32)result));
                    }
                    else
                    {
                        renderPipelineSettings.reflexMode = (NVIDIAReflexMode)item;
                    }
                }
                else
                {
                    assert(0);
                }

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }
#endif
            if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Technique");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items[] = { "None", "Screen Space Shadows", "Ray Tracing Shadows" };
                static int item = 0;
                ImGui::Combo("##ShadowsTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.shadowsTechnique = (ShadowsTechnique)item;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader("Reflections", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Technique");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items[] = { "None", "Screen Space Reflections", "Ray Tracing Reflections" };
                static int item = 0;
                ImGui::Combo("##ReflectionsTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.reflectionsTechnique = (ReflectionsTechnique)item;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Denosing");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::Checkbox("##SSRDenosing", &renderSettings.ssrSettings.denosingEnabled))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Quality");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items2[] = { "Low", "Medium", "High", "    Epic" };
                static int item2 = 0;
                ImGui::Combo("##SSRQuality", &item2, items2, IM_ARRAYSIZE(items2));
                renderSettings.ssrSettings.qualiy = (ScreenSpaceReflectionsQuality)item2;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader("Ambient Occlusion", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                bool gtaoEnabled = (renderSettings.ambientOcclusionTechnique == AmbientOcclusionTechnique::GroundTruthAmbientOcclusion) ? true : false;
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Enable");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::Checkbox("##Enable", &gtaoEnabled))
                {
                    if (gtaoEnabled)
                    {
                        renderSettings.ambientOcclusionTechnique = AmbientOcclusionTechnique::GroundTruthAmbientOcclusion;
                    }
                    else
                    {
                        renderSettings.ambientOcclusionTechnique = AmbientOcclusionTechnique::None;
                    }
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Radius");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##Radius", &renderSettings.gtaoSettings.radius))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Factor");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##Factor", &renderSettings.gtaoSettings.factor))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Thickness");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##Thickness", &renderSettings.gtaoSettings.thickness))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Multiple-Bounce");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::Checkbox("##MultipleBounce", &renderSettings.gtaoSettings.multiBounce))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader("Antialiasing", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Technique");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items[] = { "None", "TAA", "NVIDIA DLAA" };
                static int item = 0;
                ImGui::Combo("##AntialiasingTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.antialiasingTechnique = (AntialiasingTechnique)item;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader("Super Resolution", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::Columns(2);
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Technique");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                const char* items[] = { "None", "AMD FSR2", "NVIDIA DLSS" };
                int item = (int)renderSettings.superResolutionTechnique;
                ImGui::Combo("##SuperResolutionTechnique", &item, items, IM_ARRAYSIZE(items));
                renderSettings.superResolutionTechnique = (SuperResolutionTechnique)item;

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                switch (item)
                {
                case (uint32)SuperResolutionTechnique::FSR2:
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Quality Mode");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static const char* fsr2QualityModeNames[] = { "Custom", "Quality", "Balanced", "Performance", "Ultra Performance" };
                    int fsr2QualityModeNameIndex = (int)renderSettings.fsr2Settings.qualityMode;
                    ImGui::Combo("##FSR2QualityMode", &fsr2QualityModeNameIndex, fsr2QualityModeNames, IM_ARRAYSIZE(fsr2QualityModeNames));
                    renderSettings.fsr2Settings.qualityMode = (FSR2QualityMode)fsr2QualityModeNameIndex;
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    if (renderSettings.fsr2Settings.qualityMode == FSR2QualityMode::Custom)
                    {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted("Custom Upscale Ratio");
                        ImGui::NextColumn();
                        ImGui::PushItemWidth(-1);
                        if (ImGui::DragFloat("##FSR2UpscaleRatio", &renderSettings.fsr2Settings.customUpscaleRatio, 0.001f, 1.0f, 3.0f))
                        {

                        }
                        ImGui::PopItemWidth();
                        ImGui::NextColumn();
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Enable Sharpening");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Checkbox("##FSR2EnableSharpening", &renderSettings.fsr2Settings.useRCAS))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    if (!renderSettings.fsr2Settings.useRCAS)
                    {
                        ImGui::BeginDisabled();
                    }
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Sharpeness");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##FSR2Sharpeness", &renderSettings.fsr2Settings.sharpeness, 0.001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();
                    if (!renderSettings.fsr2Settings.useRCAS)
                    {
                        ImGui::EndDisabled();
                    }
                } break;
                case (uint32)SuperResolutionTechnique::DLSSSuperResolution:
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Quality Mode");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static const char* dlssQualityModeNames[] = { "Off", "Auto", "Quality", "Balanced", "Performance", "Ultra Performance" };
                    static int dlssQualityModeNameIndex = 1;
                    ImGui::Combo("##DLSSQualityMode", &dlssQualityModeNameIndex, dlssQualityModeNames, IM_ARRAYSIZE(dlssQualityModeNames));
                    renderSettings.dlssSettings.qualityMode = (DLSSQualityMode)dlssQualityModeNameIndex;
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                } break;
                default: break;
                }

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

            if (ImGui::CollapsingHeader((const char*)(u8"Post Proccesing"), ImGuiTreeNodeFlags_DefaultOpen))
            {
                // TODO: Visualize tone mapping curve
                if (ImGui::TreeNode("Tone Mapping"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Tone Mapping Operator");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static const char* toneMappingOperatorNames[] = { "Linear", "ACES" };
                    int toneMappingOperator = (int)renderSettings.toneMappingOperator;
                    ImGui::Combo("##ToneMappingOperator", &toneMappingOperator, toneMappingOperatorNames, IM_ARRAYSIZE(toneMappingOperatorNames));
                    renderSettings.toneMappingOperator = (ToneMappingOperatorType)toneMappingOperator;

                    ImGui::PopItemWidth();
                    ImGui::NextColumn();


                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Bloom"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##BloomIntensity", &renderSettings.postProcessingSettings.bloomIntensity, 0.01f, 0.0f, 10.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Radius");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##BloomRadius", &renderSettings.postProcessingSettings.bloomRadius, 0.001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Lens Dirt"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LensDirtIntensity", &renderSettings.postProcessingSettings.lensDirtIntensity, 0.001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Tint");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##LensDirtTint", &renderSettings.postProcessingSettings.lensDirtTint.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Lens Flares"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##lensFlaresIntensity", &renderSettings.postProcessingSettings.lensFlaresIntensity, 0.001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Exposure"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    const char* items[] = { "Fixed Exposure", "Auto Exposure" };
                    int item = (int)renderSettings.exposureMethod;

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Exposure Method");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Combo("##ExposureMethod", &item, items, IM_ARRAYSIZE(items)))
                    {
                        renderSettings.exposureMethod = (ExposureMethod)item;
                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Fixed Exposure Value");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##fixedExposureValue", &renderSettings.postProcessingSettings.fixedExposureValue))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Exposure Compensation");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureExposureCompensation", &renderSettings.postProcessingSettings.autoExposureExposureCompensation))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Min Exposure Value");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureMinExposureValue", &renderSettings.postProcessingSettings.autoExposureMinExposureValue))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Max Exposure Value");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureMaxExposureValue", &renderSettings.postProcessingSettings.autoExposureMaxExposureValue))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Speed Dark to Bright");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureSpeedDarkToBright", &renderSettings.postProcessingSettings.autoExposureSpeedDarkToBright))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();


                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Speed Bright to Dark");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureSpeedBrightToDark", &renderSettings.postProcessingSettings.autoExposureSpeedBrightToDark))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Low Percent");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramLowPercent", &renderSettings.postProcessingSettings.autoExposureHistogramLowPercent, 0.0001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("High Percent");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramHighPercent", &renderSettings.postProcessingSettings.autoExposureHistogramHighPercent, 0.0001f, 0.0f, 1.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Histogram Min EV100");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramMinEV100", &renderSettings.postProcessingSettings.autoExposureHistogramMinEV100))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Histogram Max EV100");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##autoExposureHistogramMaxEV100", &renderSettings.postProcessingSettings.autoExposureHistogramMaxEV100))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Local Exposure"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Enable");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Checkbox("##LocalExposure", &renderSettings.postProcessingSettings.localExposureEnabled))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Shadows");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LocalExposureShadows", &renderSettings.postProcessingSettings.localExposureShadows, 0.01f, 0.0f, 4.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Highlights");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LocalExposureHighlights", &renderSettings.postProcessingSettings.localExposureHighlights, 0.01f, 0.0f, 4.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Coarsest Mip Level");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragInt("##LocalExposureCoarsestMipLevel", &renderSettings.postProcessingSettings.localExposureCoarsestMipLevel, 1.0f, 0, 32))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Display Mip Level");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragInt("##LocalExposureDisplayMipLevel", &renderSettings.postProcessingSettings.localExposureDisplayMipLevel, 1.0f, 0, 32))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Exposure Preference Sigma");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##LocalExposurePreferenceSigma", &renderSettings.postProcessingSettings.localExposurePreferenceSigma))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Depth Of Field"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Scale");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFScale", &renderSettings.postProcessingSettings.dofScale))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Distance");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalDistance", &renderSettings.postProcessingSettings.dofFocalDistance))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Region");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalRegion", &renderSettings.postProcessingSettings.dofFocalRegion))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Near Transition Region");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalNearTransitionRegion", &renderSettings.postProcessingSettings.dofNearTransitionRegion))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Far Transition Region");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalFarTransitionRegion", &renderSettings.postProcessingSettings.dofFarTransitionRegion))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Near Region Blur Size");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalNearRegionBlurSize", &renderSettings.postProcessingSettings.dofNearRegionBlurSize))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Focal Far Region Blur Size");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##DOFFocalFarRegionBlurSize", &renderSettings.postProcessingSettings.dofFarRegionBlurSize))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Chromatic Aberration"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Intensity");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##ChromaticAberrationIntensity", &renderSettings.postProcessingSettings.chromaticAberrationIntensity))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Offset");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##ChromaticAberrationOffset", &renderSettings.postProcessingSettings.chromaticAberrationOffset))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Color Correction"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Saturation");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionSaturation", &renderSettings.postProcessingSettings.colorCorrectionSaturation.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Contrast");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionContrast", &renderSettings.postProcessingSettings.colorCorrectionContrast.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Gamma");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionGamma", &renderSettings.postProcessingSettings.colorCorrectionGamma.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Gain");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionGain", &renderSettings.postProcessingSettings.colorCorrectionGain.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Offset");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::ColorEdit4("##colorCorrectionOffset", &renderSettings.postProcessingSettings.colorCorrectionOffset.x))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Color Grading"))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("White Balance");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##ColorGraingWhiteBalance", &renderSettings.postProcessingSettings.colorGradingWhiteBalanceColorTemperature, 0.1f, 1000.0f, 25000.0f))
                    {

                    }
                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::Columns(1);
                    ImGui::Separator();
                    ImGui::PopStyleVar();

                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }

    void HorizonEditor::DrawViewSettings()
    {
        static const char* viewModes[] = { "Lit", "Wireframe", "Illuminance", "World Space Normal", "Primitive ID", "Material ID", "Motion Vectors", "Ambient Occlusion", "Screen Space Shadow Mask", "Surfel GI Surfel", "Surfel GI Heatmap"};
        static int currentViewModeIndex = 0;
        ImGui::Combo("##ViewMode", &currentViewModeIndex, viewModes, IM_ARRAYSIZE(viewModes));
        viewMode = (DebugViewMode)currentViewModeIndex;
    }

    void HorizonEditor::DrawOverlay()
    {
        static int corner = 0;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindowFlags windowFags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        if (corner != -1)
        {
            const float padding = 10.0f;
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 workPos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
            ImVec2 workSize = viewport->WorkSize;
            ImVec2 windowPos, windowPosPivot;
            windowPos.x = (corner & 1) ? (workPos.x + workSize.x - padding) : (workPos.x + padding);
            windowPos.y = (corner & 2) ? (workPos.y + workSize.y - padding) : (workPos.y + padding);
            windowPosPivot.x = (corner & 1) ? 1.0f : 0.0f;
            windowPosPivot.y = (corner & 2) ? 1.0f : 0.0f;
            ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
            ImGui::SetNextWindowViewport(viewport->ID);
            windowFags |= ImGuiWindowFlags_NoMove;
        }
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        static bool open = true;
        if (ImGui::Begin("Overlay", &open, windowFags))
        {
            ImGui::Text(HORIZON_ENGINE_NAME);
            ImGui::Separator();
            ImGui::Text("FPS: %.1f (%.2f ms/frame)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate));

            // TODO
            DrawViewSettings();
        }
        ImGui::End();
    }

    //void MaterialEditor::_nodePopup(const char* name, ShaderGraph& g)
    //{
    //    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8, 8 });
    //    if (ImGui::BeginPopup(name, ImGuiWindowFlags_NoMove))
    //    {
    //        const auto clickPos = ImGui::GetMousePosOnOpeningCurrentPopup();

    //        static std::string pattern;
    //        ImGui::InputTextWithHint(IM_UNIQUE_ID, ICON_FA_FILTER " Filter", &pattern);
    //        ImGui::SameLine();
    //        if (ImGui::SmallButton(ICON_FA_ERASER))
    //        {
    //            pattern.clear();
    //        }

    //        ImGui::Spacing();
    //        ImGui::Separator();
    //        ImGui::Spacing();

    //        const auto entries = buildNodeMenuEntries(m_scriptedFunctions, m_project.userFunctions);
    //        if (const auto node = processNodeMenu(g, entries, pattern, m_shaderType, m_project.blueprint);
    //            node)
    //        {
    //            ImNodes::ClearNodeSelection();
    //            const auto id = node->id;
    //            ImNodes::SetNodeScreenSpacePos(id, clickPos);
    //            ImNodes::SelectNode(id);
    //        }

    //        ImGui::EndPopup();
    //    }
    //    ImGui::PopStyleVar();
    //}

    void DrawMenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                //ShowExampleMenuFile();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
                if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                if (ImGui::MenuItem("Paste", "CTRL+V")) {}
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void HorizonEditor::OnDrawUI()
    {
        OPTICK_EVENT();

//#define RING_COUNT     4.0f
//#define RING_DENSITY 8.0f
//        Vector2 center = Vector2(0.0f, 0.0f);
//        for (int i = 0; i <= 4 - 1; i++)
//        {
//            float rad = 1.0f / (RING_COUNT-1.0f) * i;
//            int cnt = std::max(8 * i, 1);
//            float inc = 2.0 * glm::pi<float>() / cnt;
//            for (int j = 0; j < cnt; j++)
//            {
//                float theta = j * inc;
//                Vector2 pos = center + rad * Vector2(cos(theta), sin(theta));
//                printf("POS: %.06f %.06f\n", pos.x, pos.y);
//            }
//        }

        //DrawMenuBar();

        BeginDockSpace();

        if (!inited)
        {
            UIInit();
            inited = true;
        }

        ImVec2 cursorPos = ImGui::GetCursorPos();
        cursorPos = { 0, 0 };
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        viewportPos = Vector4(cursorPos.x + windowPos.x, cursorPos.y + windowPos.y, cursorPos.x + windowPos.x + windowSize.x, cursorPos.y + windowPos.y + windowSize.y);

        if (showSceneHierarchyWindow)
        {
            DrawSceneHierarchyWindow(&showSceneHierarchyWindow);
        }

        if (showInspectorWindow)
        {
            DrawInspectorWindow(&showInspectorWindow);
        }

        if (showProfilerWindow)
        {
            DrawProfilerWindow(&showProfilerWindow);
        }

        if (showConsoleWindow)
        {
            DrawConsoleWindow(&showConsoleWindow);
        }

        if (showRenderSettingsWindow)
        {
            DrawRenderSettingsWindow(&showRenderSettingsWindow);
        }

        bool showSceneViewportWindow = true;
        if (true)
        {
            if (!sceneViewportWindow)
            {
                sceneViewportWindow = new SceneViewportWindow("DefaultScene", this);
            }
            sceneViewportWindow->OnImGuiRender(showSceneViewportWindow);
        }

        if (true)
        {
            if (!fileBrowserWindow)
            {
                fileBrowserWindow = new FileBrowserWindow(this, nullptr);
            }
            fileBrowserWindow->OnImGuiRender();
        }

        if (true)
        {
            ImGui::Begin("node editor");
            const int hardcoded_node_id = 1;

            ImNodes::BeginNodeEditor();

            const bool openPopup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                ImNodes::IsEditorHovered() &&
                ImGui::IsMouseReleased(ImGuiMouseButton_Right);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
            if (!ImGui::IsAnyItemHovered() && openPopup)
            {
                ImGui::OpenPopup("Add Node");
            }

            if (ImGui::BeginPopup("Add Node"))
            {
                const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

                if (ImGui::MenuItem("Multiply"))
                {
                    const int node_id = ++current_id;
                    ImNodes::SetNodeScreenSpacePos(node_id, ImGui::GetMousePos());
                    ImNodes::SnapNodeToGrid(node_id);
                    ShadeGraphNode* newNode = new ShadeGraphNode();
                    newNode->id = node_id;
                    newNode->type = ShadeGraphFindNodeType("Multiply");
                    newNode->title = newNode->type->uniqueName;
                    newNode->Init();
                    nodes.push_back(newNode);

                    ImNodes::SetNodeScreenSpacePos(node_id, click_pos);
                }

                if (ImGui::MenuItem("Sample Texture 2D"))
                {
                    const int node_id = ++current_id;
                    ImNodes::SetNodeScreenSpacePos(node_id, ImGui::GetMousePos());
                    ImNodes::SnapNodeToGrid(node_id);
                    ShadeGraphNode* newNode = new ShadeGraphNode();
                    newNode->id = node_id;
                    newNode->type = ShadeGraphFindNodeType("Sample Texture 2D");
                    newNode->title = newNode->type->uniqueName;
                    newNode->Init();
                    nodes.push_back(newNode);

                    ImNodes::SetNodeScreenSpacePos(node_id, click_pos);
                }

                if (ImGui::MenuItem("Principled BSDF"))
                {
                    const int node_id = ++current_id;
                    ImNodes::SetNodeScreenSpacePos(node_id, ImGui::GetMousePos());
                    ImNodes::SnapNodeToGrid(node_id);
                    ShadeGraphNode* newNode = new ShadeGraphNode();
                    newNode->id = node_id;
                    newNode->type = ShadeGraphFindNodeType("Principled BSDF");
                    newNode->title = newNode->type->uniqueName;
                    newNode->Init();
                    nodes.push_back(newNode);

                    ImNodes::SetNodeScreenSpacePos(node_id, click_pos);
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            for (ShadeGraphNode* node : nodes)
            {
                ImNodes::BeginNode(node->id, IM_COL32(43, 101, 43, 255));

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(node->title.c_str());
                ImNodes::EndNodeTitleBar();

                for (ShadeGraphPinInstance* input : node->inputs)
                {
                    //if ()
                    {
                        ImNodes::BeginInputAttribute((node->id << 8) | (input->GetIndexInNode()));
                        ImGui::TextUnformatted(input->type->name.c_str());
                        ImNodes::EndInputAttribute();
                    }
                    //else // TODO
                    //{
                    //    ImNodes::BeginStaticAttribute(node.id << 16);
                    //    ImGui::PushItemWidth(120.0f);
                    //    ImGui::DragFloat("value", &node.value, 0.01f);
                    //    ImGui::PopItemWidth();
                    //    ImNodes::EndStaticAttribute();
                    //}
                }

                for (ShadeGraphPinInstance* output : node->outputs)
                {
                    ImNodes::BeginOutputAttribute((node->id << 24) | (output->GetIndexInNode()));
                    const float text_width = ImGui::CalcTextSize("output").x;
                    ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
                    ImGui::TextUnformatted(output->type->name.c_str());
                    ImNodes::EndOutputAttribute();
                }

                ImNodes::EndNode();
            }
            ImGui::PopStyleColor();

        /*    constexpr auto kAddNodePopupId = IM_UNIQUE_ID;
            if (ImNodes::IsEditorHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            {
                ImGui::OpenPopup(kAddNodePopupId);
            }*/

        /*    _nodePopup(kAddNodePopupId, g);
            auto changed = _inspectNodes(g, connectedVertices);
            _renderLinks(g);
            if (m_miniMap.enabled)
            {
                ImNodes::MiniMap(m_miniMap.size, m_miniMap.location);
            }*/

            for (const Link& link : links)
            {
                ImNodes::Link(link.id, link.start_attr, link.end_attr);
            }

            ImNodes::EndNodeEditor();

            {
                Link link;
                if (ImNodes::IsLinkCreated(&link.start_attr, &link.end_attr))
                {
                    link.id = ++current_id;
                    links.push_back(link);
                }
            }

            {
                int link_id;
                if (ImNodes::IsLinkDestroyed(&link_id))
                {
                    auto iter = std::find_if(
                        links.begin(), links.end(), [link_id](const Link& link) -> bool {
                            return link.id == link_id;
                        });
                    assert(iter != links.end());
                    links.erase(iter);
                }
            }

            /*if (ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows))
            {
                changed |= _handleNewLinks(g, connectedVertices);
                changed |= _handleDeletedLinks(g, connectedVertices);
                changed |= _handleDeletedNodes(g, connectedVertices);
            }*/

            ImGui::End();
        }

        OnDrawUIEx();

        if (showOverlay)
        {
            //DrawOverlay();
        }

        EndDockSpace();
    }
}

int HorizonEditorMain(int argc, char** argv)
{
    int exitCode = EXIT_SUCCESS;
    HE::HorizonEditor* editor = new HE::HorizonEditor();
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