#include "Scene.h"

#include "RenderSystem.h"
#include "Engine/Components/Components.h"

// TODO
#include "HorizonEngine.h"

#include <optick.h>

namespace Horizon
{
    Scene::Scene(const std::string& name)
        : name(name)
    {
        entityManager = new EntityManager();

        // entityManager->OnConstruct<TransformComponent>().connect<&entt::registry::emplace_or_replace<TransformDirtyComponent>>();
        // entityManager->OnUpdate<TransformComponent>().connect<&entt::registry::emplace_or_replace<TransformDirtyComponent>>();

        // entityManager->OnConstruct<SkyLightComponent>().connect<&Scene::OnSkyLightComponentConstruct>(this);
        // entityManager->OnDestroy<SkyLightComponent>().connect<&Scene::OnSkyLightComponentDestroy>(this);

        // entityManager->OnConstruct<RigidBodyComponent>().connect<&Scene::OnRigidBodyComponentConstruct>(this);
        // entityManager->OnDestroy<RigidBodyComponent>().connect<&Scene::OnRigidBodyComponentDestroy>(this);

        //physicsScene = new PhysicsScene();

        RenderSystem* renderSystem = HorizonEngine::GetInstance()->GetSubsystem<RenderSystem>();
        RenderBackend* renderBackend = renderSystem->GetRenderBackend();
        ShaderRepository* shaderRepository = renderSystem->GetShaderRepository();

        renderScene = new RenderScene(renderBackend, shaderRepository);

        if (renderSystem->enableHardwareRayTracing)
        {
            renderScene->CreateRayTracingScene();
        }
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
    static void UpdateTransform_Deprecated(EntityManager* manager, EntityHandle entity)
    {
        const SceneHierarchyComponent& hierarchy = manager->GetComponent<SceneHierarchyComponent>(entity);
        TransformComponent& transform = manager->GetComponent<TransformComponent>(entity);
        transform.Update();
        if (hierarchy.parent != EntityHandle::Null)
        {
            TransformComponent& parentTransform = manager->GetComponent<TransformComponent>(hierarchy.parent);
            transform.localToWorldMatrix = transform.relativeTransform * parentTransform.localToWorldMatrix;
        }
        else
        {
            transform.localToWorldMatrix = transform.relativeTransform;
        }
        // EntityHandle currentEntity = hierarchy.firstChild;
        // for (uint32 i = 0; i < hierarchy.numChildren; i++)
        // {
        //     if (manager->HasComponent<TransformDirtyComponent>(currentEntity))
        //     {
        //         continue;
        //     }
        //     UpdateTransform_Deprecated(manager, currentEntity);
        //     currentEntity = manager->GetComponent<SceneHierarchyComponent>(currentEntity).next;
        // }
    }

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
        OPTICK_EVENT();
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

        std::array<uint32, 10000> a = {};

        JobSystemJobCounterReference job1 = JobSystemRunJob(
            "Job1",
            JobSystemJobPriority::High,
            JobSystemJobCounterReference::Null,
            [&a](const JobSystemJobContext& jobContext)
            {
                OPTICK_EVENT("Job1");
                for (uint32 i = 0; i < 10000 / 2; i++)
                {
                    a[i] = i;
                }
            });

        JobSystemJobCounterReference job2 = JobSystemRunJob(
            "Job2",
            JobSystemJobPriority::High,
            JobSystemJobCounterReference::Null,
            [&a](const JobSystemJobContext& jobContext)
            {
                OPTICK_EVENT("Job2");
                for (uint32 i = 10000 / 2; i < 10000; i++)
                {
                    a[i] = i;
                }
            });

        std::array<JobSystemJobCounterReference, 2> joba = { job1, job2 };
        JobSystemJobCounterReference job12 = JobSystemCombineDependencies(joba.data(), joba.size());

        JobSystemJobCounterReference job3 = JobSystemRunJob(
            "Job3",
            JobSystemJobPriority::High,
            job12,
            [&a](const JobSystemJobContext& jobContext)
            {
                OPTICK_EVENT("Job3");
                for (uint32 i = 0; i < 10000 / 2; i++)
                {
                    a[i] += a[i + 10000 / 2];
                }
            });

        JobSystemWaitForCounter(job3);

        // Update transforms
        JobSystemJobCounterReference transformComponentUpdateJob = JobSystemRunJob(
            "UpdateTransformComponents",
            JobSystemJobPriority::High,
            JobSystemJobCounterReference::Null,
            [this](const JobSystemJobContext& jobContext)
            {
                OPTICK_EVENT("UpdateTransformComponents");
                entityManager->Get()->sort<TransformComponent>([&](EntityHandle lhs, EntityHandle rhs)
                {
                    const SceneHierarchyComponent& lc = entityManager->GetComponent<SceneHierarchyComponent>(lhs);
                    const SceneHierarchyComponent& rc = entityManager->GetComponent<SceneHierarchyComponent>(rhs);
                    return lc.depth < rc.depth;
                });

                entityManager->GetView<TransformComponent>().each([&](EntityHandle entity)
                {
                    UpdateTransform_Deprecated(entityManager, entity);
                });
            });

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

        JobSystemWaitForCounter(transformComponentUpdateJob);

        JobSystemJobCounterReference lightComponentUpdateJob = JobSystemRunJob(
            "UpdateLightComponents",
            JobSystemJobPriority::High,
            transformComponentUpdateJob,
            [this](const JobSystemJobContext& jobContext)
            {
                OPTICK_EVENT("UpdateLightComponents");
                entityManager->GetView<LightComponent>().each([&](EntityHandle entity, LightComponent& lightComponent)
                {
                    const TransformComponent& transform = entityManager->GetComponent<TransformComponent>(entity);
                    lightComponent.position = transform.position;
                    lightComponent.direction = Math::Normalize(Vector3f(Math::QuaternionFromEulerAngles(Math::DegreesToRadians(transform.rotation)) * Vector4f(0.0f, 0.0f, -1.0f, 0.0f)));
                    lightComponent.UpdateRenderObject();
                });
            });

        JobSystemJobCounterReference meshComponentUpdateJob = JobSystemRunJob(
            "UpdateMeshComponents",
            JobSystemJobPriority::High,
            transformComponentUpdateJob,
            [this, deltaTimeInSeconds](const JobSystemJobContext& jobContext)
            {
                OPTICK_EVENT("UpdateMeshComponents");
                entityManager->GetView<MeshComponent>().each([&](EntityHandle entity, MeshComponent& meshComponent)
                {
                    const TransformComponent& transform = entityManager->GetComponent<TransformComponent>(entity);
                    meshComponent.localToWorldMatrix = transform.localToWorldMatrix;
                    meshComponent.UpdateRenderObject();

                    if (meshComponent.skeleton)
                    {
                        meshComponent.UpdatePose(deltaTimeInSeconds);
                    }
                });
            });

        entityManager->GetView<SkyAtmosphereComponent>().each([&](EntityHandle entity)
        {
            SkyAtmosphereComponent& skyAtmosphereComponent = entityManager->GetComponent<SkyAtmosphereComponent>(entity);
            skyAtmosphereComponent.UpdateRenderObject();
        });

        entityManager->GetView<GlobalFogComponent>().each([&](EntityHandle entity)
        {
            GlobalFogComponent& globalFogComponent = entityManager->GetComponent<GlobalFogComponent>(entity);
            globalFogComponent.UpdateRenderObject();
        });

        entityManager->GetView<LocalFogVolumeComponent>().each([&](EntityHandle entity)
        {
            const TransformComponent& transformComponent = entityManager->GetComponent<TransformComponent>(entity);

            LocalFogVolumeComponent& localFogVolumeComponent = entityManager->GetComponent<LocalFogVolumeComponent>(entity);
            localFogVolumeComponent.UpdateRenderObject(transformComponent.localToWorldMatrix);
        });

        // Update audio sources and listeners
        {

        }

        JobSystemWaitForCounter(lightComponentUpdateJob);
        JobSystemWaitForCounter(meshComponentUpdateJob);
        //renderScene->UpdateGPUScene();
    }
}