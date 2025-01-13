#pragma once

#include "Foundation/StdHeaders.h"
#include "Foundation/Definitions.h"
#include "Foundation/FundamentalTypes.h"

namespace Horizon
{
    template <typename T, uint64 N>
    FORCEINLINE constexpr uint64 ArraySize(T(&array)[N])
    {
        return N;
    }

    const float MetersToKilometers = 0.001f;
    const float KilometersToMeters = 1000.0f;

    const Vector3f DefaultLightDirection = Vector3f(0.0f, 0.0f, -1.0f);

    struct Point2D
    {
        int32 x;
        int32 y;
    };

    struct Point3D
    {
        int32 x;
        int32 y;
        int32 z;
    };

    struct Rect
    {
        // offset
        int32 x;
        int32 y;

        // extent
        uint32 width;
        uint32 height;
    };

    class Box
    {
    public:
        explicit constexpr Box()
            : minimumPoint(0)
            , maximumPoint(0)
        {

        }
        Vector3f GetExtent() const
        {
            return (maximumPoint - minimumPoint);
        }
    private:
        Vector3f minimumPoint;
        Vector3f maximumPoint;
    };

    struct Extent2D
    {
        uint32 width;
        uint32 height;

        Extent2D() : width(0), height(0) {}
        explicit Extent2D(uint32 w, uint32 h) : width(w), height(h) {}
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

        Offset2D() : x(0), y(0) {}
        explicit Offset2D(int32 x, int32 y) : x(x), y(y) {}
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

        Bounds3D(const Vector3f& minimum, const Vector3f& maximum) : minimum(minimum), maximum(maximum) {}

        Bounds3D(const Bounds3D& other)
        {
            minimum = other.minimum;
            maximum = other.maximum;
        }

        Bounds3D& operator=(const Bounds3D& other)
        {
            minimum = other.minimum;
            maximum = other.maximum;
            return *this;
        }

        Vector3f minimum;
        Vector3f maximum;
    };

    struct Sphere
    {
        Vector3f position;
        float radius;
    };

    struct Plane
    {
        Vector3f normal;
        float distance;

        Plane() = default;

        FORCEINLINE Plane(const Vector3f& normal, const Vector3f& point)
            : normal(normal)
            , distance(-glm::dot(normal, point))
        {

        }

        FORCEINLINE Plane(const Vector3f& a, const Vector3f& b, const Vector3f& c)
            : normal(glm::normalize(glm::cross(b - a, c - a)))
            , distance(-glm::dot(normal, a))
        {

        }
    };

    struct Frustum
    {
        Plane planes[6];
    };

    // @todo SIMD optimization
    inline bool FrustumSphereIntersectionTest(const Frustum& frustum, const Sphere& sphere)
    {
        for (uint32 i = 0; i < 6; i++)
        {
            Vector3f normal = frustum.planes[i].normal;
            float dist = frustum.planes[i].distance;
            float side = Math::DotProduct(sphere.position, normal) + dist;
            if (side < -sphere.radius)
            {
                return false;
            }
        }

        return true;
    }
}