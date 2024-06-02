#pragma once

#include "Engine/HorizonEngineModule.h"
#include "WindowSystem.h"
#include "HorizonEditorUI.h"
#include "EditorCamera.h"
#include "EditorSceneManager.h"

#define HORIZON_EDITOR_APPLICATION_NAME "Horizon Editor"

struct ImGuiContext;

namespace Horizon
{
    //class FileBrowserWindow;
    //class SceneViewportWindow;

    //struct ImNodesEditorContext;
    ////struct Node
    ////{
    ////    int   id;
    ////    float value;

    ////    Node(const int i, const float v) : id(i), value(v) {}
    ////};

    //struct Link
    //{
    //    int id;
    //    int start_attr, end_attr;
    //};

    //class SelectionManager
    //{
    //public:
    //    void Select(EntityHandle entity)
    //    {
    //        if (IsSelected(entity))
    //        {
    //            return;
    //        }
    //        selection.push_back(entity);
    //    }
    //    bool IsSelected(EntityHandle entity) const
    //    {
    //        return std::find(selection.begin(), selection.end(), entity) != selection.end();
    //    }
    //    void Deselect(EntityHandle entity)
    //    {
    //        const auto& it = std::find(selection.begin(), selection.end(), entity);
    //        if (it != selection.end())
    //        {
    //            selection.erase(it);
    //        }
    //    }
    //    void DeselectAll()
    //    {
    //        selection.clear();
    //    }
    //    uint64 GetSelectionCount() const
    //    {
    //        return (uint64)selection.size();
    //    }
    //private:
    //    std::vector<EntityHandle> selection;
    //};

    //class ProjectSettings
    //{
    //public:
    //    RenderBackendGraphicsAPI graphicsAPI;
    //};

    class HorizonEditor
    {
    public:

        static HorizonEditor* Instance;
        static HorizonEditor* GetInstance()
        {
            return Instance;
        }

        HorizonEditor();
        ~HorizonEditor();

        HorizonEditor(HorizonEditor&&) = delete;
        HorizonEditor(const HorizonEditor&) = delete;
        HorizonEditor& operator=(HorizonEditor&&) = delete;
        HorizonEditor& operator=(const HorizonEditor&) = delete;

        bool Init();
        void Exit();
        int Run();
        void Tick();

        //void Setup();
        //void Clear();

        void SetColorTheme(HorizonEditorColorTheme theme) const;

        //void OnUpdate(float deltaTime);
        //void OnRender(float deltaTime);
        void OnDrawUI();
        //void OnDrawUIEx();

        //void OnKeyPressed(KeyCode key, bool repeat) {}
        //void OnKeyReleased(KeyCode key) {}

        //uint32 GetFrameCounter() const
        //{
        //    return frameCounter;
        //}

        //EntityHandle GetSelectedEntity() const
        //{
        //    return selectedEntity;
        //}

        //void SetSelectedEntity(EntityHandle entity)
        //{
        //    selectedEntity = entity;
        //}

        bool IsExitRequested() const
        {
            return isExitRequested;
        }

        void SetExitRequest(bool value)
        {
            isExitRequested = value;
        }

        //const std::string& GetApplicationName() const
        //{
        //    return applicationName;
        //}

        //const std::filesystem::path& GetExecutableDirectory() const
        //{
        //    return executableDirectory;
        //}

        //const std::filesystem::path& GetExecutablePath() const
        //{
        //    return executablePath;
        //}

        //Window* GetMainWindow()
        //{
        //    return window;
        //}

        //ImNodesEditorContext* context = nullptr;
        //std::vector<ShadeGraphNode*>     nodes;
        //std::vector<Link>     links;
        //int                   current_id = 0;

        //RenderBackendTextureHandle directoryIcon;
        //RenderBackendTextureHandle fileIcon;

        //RenderBackendTextureHandle backwardButtonIcon;
        //RenderBackendTextureHandle forwardButtonIcon;
        //RenderBackendTextureHandle parentButtonIcon;
        //RenderBackendTextureHandle searchButtonIcon;
        //RenderBackendTextureHandle refreshButtonIcon;

        //RenderBackendTextureHandle playButtonIcon;
        //RenderBackendTextureHandle stopButtonIcon;
        //RenderBackendTextureHandle pauseButtonIcon;

        //SceneViewportWindow* sceneViewportWindow = nullptr;
        //FileBrowserWindow* fileBrowserWindow = nullptr;

        //enum class SceneViewportState
        //{
        //    Edit = 0,
        //    Play = 1,
        //    Pause = 2,
        //};
        //SceneViewportState sceneViewportState = SceneViewportState::Edit;

        //Vector4 viewportPos;
    private:

        void InitializeImGuiContext();

        std::string applicationName;
        //std::filesystem::path executableDirectory;
        std::filesystem::path executablePath;

        //uint32 initialWidth = 2048;
        //uint32 initialHeight = 1024;

        //EntityHandle selectedEntity = EntityHandle::Null;

        //EngineSubsystem* renderEngine = nullptr;
        //SelectionManager* selectionManager;

        //int gizmoOperationType;

        //float GetSnapValue();

        //bool showOverlay = true;
        //bool showConsoleWindow = true;
        //bool showProfilerWindow = true;
        //bool showSceneHierarchyWindow = true;
        //bool showInspectorWindow = true;
        //bool showRenderSettingsWindow = true;

        //void DrawSceneHierarchyWindow(bool* open);
        //void DrawInspectorWindow(bool* open);
        //void DrawProfilerWindow(bool* open);
        //void DrawConsoleWindow(bool* open);
        //void DrawRenderSettingsWindow(bool* open);
        //void DrawOverlay();
        //void DrawViewSettings();

        //float CalculateDeltaTime();

        //void OnKeyPressedEvent(KeyCode key, bool repeat);
        //void OnKeyReleasedEvent(KeyCode key);
        //void OnMouseButtonPressedEvent(MouseButtonID id);
        //void OnMouseButtonReleasedEvent(MouseButtonID id);

        //uint32 frameCounter = 0;
        float deltaTimeInSeconds = 0.0f;

        bool isExitRequested = false;

        HorizonEditorColorTheme colorTheme = HorizonEditorColorTheme::Light;
        RenderBackendTextureHandle fontTexture;

        ImGuiContext* imguiContext = nullptr;
        RenderSettings renderSettings;
        // RenderBackendType renderBackendType = RenderBackendType::Vulkan;
        RenderBackend* renderBackend = nullptr;
        // ShaderLibrary* shaderLibrary = nullptr;
        // RenderGraphResourcePool* renderGraphResourcePool;
        // RenderBackendGPUProfiler* gpuProfiler;
        // RendererDefaultResources* rendererDefaultResources;

        HorizonEngine* engine = nullptr;
        // RenderScene* renderScene;

        // Begin Scene View Window
        SceneView* sceneView;
        std::string currentDebugVisualizationMode;
        EditorCamera editorCamera;
        Point2D currentMousePosition;
        RealTimeRenderer* renderer;
        RenderGraphPersistentTexture* targetTexture;
        // End Scene View Window

        EditorSceneManager* editorSceneManager;

        Window* window = nullptr;
        RenderBackendSwapChainHandle swapChain = RenderBackendSwapChainHandle::Null;
        uint32 swapChainWidth = 0;
        uint32 swapChainHeight = 0;
    };
}

namespace Horizon::UI
{
    extern int s_UIContextID;
    extern uint32_t s_Counter;
    extern char s_IDBuffer[16];

    extern const char* GenerateID();
    extern void PushID();
    extern void PopID();
}

extern int HorizonEditorMain();