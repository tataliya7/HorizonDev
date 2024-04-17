#pragma once

#include "Core/CoreModule.h"
#include "Entity/EntityModule.h"

namespace HE
{
    struct UnitSettings
    {
        char lengthUnit;
        char massUnit;
        char timeUnit;
        char temperatureUnit;
    };

    class EntityManager;
    class PhysicsScene;
    class EngineSubsystem;

    struct SceneStats
    {
        uint64 meshCount = 0;
        uint64 meshInstanceCount = 0;
        uint64 transformCount = 0;
        uint64 triangleCount = 0;
        uint64 vertexCount = 0;
        uint64 instancedTriangleCount = 0;
        uint64 instancedVertexCount = 0;
        uint64 indexMemoryInBytes = 0;
        uint64 vertexMemoryInBytes = 0;
        uint64 geometryMemoryInBytes = 0;
        uint64 animationMemoryInBytes = 0;

        // Lights
        uint64 totalLightCount = 0;
        uint64 lightsMemoryInBytes = 0;
        uint64 environmentMapMemoryInBytes = 0;
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

        void Update(float deltaTime);

        void Clear();

        UnitSettings unit;

    private:
        friend class SceneSerializer;

        void OnRigidBodyComponentConstruct(entt::registry& registry, entt::entity entity);
        void OnRigidBodyComponentDestroy(entt::registry& registry, entt::entity entity);

        std::string name;//AssetID* id;
        EntityManager* entityManager;
        PhysicsScene* physicsScene;
        EngineSubsystem* renderEngine;
        bool shouldSimulate;
        bool shouldUpdateScripts;
    };
}