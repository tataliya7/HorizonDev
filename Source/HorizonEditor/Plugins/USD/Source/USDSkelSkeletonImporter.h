#pragma once

#include "USD.h"
#include "USDImportContext.h"
#include "USDGemoXformableImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usdSkel/skeleton.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    class USDSkelSkeletonImporter : public USDGemoXformableImporter
    {
    public:
        USDSkelSkeletonImporter(const USDImportContext& context, const pxr::UsdPrim& prim)
            : USDGemoXformableImporter(context, prim)
            , skeleton(prim)
        {

        }
    private:
        pxr::UsdSkelSkeleton skeleton;
    };

    void ImportSkeletonBinding(const pxr::UsdPrim& prim);
}