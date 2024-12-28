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

    }

    void USDSkelRootImporter::AddComponents(Scene* scene)
    {

    }
}