#include "USDGemoXformableImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/base/gf/math.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/usd/usdGeom/xform.h>
#include "USDIncludeEnd.h"

namespace HE::USDImporter
{
    USDGemoXformableImporter::USDGemoXformableImporter(const USDImportContext& context, const pxr::UsdPrim& prim)
        : USDPrimImporter(context, prim)
        , isRoot(false)
    {

    }

    bool USDGemoXformableImporter::IsRoot() const
    {
        return isRoot;
    }

    void USDGemoXformableImporter::GetLocalTransformation(Matrix4x4* transform, float time, float scale)
    {
        pxr::UsdGeomXformable xformable = pxr::UsdGeomXformable(prim);
        if (!xformable)
        {
            return;
        }

        pxr::GfMatrix4d pxrMat4d = {};
        bool resetsXformStack = false;
        xformable.GetLocalTransformation(&pxrMat4d, &resetsXformStack, time);

        // This explicit constructor converts a "double" matrix to a "float" matrix.
        pxr::GfMatrix4f pxrMat4f = pxr::GfMatrix4f(pxrMat4d);

        // pxr::GfMatrix4f: row-major, Matrix4x4:: column-major
        *transform = Matrix4x4(
            pxrMat4f[0][0], pxrMat4f[1][0], pxrMat4f[2][0], pxrMat4f[3][0],
            pxrMat4f[0][1], pxrMat4f[1][1], pxrMat4f[2][1], pxrMat4f[3][1],
            pxrMat4f[0][2], pxrMat4f[1][2], pxrMat4f[2][2], pxrMat4f[3][2],
            pxrMat4f[0][3], pxrMat4f[1][3], pxrMat4f[2][3], pxrMat4f[3][3]);

        // Apply scaling and rotation only to root xformables.
        if (IsRoot() && (scale != 1.0))
        {
            Matrix4x4 scaleMatrix = Math::ScaleMatrix(Vector3(scale, scale, scale));
            *transform = (*transform) * scaleMatrix;
        }
    }

    void USDGemoXformableImporter::CreateEntity(Scene* scene)
    {
        entity = scene->CreateEntity(GetName());
    }

    void USDGemoXformableImporter::AddComponents(Scene* scene)
    {

    }
}