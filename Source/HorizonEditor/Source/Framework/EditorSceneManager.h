#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    class EditorSceneManager
    {
    public:
        Scene* CreateScene(const std::string& name);
        void DestroyScene(Scene* scene);
        Scene* GetActiveScene() const;
        Scene* GetSceneByName(const std::string& name);
        void SetActiveScene(Scene* scene);
        void OpenScene(const std::string& name);
        void CloseScene(Scene* scene);
        bool SaveScene(Scene* scene, std::string path);
    private:
        uint32 loadedSceneCount = 0;
        Scene* activeScene = nullptr;
        std::unordered_map<std::string, Scene*> scenes;
    };
}