#pragma once

#include "USD.h"
#include "USDImportContext.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usdSkel/skeleton.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    class USDSkelSkeletonImporter
    {
    public:
        USDSkelSkeletonImporter(USDImportContext& context);
        Skeleton* ImportSkeleton(const pxr::UsdSkelSkeleton& usdSkelSkeleton);
    private:
        USDImportContext* context;
    };
}