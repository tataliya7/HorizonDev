#include "TransformComponent.h"

namespace Horizon
{
    TransformComponent::TransformComponent()
        : position(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f)
        , scale(1.0f, 1.0f, 1.0f)
        , relativeTransform(Matrix4x4f(1.0f))
        , localToWorldMatrix(Matrix4x4f(1.0f)) {}
}