#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"
//#include "Physics/PhysicsModule.h"
#include "Engine/ECS/EntityManager.h"
#include "Engine/Serialization/SerializationModule.h"

namespace Horizon
{
    class EntityManager;

    struct SceneSettings
    {
        float gravity;
    };

    class Scene
    {
    public:

        Scene(const std::string& name);

        ~Scene();

        const std::string& GetName() const
        {
            return name;
        }

        EntityHandle CreateEntity(const std::string& name);

        void DestroyEntity(EntityHandle entity)
        {
            entityManager->DestroyEntity(entity);
        }

        EntityManager* GetEntityManager()
        {
            return entityManager;
        }

        void SetParent(EntityHandle child, EntityHandle parent);

        bool enablePhysicsSimulation;

        bool shouldUpdateScripts;

        SceneSettings* GetSceneSettings();

        /**
         * Returns a pointer to the physics scene for this scene.
         */
        //PhysicsScene* GetPhysicsScene() const;

        /**
         * Returns a pointer to the render scene for this scene.
         */
        RenderScene* GetRenderScene() const;

        /**
         * Returns the entity count.
         */
        uint32 GetEntityCount() const;

        void Tick(float deltaTimeInSeconds);

        void Serialize(Archive& archive);

    private:

        void OnRigidBodyComponentConstruct(entt::registry& registry, entt::entity entity);
        void OnRigidBodyComponentDestroy(entt::registry& registry, entt::entity entity);

        std::string name;//AssetID* id;

        SceneSettings settings;

        EntityManager* entityManager;

        RenderScene* renderScene;

        //PhysicsScene* physicsScene;

        bool paused;
    };
}