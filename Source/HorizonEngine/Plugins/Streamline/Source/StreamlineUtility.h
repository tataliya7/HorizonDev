#pragma once

#include "Foundation/FoundationModule.h"

#include <sl.h>
#include <sl_helpers.h>

#define STREAMLINE_CHECK(f) assert((f) == sl::Result::eOk)

static inline sl::float4x4 ToSL(const Horizon::Matrix4x4f& matrix)
{
    sl::float4x4 result;
    result.setRow(0, sl::float4(matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3]));
    result.setRow(1, sl::float4(matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3]));
    result.setRow(2, sl::float4(matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3]));
    result.setRow(3, sl::float4(matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]));
    return result;
};

static inline sl::float4 ToSL(const Horizon::Vector4f& vector)
{
    return sl::float4(vector.x, vector.y, vector.z, vector.w);
};

static inline sl::float3 ToSL(const Horizon::Vector3f& vector)
{
    return sl::float3(vector.x, vector.y, vector.z);
};

static inline sl::float2 ToSL(const Horizon::Vector2f& vector)
{
    return sl::float2(vector.x, vector.y);
};

static inline sl::Boolean ToSL(bool b)
{
    return b ? sl::eTrue : sl::eFalse;
};

// static inline sl::Extent ToSL(const Horizon::Rect& rect)
// {
//     sl::Extent result =
//     {
//         .top = rect.top,
//         .left = rect.left,
//         .width = rect.GetWidth(),
//         .height = rect.GetHeight(),
//     };
//     return result;
// };