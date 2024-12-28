#include "USDPrimImporter.h"

namespace Horizon::USDImporter
{
    USDPrimImporter::USDPrimImporter(const USDImportContext& context, const pxr::UsdPrim& prim)
        : context(&context)
        , prim(prim)
        , parent(nullptr)
        , entity()
    {

    }

    USDPrimImporter::~USDPrimImporter()
    {

    }

    const std::string& USDPrimImporter::GetName() const
    {
        return prim.GetName().GetString();
    }

    const std::string& USDPrimImporter::GetPrimPath() const
    {
        return prim.GetPrimPath().GetString();
    }
}