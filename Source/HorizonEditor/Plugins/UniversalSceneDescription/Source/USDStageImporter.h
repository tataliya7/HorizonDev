#pragma once

#include "USD.h"
#include "USDImportContext.h"
#include "USDPrimImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/imageable.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    class USDStageImporter
    {
    public:
        USDStageImporter(pxr::UsdStageRefPtr stage, const USDImportSettings& settings);
        ~USDStageImporter();

        pxr::UsdStageRefPtr GetStage()
        {
            return stage;
        }

        bool IsValid() const
        {
            return stage != nullptr;
        }

        const USDImportSettings& GetImportSettings() const
        {
            return importSettings;
        }

        const USDImportContext& GetImportContext() const
        {
            return importContext;
        }

        const std::vector<USDPrimImporter*>& GetPrimImporters() const
        {
            return primImporters;
        };

        void CollectPrimImporters();

        void ImportAllMaterials();

        void ImportAllSkeletons();

        void DestroyAllPrimImporters();

    protected:
        pxr::UsdStageRefPtr stage;
        USDImportSettings importSettings;
        USDImportContext importContext;
        std::vector<USDPrimImporter*> primImporters;
    private:

        USDPrimImporter* CreatePrimImporters(const pxr::UsdPrim& prim);
        USDPrimImporter* CreatePrimImporterImpl(const pxr::UsdPrim& prim);
    };
}