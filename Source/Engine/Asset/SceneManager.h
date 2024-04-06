#pragma once

#include "Core/CoreModule.h"

namespace HE
{
    class Scene;

    /**
     * Scene management at runtime.
     */
    class SceneManager
    {
    public:
        static uint32 LoadedSceneCount;
        static Scene* ActiveScene;
        static std::map<std::string, std::shared_ptr<Scene>> SceneMapByName;
        static Scene* CreateScene(const std::string& name);
        static void DestroyScene(Scene* scene);
        static Scene* GetActiveScene();
        static Scene* GetSceneByName(const std::string& name);
        static void SetActiveScene(Scene* scene);
        static void LoadScene(const std::string& name);
        static void LoadSceneAsync(const std::string& name);
        static void UnloadSceneAsync(Scene* scene);
        static void MergeScenes(Scene* dstScene, Scene* srcScene);
    };
}