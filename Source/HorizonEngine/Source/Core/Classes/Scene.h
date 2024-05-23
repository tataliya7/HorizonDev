#pragma once

#include "Core/CoreCommon.h"
#include "Core/Serialization/SerializationModule.h"

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

        EntityHandle CreateEntity(const std::string& name)
        {
            EntityHandle entity = entityManager->CreateEntity();
            entityManager->AddComponent<NameComponent>(entity, name);
            entityManager->AddComponent<TransformComponent>(entity);
            entityManager->AddComponent<SceneHierarchyComponent>(entity);
            return entity;
        }

        void DestroyEntity(EntityHandle entity)
        {
            entityManager->DestroyEntity(entity);
        }

        EntityManager* GetEntityManager()
        {
            return entityManager;
        }

        void SetParent(EntityHandle child, EntityHandle parent)
        {
            if (parent != EntityHandle::Null)
            {
                SceneHierarchyComponent& childHierarchyComponent = entityManager->GetComponent<SceneHierarchyComponent>(child);
                SceneHierarchyComponent& parentHierarchyComponent = entityManager->GetComponent<SceneHierarchyComponent>(parent);

                childHierarchyComponent.depth = parentHierarchyComponent.depth + 1;
                childHierarchyComponent.parent = parent;
                childHierarchyComponent.next = parentHierarchyComponent.firstChild;

                if (parentHierarchyComponent.firstChild != EntityHandle::Null)
                {
                    SceneHierarchyComponent& siblingHierarchyComponent = entityManager->GetComponent<SceneHierarchyComponent>(parentHierarchyComponent.firstChild);
                    siblingHierarchyComponent.prev = child;
                }

                parentHierarchyComponent.firstChild = child;
                parentHierarchyComponent.numChildren += 1;
            }
        }

        bool ShouldSimulate() const
        {
            return shouldSimulate;
        }

        bool ShouldUpdateScripts() const
        {
            return shouldUpdateScripts;
        }

        void SetShouldSimulate(bool value)
        {
            shouldSimulate = value;
        }

        void SetShouldUpdateScripts(bool value)
        {
            shouldUpdateScripts = value;
        }

        SceneSettings* GetSceneSettings() const;

        /**
         * Returns a pointer to the physics scene for this scene.
         */
        PhysicsScene* GetPhysicsScene() const;

        /**
         * Returns a pointer to the render scene for this scene.
         */
        RenderScene* GetRenderScene() const;

        /**
         * Returns the entity count.
         */
        uint32 GetEntityCount() const;

        void Serialize(Archive& archive);

    private:

        void OnRigidBodyComponentConstruct(entt::registry& registry, entt::entity entity);
        void OnRigidBodyComponentDestroy(entt::registry& registry, entt::entity entity);

        std::string name;//AssetID* id;

        SceneSettings settings;

        EntityManager* entityManager;

        PhysicsScene* physicsScene;

        RenderScene* renderScene;

        bool enablePhysicsSimulation;

        bool shouldUpdateScripts;

        bool paused;
    };
}