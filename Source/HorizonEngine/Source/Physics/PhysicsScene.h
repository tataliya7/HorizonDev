#pragma once

#include "Core/CoreModule.h"
#include "Entity/EntityModule.h"
#include "Physics/PhysicsCore.h"

namespace physx
{
    class PxRigidActor;
    class PxScene;
    class PxMaterial;
}

namespace Horizon
{
    class PhysicsActor
    {
    public:
        void SynchronizeTransform();
        EntityManager* manager;
        EntityHandle entity;
        physx::PxRigidActor* physxActor;
    };

    class PhysicsScene
    {
    public:
        PhysicsScene();
        ~PhysicsScene();

        void CreateActor(EntityManager* manager, EntityHandle entity);

        void RemoveActor(EntityManager* manager, EntityHandle entity)
        {

        }

        void Simulate(float deltaTime);

        void Clear()
        {

        }

        void SetGravity(const Vector3& gravity);
        Vector3 GetGravity();

    private:
        physx::PxScene* physxScene;
        physx::PxMaterial* material;
    };
}