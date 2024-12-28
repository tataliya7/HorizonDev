#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    class TransformComponent
    {
    public:

        TransformComponent();

        Vector3f position;
        Vector3f rotation;
        Vector3f scale;
        Matrix4x4f relativeTransform;
        Matrix4x4f localToWorldMatrix;

        void Update()
        {
            relativeTransform = Math::ComposeTransformationMatrix(position, Quaternion(Math::DegreesToRadians(rotation)), scale);
        }

        bool operator==(const TransformComponent& other) const
        {
            if (position != other.position)
            {
                return false;
            }
            if (rotation != other.rotation)
            {
                return false;
            }
            if (scale != other.scale)
            {
                return false;
            }
            return true;
        }

        bool operator!=(const TransformComponent& other) const
        {
            return !((*this) == other);
        }
    };
}