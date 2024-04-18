#include "USDStageImporter.h"
#include "USDGemoXformableImporter.h"
#include "USDGeomMeshImporter.h"
#include "USDGeomCameraImporter.h"
#include "USDSkelSkeletonImporter.h"
#include "USDShadeMaterialImpoter.h"

#include "USDIncludeBegin.h"
#include <pxr/pxr.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/capsule.h>
#include <pxr/usd/usdGeom/cone.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/curves.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/cylinder.h>
#include <pxr/usd/usdGeom/nurbsCurves.h>
#include <pxr/usd/usdShade/material.h>
#include "USDIncludeEnd.h"

namespace HE::USDImporter
{
    USDStageImporter::USDStageImporter(pxr::UsdStageRefPtr stage, const USDImportSettings& settings)
        : stage(stage)
        , importSettings(settings)
    {
        importContext.importSettings = &importSettings;
    }

    USDStageImporter::~USDStageImporter()
    {
        
    }

    void USDStageImporter::CollectPrimImporters()
    {
        if (!IsValid())
        {
            return;
        }

        DestroyAllPrimImporters();

        pxr::UsdPrim root = stage->GetPseudoRoot();

        stage->SetInterpolationType(pxr::UsdInterpolationType::UsdInterpolationTypeHeld);

        CreatePrimImporters(root);
    }

    void USDStageImporter::ImportAllMaterials()
    {
        if (!IsValid())
        {
            return;
        }

        USDShadeMaterialImpoter materialImporter(importContext);
        for (const std::string& materialPath : importContext.materialPaths)
        {
            pxr::UsdPrim prim = stage->GetPrimAtPath(pxr::SdfPath(materialPath));
            pxr::UsdShadeMaterial usdShadeMaterial(prim);
            if (!usdShadeMaterial)
            {
                continue;
            }

            printf("Material Path: %s\n", materialPath.c_str());

            if (importContext.materialMap.find(materialPath) != importContext.materialMap.end())
            {
                continue;
            }

            Material material = materialImporter.ImportMaterial(usdShadeMaterial);
            importContext.materialMap[materialPath] = material;
        }
    }

    void USDStageImporter::DestroyAllPrimImporters()
    {

    }

    USDPrimImporter* USDStageImporter::CreatePrimImporterImpl(const pxr::UsdPrim& prim)
    {
        /* if (settings.importCameras && prim.IsA<pxr::UsdGeomCamera>())
        {
            return new USDCameraImporter(prim, settings);
        }*/

        if (importSettings.importMeshes && prim.IsA<pxr::UsdGeomMesh>())
        {
            return new USDGeomMeshImporter(importContext, prim);
        }

        /* if (settings.importLights && (prim.IsA<pxr::UsdLuxBoundableLightBase>() || prim.IsA<pxr::UsdLuxNonboundableLightBase>()))
        {
            return new USDLightReader(prim, settings);
        }*/

        if (prim.IsA<pxr::UsdGeomImageable>())
        {
            //return new USDXformReader(prim, settings);
        }

        if (importSettings.importSkeletons && prim.IsA<pxr::UsdSkelSkeleton>())
        {
            return new USDSkelSkeletonImporter(importContext, prim); 
        }

        if (prim.IsA<pxr::UsdGeomXformable>())
        {
            return new USDGemoXformableImporter(importContext, prim);
        }

        if (prim.IsA<pxr::UsdShadeMaterial>())
        {
            importContext.materialPaths.push_back(prim.GetPath().GetAsString());
            return nullptr;
        }

        return nullptr;
    }

    USDPrimImporter* USDStageImporter::CreatePrimImporters(const pxr::UsdPrim& prim)
    {
        pxr::Usd_PrimFlagsPredicate predicate = pxr::UsdPrimDefaultPredicate;

        std::vector<USDPrimImporter*> childPrimImporters;
        pxr::UsdPrimSiblingRange children = prim.GetFilteredChildren(predicate);
        for (const pxr::UsdPrim& childPrim : children)
        {
            USDPrimImporter* childPrimImporter = CreatePrimImporters(childPrim);
            if (childPrimImporter)
            {
                childPrimImporters.push_back(childPrimImporter);
            }
        }

        // printf("Prim Path: %s\n", prim.GetPath().GetAsString().c_str());

        if (prim.IsPseudoRoot())
        {
            return nullptr;
        }

        USDPrimImporter* primImporter = CreatePrimImporterImpl(prim);
        if (!primImporter)
        {
            return nullptr;
        }

        this->primImporters.push_back(primImporter);

        for (USDPrimImporter* childPrimImporter : childPrimImporters)
        {
            childPrimImporter->SetParent(primImporter);
        }

        return primImporter;
    }
}
