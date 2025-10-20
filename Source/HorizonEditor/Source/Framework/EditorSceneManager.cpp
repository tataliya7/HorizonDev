#include "EditorSceneManager.h"

namespace Horizon
{
    Scene* EditorSceneManager::CreateScene(const std::string& name)
    {
        if (GetSceneByName(name) != nullptr)
        {
            return nullptr;
        }
        Scene* scene = new Scene(name);
        scenes[name] = scene;
        loadedSceneCount++;
        return scene;
    }

    void EditorSceneManager::DestroyScene(Scene* scene)
    {
        assert(scene != nullptr);
        const std::string& name = scene->GetName();
        if (GetSceneByName(name))
        {
            delete scenes[name];
            scenes[name] = nullptr;
            loadedSceneCount--;
        }
    }

    void EditorSceneManager::SetActiveScene(Scene* scene)
    {
        assert(scene != nullptr);
        assert(GetSceneByName(scene->GetName()) != nullptr);
        activeScene = scene;
    }

    void EditorSceneManager::OpenScene(const std::string& name)
    {

    }

    Scene* EditorSceneManager::GetActiveScene() const
    {
        return activeScene;
    }

    Scene* EditorSceneManager::GetSceneByName(const std::string& name)
    {
        if (!scenes.contains(name))
        {
            return nullptr;
        }
        return scenes[name];
    }

    //
    //     void SceneManager::LoadScene(const std::string& name)
    //     {
    //         if (SceneMapByName.find(name) == SceneMapByName.end())
    //         {
    //             return;
    //         }
    //     }
    //
    //     void SceneManager::LoadSceneAsync(const std::string& name)
    //     {
    //         // TODO
    //     }
    //
    //     void SceneManager::UnloadSceneAsync(Scene* scene)
    //     {
    //         // TODO
    //     }
    //
    //     void SceneManager::MergeScenes(Scene* dstScene, Scene* srcScene)
    //     {
    //         // TODO
    //     }
}