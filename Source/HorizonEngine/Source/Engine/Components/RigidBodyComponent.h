#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    struct RigidBodyComponent
    {
        enum class Type
        {
            Static = 0,
            Dynamic = 1,
        };

        enum class CollisionShape
        {
            Box = 0,
            Sphere = 1,
            Capsule = 2,
            Mesh = 3,
        };

        struct BoxCollider
        {
            Vector3 halfExtent = Vector3(0.5f, 0.5f, 0.5f);
            Vector3 offset = Vector3(0.0f, 0.0f, 0.0f);
        };

        struct SphereCollider
        {
            float radius = 1.0f;
        };

        struct CapsuleCollider
        {
            float radius = 1.0f;
            float height = 1.0f;
        };

        Type type = Type::Static;
        CollisionShape shape = CollisionShape::Mesh;

        BoxCollider boxCollider;
        SphereCollider sphereCollider;
        CapsuleCollider capsuleCollider;

        bool disableGravity = false;
        bool isKinematic = false;

        float mass = 1.0f;
        float linearDamping = 0.01f;
        float angularDamping = 0.05f;

        void SetBoxCollider(const Vector3& halfExtent, const Vector3& offset)
        {
            shape = CollisionShape::Box;
            boxCollider.halfExtent = halfExtent;
            boxCollider.offset = offset;
        }

        void SetSphereCollider(float radius)
        {
            shape = CollisionShape::Sphere;
            sphereCollider.radius = radius;
        }

        void SetCapsuleCollider(float radius, float height)
        {
            shape = CollisionShape::Capsule;
            capsuleCollider.radius = radius;
            capsuleCollider.height = height;
        }

        void SetMeshCollider()
        {
            shape = CollisionShape::Mesh;
        }

        RigidBodyComponent() = default;
        RigidBodyComponent(const RigidBodyComponent& other) = default;
    };
}