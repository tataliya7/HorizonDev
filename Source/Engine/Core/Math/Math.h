#pragma once

#include "Core/CoreCommon.h"

#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#define M_PI                   (3.1415926535897932f)
#define M_INV_PI              (0.3183098861837067f)
#define M_HALF_PI              (1.5707963267948966f)
#define M_TWO_PI              (6.2831853071795864f)
#define M_PI_SQUARED          (9.8696044010893580f)
#define M_SQRT_PI                (1.4142135623730950f)
#define SMALL_NUMBER          (1.e-8f)
#define KINDA_SMALL_NUMBER    (1.e-4f)
#define BIG_NUMBER              (3.4e+38f)
#define DELTA                  (0.00001f)
#define FLOAT_MIN              (1.175494351e-38f)
#define FLOAT_MAX              (3.402823466e+38f)

namespace HE
{
    //extern const float M_PI = 3.1415926535897932f;
    //extern const float M_INV_PI = 0.3183098861837067f;
    //extern const float M_HALF_PI = 1.5707963267948966f;
    //extern const float M_TWO_PI = 6.2831853071795864f;
    //extern const float M_PI_SQUARED = 9.8696044010893580f;
    //extern const float M_SQRT_PI = 1.4142135623730950f;
    //extern const float FLOAT_MIN = 1.175494351e-38f;
    //extern const float FLOAT_MAX = 3.402823466e+38f;
    //extern const float SMALL_NUMBER = 1.e-8f;
    //extern const float KINDA_SMALL_NUMBER = 1.e-4f;
    //extern const float DELTA = 3.4e+38f;
    //extern const float BIG_NUMBER = 0.00001f;

    using Vector2 = glm::vec2;
    using Vector3 = glm::vec3;
    using Vector4 = glm::vec4;

    using Matrix3x3 = glm::mat3;
    using Matrix4x4 = glm::mat4;

    using Vector2i = glm::ivec2;
    using Vector3i = glm::ivec3;
    using Vector4i = glm::ivec4;

    using Vector2u = glm::uvec2;
    using Vector3u = glm::uvec3;
    using Vector4u = glm::uvec4;

    using Vector2f32 = glm::fvec2;
    using Vector3f32 = glm::fvec3;
    using Vector4f32 = glm::fvec4;

    using Vector2f64 = glm::dvec2;
    using Vector3f64 = glm::dvec3;
    using Vector4f64 = glm::dvec4;

    using Matrix3x3f64 = glm::f64mat3x3;
    using Matrix4x4f64 = glm::f64mat4x4;

    using Quaternion = glm::quat;

    struct Point
    {
        int x;
        int y;
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
}

namespace HE::Math
{
    FORCEINLINE uint32 CeilDiv(uint32 x, uint32 y)
    {
        return ((x + y - 1) / y);
    }

    FORCEINLINE Vector4 GetPlane(Vector3 normal, Vector3 point)
    {
        return Vector4(normal.x, normal.y, normal.z, -glm::dot(normal, point));
    }

    FORCEINLINE Vector4 GetPlane(Vector3 a, Vector3 b, Vector3 c)
    {
        Vector3 normal = glm::normalize(glm::cross(b - a, c - a));
        return GetPlane(normal, a);
    }

    FORCEINLINE float Halton(int32 index, int32 base)
    {
        float result = 0.0f;
        float invBase = 1.0f / base;
        float fraction = invBase;
        while (index > 0)
        {
            result += (index % base) * fraction;
            index /= base;
            fraction *= invBase;
        }
        return result;
    }

    FORCEINLINE uint32 RoundUpToPowerOfTwo(uint32 value)
    {
        uint32 result = value;
        result--;
        result |= result >> 1;
        result |= result >> 2;
        result |= result >> 4;
        result |= result >> 8;
        result |= result >> 16;
        result++;
        return result;
    }

    FORCEINLINE uint32 RoundDownToPowerOfTwo(uint32 value)
    {
        uint32 result = value;
        result |= result >> 1;
        result |= result >> 2;
        result |= result >> 4;
        result |= result >> 8;
        result |= result >> 16;
        return result - (result >> 1);
    }

    FORCEINLINE bool IsPowerOfTwo(uint32 n)
    {
        return (n > 0) && ((n & (n - 1)) == 0);
    }

    FORCEINLINE bool IsPowerOfTwo(int32 n)
    {
        return (n > 0) && ((n & (n - 1)) == 0);
    }

    FORCEINLINE double Cos(double radians)
    {
        return cos(radians);
    }

    FORCEINLINE float Cos(float radians)
    {
        return cos(radians);
    }

    FORCEINLINE float Tan(float x)
    {
        return tan(x);
    }

    template <typename T>
    FORCEINLINE T Max(const T& a, const T& b)
    {
        return (a >= b) ? a : b;
    }

    template <typename T>
    FORCEINLINE T Min(const T& a, const T& b)
    {
        return (a <= b) ? a : b;
    }

    FORCEINLINE float Lerp(float x, float y, float t)
    {
        return x + (y - x) * t;
    }

    FORCEINLINE Vector3 Lerp(const Vector3& x, const Vector3& y, float t)
    {
        return x + (y - x) * t;
    }

    FORCEINLINE float Abs(float x)
    {
        return abs(x);
    }

    FORCEINLINE float Fmod(float x, float y)
    {
        return fmod(x, y);
    }

    FORCEINLINE float Square(float x)
    {
        return x * x;
    }

    FORCEINLINE Vector3 Normalize(const Vector3& v)
    {
        return glm::normalize(v);
    }

