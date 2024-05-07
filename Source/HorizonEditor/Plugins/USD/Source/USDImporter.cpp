#include "USD.h"
#include "USDPrimImporter.h"
#include "USDGeomMeshImporter.h"
#include "USDStageImporter.h"
#include "USDUtils.h"

#include "USDIncludeBegin.h"
#include <pxr/pxr.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/diagnosticMgr.h>
#include <pxr/base/tf/errorMark.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/tf/setenv.h>
#include <pxr/usd/ar/asset.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/defineResolver.h>
#include <pxr/usd/ar/packageUtils.h>
#include <pxr/usd/ar/writableAsset.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/debugCodes.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/schemaBase.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stageCacheContext.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>
#include <pxr/usd/usd/usdzFileFormat.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/subset.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/modelAPI.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdUtils/dependencies.h>
#include <pxr/usd/usdUtils/stageCache.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    enum class USDImportResult
    {
        Succeed,
        Failed,
    };

    struct USDImportTaskData
    {
        Scene* scene;
        USDImportSettings settings;
        USDStageImporter* stageImporter;
        std::string filename;
        bool canceled;
        USDImportResult result;
    };

    static void ConvertToZUp(pxr::UsdStageRefPtr stage, const USDImportSettings& settings)
    {
        if (!stage || (pxr::UsdGeomGetStageUpAxis(stage) == pxr::UsdGeomTokens->z))
        {
            return;
        }

        //r_settings->do_convert_mat = true;

        ///* Rotate 90 degrees about the X-axis. */
        //float rmat[3][3];
        //float axis[3] = { 1.0f, 0.0f, 0.0f };
        //axis_angle_normalized_to_mat3(rmat, axis, M_PI_2);

        //unit_m4(r_settings->conversion_mat);
        //copy_m4_m3(r_settings->conversion_mat, rmat);
    }

    void USDImportStart(USDImportTaskData* data)
    {
        pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(data->filename);
        if (!stage)
        {
            // LogError(GLogger, "USD Import: unable to open stage to read %s", filename);
            data->result = USDImportResult::Failed;
            return;
        }

        //
        //#if 0
        //    ConvertToZUp(stage, data->settings);
        //#else
        //    pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->z);
        //#endif
        //
        //
        //    data->settings.stage_meters_per_unit = UsdGeomGetStageMetersPerUnit(stage);

        USDStageImporter* stageImporter = new USDStageImporter(stage, data->settings);

        data->stageImporter = stageImporter;

        stageImporter->CollectPrimImporters();

        if (data->settings.importMaterials)
        {
            stageImporter->ImportAllMaterials();
        }

        // Create entities
        for (USDPrimImporter* impoter : stageImporter->GetPrimImporters())
        {
            if (!impoter)
            {
                continue;
            }

            impoter->CreateEntity(data->scene);
        }

        //
        for (USDPrimImporter* impoter : stageImporter->GetPrimImporters())
        {
            if (!impoter)
            {
                continue;
            }

            impoter->AddComponents(data->scene);

            USDPrimImporter* parent = impoter->GetParent();
            if (!parent)
            {

            }
            else
            {

            }
        }
    }

    void USDImportEnd(USDImportTaskData* data)
    {

    }
}

bool USDImport(const char* filename, const USDImportSettings* settings, bool asyncTask)
{
    using namespace Horizon;
    using namespace Horizon::USDImporter;

    USDImportTaskData* data = new USDImportTaskData();
    data->settings = *settings;
    data->filename = filename;
    data->canceled = false;
    data->result = USDImportResult::Succeed;
    data->scene = SceneManager::GetActiveScene();

    bool succeed = true;
    if (asyncTask)
    {

    }
    else
    {
        USDImportStart(data);
        USDImportEnd(data);

        succeed = (data->result == USDImportResult::Succeed);
        // USDImportFreeTaskData(data);

        delete data;
    }

    return succeed;
}

int USDGetVersion()
{
    return PXR_VERSION;
}

void USDInit_DEPRECATED(const std::string& path)
{
    std::vector<std::string> pluginPaths = {};
    pluginPaths.push_back(path);
    pxr::PlugRegistry::GetInstance().RegisterPlugins(pluginPaths);
}