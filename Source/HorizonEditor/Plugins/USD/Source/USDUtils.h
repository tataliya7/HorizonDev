#pragma once

#include "USD.h"

#include "USDIncludeBegin.h"
#include <pxr/pxr.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/gf/matrix4f.h>
#include "USDIncludeEnd.h"

namespace UsdTokens
{
    extern const pxr::TfToken st;
    extern const pxr::TfToken normals;
    extern const pxr::TfToken UsdPreviewSurface;
    extern const pxr::TfToken UsdUVTexture;
    extern const pxr::TfToken file;
    extern const pxr::TfToken diffuseColor;
    extern const pxr::TfToken metallic;
    extern const pxr::TfToken roughness;
    extern const pxr::TfToken emissiveColor;
    extern const pxr::TfToken normal;
}

namespace UsdToHorizon
{
    Horizon::Matrix4x4 ConvertMatrix(const pxr::GfMatrix4f& pxrMat4f);
}