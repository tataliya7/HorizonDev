#pragma once

#include "USD.h"
#include "USDImportContext.h"
#include "USDPrimImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usdGeom/mesh.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    class USDGeomMeshImporter : public USDPrimImporter
    {
    public:
        USDGeomMeshImporter(const USDImportContext& context, const pxr::UsdPrim& prim);

        void CreateEntity(Scene* scene) override;
        void AddComponents(Scene* scene) override;
    private:
        pxr::UsdGeomMesh geomMesh;
        bool hasUVs;
    };
}