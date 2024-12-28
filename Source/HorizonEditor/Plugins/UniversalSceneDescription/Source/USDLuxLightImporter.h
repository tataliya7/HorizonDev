#pragma once

#include "USD.h"
#include "USDImportContext.h"
#include "USDGemoXformableImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usdLux/lightAPI.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/sphereLight.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/diskLight.h>
#include <pxr/usd/usdLux/domeLight.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    class USDLuxLightImporter : public USDGemoXformableImporter
    {
    public:
        USDLuxLightImporter(const USDImportContext& context, const pxr::UsdPrim& prim);

        void CreateEntity(Scene* scene) override;
        void AddComponents(Scene* scene) override;
    };
}