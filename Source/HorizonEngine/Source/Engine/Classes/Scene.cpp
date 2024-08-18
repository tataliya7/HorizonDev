#include "Scene.h"

#include "RenderSystem.h"
#include "Engine/Components/Components.h"

// TODO
#include "HorizonEngine.h"

namespace Horizon
{
    Scene::Scene(const std::string& name)
        : name(name)
    {
        entityManager = new EntityManager();

        // entityManager->OnConstruct<TransformComponent>().connect<&entt::registry::emplace_or_replace<TransformDirtyComponent>>();
        // entityManager->OnUpdate<TransformComponent>().connect<&entt::registry::emplace_or_replace<TransformDirtyComponent>>();
        //
        // entityManager->OnConstruct<SkyLightComponent>().connect<&Scene::OnSkyLightComponentConstruct>(this);
        // entityManager->OnDestroy<SkyLightComponent>().connect<&Scene::OnSkyLightComponentDestroy>(this);
        //
        // entityManager->OnConstruct<RigidBodyComponent>().connect<&Scene::OnRigidBodyComponentConstruct>(this);
        // entityManager->OnDestroy<RigidBodyComponent>().connect<&Scene::OnRigidBodyComponentDestroy>(this);

        //physicsScene = new PhysicsScene();

        RenderBackend* renderBackend = HorizonEngine::GetInstance()->GetSubsystem<RenderSystem>()->GetRenderBackend();

        renderScene = new RenderScene(renderBackend);
    }

    Scene::~Scene()
    {
        //delete physicsScene;
        delete entityManager;
    }

    EntityHandle Scene::CreateEntity(const std::string& name)
    {
        EntityHandle entity = entityManager->CreateEntity();
        entityManager->AddComponent<NameComponent>(entity, name);
        entityManager->AddComponent<TransformComponent>(entity);
        entityManager->AddComponent<SceneHierarchyComponent>(entity);
        return entity;
    }

    void Scene::SetParent(EntityHandle child, EntityHandle parent)
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

    // void Scene::Clear()
    // {
    //     entityManager->Clear();
    // }

    /*void Scene::OnSkyLightComponentConstruct(entt::registry& registry, entt::entity entity)
    {
        if (!hasSkyLight)
        {
            auto& component = entityManager->GetComponent<SkyLightComponent>(entity);
            component.renderObject = new SkyLightRenderObject(&component);
            renderScene->SetSkyLight(component.renderObject);
            hasSkyLight = true;
        }
        else
        {
            ASSERT(!hasSkyLight && "Can't create more than one SkyLightComponent!");
        }
    }

    void Scene::OnSkyLightComponentDestroy(entt::registry& registry, entt::entity entity)
    {

    }*/

    SceneSettings* Scene::GetSceneSettings()
    {
        return &settings;
    }

    // PhysicsScene* Scene::GetPhysicsScene() const
    // {
    //     return physicsScene;
    // }

    RenderScene* Scene::GetRenderScene() const
    {
        return renderScene;
    }

    // uint32 Scene::GetEntityCount() const
    // {
    //
    // }
    //
    // void Scene::OnRigidBodyComponentConstruct(entt::registry& registry, entt::entity entity)
    // {
    //     physicsScene->CreateActor(entityManager, entity);
    // }
    //
    // void Scene::OnRigidBodyComponentDestroy(entt::registry& registry, entt::entity entity)
    // {
    //     physicsScene->RemoveActor(entityManager, entity);
    // }
    //
    // static void UpdateTransform_Deprecated(EntityManager* manager, EntityHandle entity)
    // {
    //     const SceneHierarchyComponent& hierarchy = manager->GetComponent<SceneHierarchyComponent>(entity);
    //     TransformComponent& transform = manager->GetComponent<TransformComponent>(entity);
    //     transform.Update();
    //     if (hierarchy.parent != EntityHandle::Null)
    //     {
    //         TransformComponent& parentTransform = manager->GetComponent<TransformComponent>(hierarchy.parent);
    //         transform.localToWorldMatrix = transform.relativeTransform * parentTransform.localToWorldMatrix;
    //     }
    //     else
    //     {
    //         transform.localToWorldMatrix = transform.relativeTransform;
    //     }
    //     EntityHandle currentEntity = hierarchy.firstChild;
    //     for (uint32 i = 0; i < hierarchy.numChildren; i++)
    //     {
    //         if (manager->HasComponent<TransformDirtyComponent>(currentEntity))
    //         {
    //             continue;
    //         }
    //         UpdateTransform_Deprecated(manager, currentEntity);
    //         currentEntity = manager->GetComponent<SceneHierarchyComponent>(currentEntity).next;
    //     }
    //     manager->RemoveComponent<TransformDirtyComponent>(entity);
    // }
    //
    // struct UpdateTransformJobData
    // {
    //     EntityManager* manager;
    //     EntityHandle entity;
    // };
    //
    // static void UpdateTransform(void* data)
    // {
    //     EntityManager* manager = ((UpdateTransformJobData*)data)->manager;
    //     EntityHandle entity = ((UpdateTransformJobData*)data)->entity;
    //     const SceneHierarchyComponent& hierarchy = manager->GetComponent<SceneHierarchyComponent>(entity);
    //     TransformComponent& transform = manager->GetComponent<TransformComponent>(entity);
    //     transform.Update();
    //     EntityHandle currentEntity = hierarchy.firstChild;
    //     for (uint32 i = 0; i < hierarchy.numChildren; i++)
    //     {
    //         if (manager->HasComponent<TransformDirtyComponent>(currentEntity))
    //         {
    //             continue;
    //         }
    //         UpdateTransform(data);
    //         currentEntity = manager->GetComponent<SceneHierarchyComponent>(currentEntity).next;
    //     }
    //     manager->RemoveComponent<TransformDirtyComponent>(entity);
    // }

