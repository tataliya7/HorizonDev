#include "USDSkelRootImporter.h"

namespace Horizon::USDImporter
{
    USDSkelRootImporter::USDSkelRootImporter(const USDImportContext& context, const pxr::UsdPrim& prim)
        : USDGemoXformableImporter(context, prim)
        , skelRoot(prim)
    {

    }

    void USDSkelRootImporter::CreateEntity(Scene* scene)
    {
        entity = scene->CreateEntity(GetName());
    }

    void USDSkelRootImporter::AddComponents(Scene* scene)
    {
        if (parent != nullptr)
        {
            scene->SetParent(entity, parent->GetEntity());
        }

        Matrix4x4f transform;
        float time = 0.0f;
        float scale = 1.0f;
        GetLocalTransformation(&transform, time, scale);

        Vector3f transformPosition;
        Vector3f transformRotation;
        Vector3f transformScale;
        Math::DecomposeTransformationMatrix(transform, transformPosition, transformRotation, transformScale);

        TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(entity);
        transformComponent.position = transformPosition;
        transformComponent.rotation = transformRotation;
        //transformComponent.scale = transformScale;
    }
}