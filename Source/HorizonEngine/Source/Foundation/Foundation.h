#pragma once

#include "Foundation/Math.h"

namespace Horizon
{
    using Vector2 = Vector2f;
    using Vector3 = Vector3f;
    using Vector4 = Vector4f;

    using Matrix3x3 = Matrix3x3f;
    using Matrix4x4 = Matrix3x3f;

    template <typename T, uint64 N>
    FORCEINLINE constexpr uint64 ArraySize(T(&array)[N])
    {
        return N;
    }

    const float MetersToKilometers = 0.001f;
    const float KilometersToMeters = 1000.0f;

    const Vector3 DefaultLightDirection = Vector3(0.0f, 0.0f, -1.0f);

    class Plane
    {

    };

    struct Frustum
    {
        Vector4 planes[6];
    };

    struct Point2D
    {
        int x;
        int y;
    };

    struct Point3D
    {
        int x;
        int y;
        int z;
    };

    struct Rect
    {
        int left;
        int top;
        int right;
        int bottom;

        Rect() : left(0), top(0), right(0), bottom(0) {}
        Rect(int x0, int y0, int x1, int y1) : left(x0), top(y0), right(x1), bottom(y1) {}

        uint32 GetWidth() const { return right - left; }
        uint32 GetHeight() const { return bottom - top; }
    };

    struct Extent2D
    {
        uint32 width;
        uint32 height;

        Extent2D() {}
        Extent2D(uint32 w, uint32 h) : width(w), height(h) {}
    };

    struct Extent3D
    {
        uint32 width;
        uint32 height;
        uint32 depth;
    };

    struct Offset2D
    {
        int32 x;
        int32 y;
    };

    struct Offset3D
    {
        int32 x;
        int32 y;
        int32 z;
    };

    class Bounds3D
    {
    public:

        Bounds3D() {}

        Bounds3D(const Vector3& minimum, const Vector3& maximum) : minimum(minimum), maximum(maximum) {}

        Bounds3D(const Bounds3D& other)
        {
            minimum = other.minimum;
            maximum = other.maximum;
        }

        void operator=(const Bounds3D& other)
        {
            minimum = other.minimum;
            maximum = other.maximum;
        }

        Vector3 minimum;
        Vector3 maximum;
    };
}