    FORCEINLINE float Length(const Vector3& v)
    {
        return glm::length(v);
    }

    FORCEINLINE float LengthSquared(const Vector3& v)
    {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    FORCEINLINE Matrix3x3 Transpose(const Matrix3x3& matrix)
    {
        return glm::transpose(matrix);
    }

    FORCEINLINE Matrix4x4 Transpose(const Matrix4x4& matrix)
    {
        return glm::transpose(matrix);
    }

    FORCEINLINE Matrix4x4 ScaleMatrix(const Vector3& scale)
    {
        return glm::scale(glm::mat4(1.0f), scale);
    }

    FORCEINLINE uint32 MaxNumMipLevels(uint32 size)
    {
        return 1 + uint32(std::floor(std::log2(size)));
    }

    FORCEINLINE uint32 MaxNumMipLevels(uint32 width, uint32 height)
    {
        return 1 + uint32(std::floor(std::log2(std::min(width, height))));
    }

    FORCEINLINE float Clamp(float x, float min = 0, float max = 1)
    {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    FORCEINLINE uint32 Clamp(uint32 x, uint32 min = 0, uint32 max = 1)
    {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    FORCEINLINE Vector3 Clamp(const Vector3& vec, float min = 0, float max = 1)
    {
        return Vector3(Clamp(vec[0]), Clamp(vec[1]), Clamp(vec[2]));
    }

    FORCEINLINE Vector3 Bezier3(float t, Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3)
    {
        t = Math::Clamp(t, 0.0f, 1.0f);
        float d = 1.0f - t;
        return d * d * d * p0 + 3.0f * d * d * t * p1 + 3.0f * d * t * t * p2 + t * t * t * p3;
    }

    FORCEINLINE float DotProduct(const Vector3& x, const Vector3& y)
    {
        return glm::dot(x, y);
    }

    FORCEINLINE Vector3 CrossProduct(const Vector3& x, const Vector3& y)
    {
        return glm::cross(x, y);
    }

    FORCEINLINE float DegreesToRadians(float x)
    {
        return glm::radians(x);
    }

    FORCEINLINE Vector3 DegreesToRadians(const Vector3& v)
    {
        return glm::radians(v);
    }

    FORCEINLINE float RadiansToDegrees(float x)
    {
        return glm::degrees(x);
    }

    FORCEINLINE Vector3 RadiansToDegrees(const Vector3& v)
    {
        return glm::degrees(v);
    }

    FORCEINLINE Vector3 EulerAnglesFromQuaternion(const Quaternion& quat)
    {
        return glm::eulerAngles(quat);
    }

    FORCEINLINE Quaternion QuaternionFromEulerAngles(const Vector3& euler)
    {
        return Quaternion(euler);
    }

    FORCEINLINE Quaternion QuaternionFromAngleAxis(float angle, const Vector3& axis)
    {
        return glm::angleAxis(angle, axis);
    }

    FORCEINLINE Quaternion ConvertMatrix4x4ToQuaternion(const Matrix4x4& matrix)
    {
        return glm::quat_cast(matrix);
    }

    FORCEINLINE Matrix3x3 Inverse(const Matrix3x3& matrix)
    {
        return glm::inverse(matrix);
    }

    FORCEINLINE Matrix4x4 Inverse(const Matrix4x4& matrix)
    {
        return glm::inverse(matrix);
    }

    FORCEINLINE Matrix4x4 PerspectiveReverseZ_RH_ZO(float fovy, float aspect, float zNear, float zFar)
    {
        return glm::perspectiveRH_ZO(fovy, aspect, zFar, zNear);
    };

    FORCEINLINE Matrix4x4 Compose(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
    {
        return glm::translate(glm::mat4(1), translation) * glm::mat4_cast(glm::normalize(rotation)) * glm::scale(glm::mat4(1.0f), scale);
    }

    FORCEINLINE void Decompose(const Matrix4x4& matrix, Vector3& outTranslation, Quaternion& outQuat, Vector3& outScale)
    {
        Vector3 skew;
        Vector4 perspective;
        glm::decompose(matrix, outScale, outQuat, outTranslation, skew, perspective);
    }

    FORCEINLINE void Decompose(const Matrix4x4& matrix, Vector3& outTranslation, Vector3& outRotation, Vector3& outScale)
    {
        Vector3 skew;
        Vector4 perspective;
        Quaternion quat;
        glm::decompose(matrix, outScale, quat, outTranslation, skew, perspective);
        outRotation = glm::degrees(glm::eulerAngles(quat));
    }

    FORCEINLINE bool RayTriangleIntersection(Vector3 rayOrigin, Vector3 rayDirection, Vector3 v0, Vector3 v1, Vector3 v2, float& outT)
    {
        Vector3 v0v1 = v1 - v0;
        Vector3 v0v2 = v2 - v0;
        Vector3 pvec = glm::cross(rayDirection, v0v2);
        float det = glm::dot(v0v1, pvec);
        det = det > 0.0f ? std::max(0.0000001f, det) : std::min(-0.0000001f, det);
        float det_rcp = 1.0f / det;//rcp(det);
        Vector3 tvec = rayOrigin - v0;
        Vector3 qvec = glm::cross(tvec, v0v1);
        float u = glm::dot(tvec, pvec) * det_rcp;
        float v = glm::dot(rayDirection, qvec) * det_rcp;
        outT = glm::dot(v0v2, qvec) * det_rcp;
        return (det >= 1e-6f && outT >= 0.0f && u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f);
    }
}
