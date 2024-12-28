#pragma once

#include "USD.h"
#include "USDImportContext.h"
#include "USDGemoXformableImporter.h"

namespace Horizon::USDImporter
{
    class USDGeomCameraImporter : public USDGemoXformableImporter
    {
    public:
        USDGeomCameraImporter(const USDImportContext& context, const pxr::UsdPrim& prim);

        void CreateEntity(Scene* scene) override;
        void AddComponents(Scene* scene) override;
    };
}