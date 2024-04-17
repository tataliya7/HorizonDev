#include "USDUtils.h"

namespace UsdTokens
{
    const pxr::TfToken st("st", pxr::TfToken::Immortal);
    const pxr::TfToken normals("normals", pxr::TfToken::Immortal);

    const pxr::TfToken UsdPreviewSurface("UsdPreviewSurface", pxr::TfToken::Immortal);
    const pxr::TfToken UsdUVTexture("UsdUVTexture", pxr::TfToken::Immortal);

    const pxr::TfToken file("file", pxr::TfToken::Immortal);

    const pxr::TfToken diffuseColor("diffuseColor", pxr::TfToken::Immortal);
    const pxr::TfToken metallic("metallic", pxr::TfToken::Immortal);
    const pxr::TfToken roughness("roughness", pxr::TfToken::Immortal);
    const pxr::TfToken emissiveColor("emissiveColor", pxr::TfToken::Immortal);
    const pxr::TfToken normal("normal", pxr::TfToken::Immortal);
}

namespace UsdToHorizon
{
    HE::Matrix4x4 ConvertMatrix(const pxr::GfMatrix4f& pxrMat4f)
    {
        return HE::Matrix4x4(
            pxrMat4f[0][0], pxrMat4f[1][0], pxrMat4f[2][0], pxrMat4f[3][0],
            pxrMat4f[0][1], pxrMat4f[1][1], pxrMat4f[2][1], pxrMat4f[3][1],
            pxrMat4f[0][2], pxrMat4f[1][2], pxrMat4f[2][2], pxrMat4f[3][2],
            pxrMat4f[0][3], pxrMat4f[1][3], pxrMat4f[2][3], pxrMat4f[3][3]);
    }
}