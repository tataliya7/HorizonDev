#include "USDGemoXformableImporter.h"
#include "USDUtils.h"

#include "USDIncludeBegin.h"
#include <pxr/base/gf/math.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/usd/usdGeom/xform.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    USDGemoXformableImporter::USDGemoXformableImporter(const USDImportContext& context, const pxr::UsdPrim& prim)
        : USDPrimImporter(context, prim)
        , isRoot(false)
        , geomXformable(prim)
    {

    }

    bool USDGemoXformableImporter::IsRoot() const
    {
        return isRoot;
    }

    void USDGemoXformableImporter::GetLocalTransformation(Matrix4x4* transform, float time, float scale)
    {
        if (!geomXformable)
        {
            return;
        }

        pxr::GfMatrix4d pxrMat4d = {};
        bool resetsXformStack = false;
        geomXformable.GetLocalTransformation(&pxrMat4d, &resetsXformStack, time);

        // This explicit constructor converts a "double" matrix to a "float" matrix.
        pxr::GfMatrix4f pxrMat4f = pxr::GfMatrix4f(pxrMat4d);

        *transform = UsdToHorizon::ConvertMatrix(pxrMat4f);

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
        if (parent != nullptr)
        {
            scene->SetParent(entity, parent->GetEntity());
        }

        Matrix4x4 transform;
        float time = 0.0f;
        float scale = 1.0f;
        GetLocalTransformation(&transform, time, scale);

        Vector3 transformPosition;
        Vector3 transformRotation;
        Vector3 transformScale;
        Math::Decompose(transform, transformPosition, transformRotation, transformScale);

        TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(entity);
        transformComponent.position = transformPosition;
        transformComponent.rotation = transformRotation;
        transformComponent.scale = transformScale;
        scene->GetEntityManager()->AddOrReplaceComponent<TransformDirtyComponent>(entity);
    }
}