    void Scene::Tick(float deltaTimeInSeconds)
    {
        // if (ShouldUpdateScripts())
        // {
        //     // Update scripts
        //     entityManager->GetView<ScriptComponent>().each([&](EntityHandle entity, auto& script)
        //     {
        //         if (script.scriptable == nullptr)
        //         {
        //             script.ConstructorFunc();
        //             script.scriptable->manager = entityManager;
        //             script.scriptable->entity = entity;
        //             if (script.OnCreateFunc)
        //             {
        //                 script.OnCreateFunc(script.scriptable);
        //             }
        //         }
        //         if (script.OnUpdateFunc)
        //         {
        //             script.OnUpdateFunc(script.scriptable, deltaTime);
        //         }
        //     });
        // }
        //
        // if (ShouldSimulate())
        // {
        //     physicsScene->Simulate(deltaTime);
        // }
        //
        // // Update transforms
        // {
        //     entityManager->Get()->sort<TransformDirtyComponent>([&](EntityHandle lhs, EntityHandle rhs)
        //     {
        //         const auto& lc = entityManager->GetComponent<SceneHierarchyComponent>(lhs);
        //         const auto& rc = entityManager->GetComponent<SceneHierarchyComponent>(rhs);
        //         return lc.depth < rc.depth;
        //     });
        //
        //     entityManager->GetView<TransformDirtyComponent>().each([&](EntityHandle entity)
        //     {
        //         UpdateTransform_Deprecated(entityManager, entity);
        //     });

            /*uint32 numTransformsToUpdate = 0;
            std::vector<JobSystemJobDecl> updateTransformJobs;
            std::vector<UpdateTransformJobData> updateTransformJobData;

            entityManager->GetView<TransformDirtyComponent>().each([&](EntityHandle entity)
            {
                UpdateTransformJobData data = {
                    .manager = entityManager,
                    .entity = entity
                };
                updateTransformJobData.push_back(data);

                JobSystemJobDecl jobDecl = {
                    .jobFunc = UpdateTransform,
                    .data = &updateTransformJobData[numTransformsToUpdate]
                };
                updateTransformJobs.push_back(jobDecl);

                numTransformsToUpdate++;
            });

            JobSystemAtomicCounterHandle counter = JobSystemRunJobs(updateTransformJobs.data(), numTransformsToUpdate);
            JobSystemWaitForCounter(counter, 0);*/
        //}

        // Update armatures
        /*{
            entityManager->GetView<ArmatureComponent>().each([&](EntityHandle entity, ArmatureComponent& armature)
            {
                const TransformComponent& transform = entityManager->GetComponent<TransformComponent>(entity);

                if (armature.skinningMatrices.size() != armature.bones.size())
                {
                    armature.skinningMatrices.resize(armature.bones.size());
                }

                for (uint32 i = 0; i < armature.bones.size(); i++)
                {
                    EntityHandle entity = armature.bones[i];
                    const TransformComponent& boneTransform = entityManager->GetComponent<TransformComponent>(entity);
                    armature.skinningMatrices[i] = boneTransform.matrix * armature.inverseBindMatrices[i];
                }
            });
        }*/

        // Update cameras
        // {
        //     entityManager->GetView<CameraComponent>().each([&](EntityHandle entity, CameraComponent& camera)
        //     {
        //         const TransformComponent& transform = entityManager->GetComponent<TransformComponent>(entity);
        //         camera.position = transform.position;
        //         camera.rotation = Math::QuaternionFromEulerAngles(Math::DegreesToRadians(transform.rotation));
        //         camera.Update();
        //     });
        // }
        //
        // Update lights
        {
            entityManager->GetView<LightComponent>().each([&](EntityHandle entity, LightComponent& lightComponent)
            {
                const TransformComponent& transform = entityManager->GetComponent<TransformComponent>(entity);
                lightComponent.position = transform.position;
                lightComponent.direction = Math::Normalize(Vector3(Math::QuaternionFromEulerAngles(Math::DegreesToRadians(transform.rotation)) * Vector4(0.0, 0.0, -1.0, 0.0)));
                lightComponent.UpdateRenderObject();
            });
        }
        //
        // // Update meshes
        // {
        //     entityManager->GetView<MeshComponent>().each([&](EntityHandle entity, MeshComponent& mesh)
        //     {
        //         auto& transform = entityManager->GetComponent<TransformComponent>(entity);
        //         // mesh.worldMatrix = transform.world;
        //     });
        // }

        entityManager->GetView<SkyAtmosphereComponent>().each([&](EntityHandle entity)
        {
            SkyAtmosphereComponent& skyAtmosphereComponent = entityManager->GetComponent<SkyAtmosphereComponent>(entity);
            skyAtmosphereComponent.UpdateRenderObject();
        });

        // Update audio sources and listeners
        {

        }

        //renderScene->UpdateGPUScene();
    }
}