#pragma once

#include "USD.h"
#include "USDImportContext.h"
#include "USDGemoXformableImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/binding.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/blendShapeQuery.h>
#include <pxr/usd/usdSkel/skeletonQuery.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    class USDSkelRootImporter : public USDGemoXformableImporter
    {
    public:
        USDSkelRootImporter(const USDImportContext& context, const pxr::UsdPrim& prim);

        void CreateEntity(Scene* scene) override;
        void AddComponents(Scene* scene) override;

    private:
        pxr::UsdSkelRoot skelRoot;
    };
}