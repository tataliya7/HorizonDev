#pragma once

namespace Horizon
{
    class TransformComponent
    {
    public:

        TransformComponent();

        Vector3 position;
        Vector3 rotation;
        Vector3 scale;
        Matrix4x4 relativeTransform;
        Matrix4x4 localToWorldMatrix;

        void Update()
        {
            relativeTransform = Math::Compose(position, Quaternion(Math::DegreesToRadians(rotation)), scale);
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
