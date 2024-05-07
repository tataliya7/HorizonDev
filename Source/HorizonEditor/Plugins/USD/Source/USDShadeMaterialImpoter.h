#pragma once

#include "USD.h"
#include "USDImportContext.h"

#include "USDIncludeBegin.h"
#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/material.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    class USDShadeMaterialImpoter
    {
    public:
        USDShadeMaterialImpoter(USDImportContext& context);
        Material ImportMaterial(const pxr::UsdShadeMaterial& usdShadeMaterial);
    private:
        // TODO: const
        USDImportContext* context;
    };
}