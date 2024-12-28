#pragma once

#include "USD.h"
#include "USDImportContext.h"
#include "USDPrimImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usdGeom/xformable.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    class USDGemoXformableImporter : public USDPrimImporter
    {
    public:

        USDGemoXformableImporter(const USDImportContext& context, const pxr::UsdPrim& prim);

        void GetLocalTransformation(Matrix4x4f* transform, float time, float scale);

        void CreateEntity(Scene* scene) override;
        void AddComponents(Scene* scene) override;

    protected:

        bool IsRoot() const;

    private:

        pxr::UsdGeomXformable geomXformable;

        bool isRoot;
    };
}