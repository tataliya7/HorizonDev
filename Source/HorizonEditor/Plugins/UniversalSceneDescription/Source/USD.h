#pragma once

#include "Engine/HorizonEngineModule.h"

extern "C"
{
    struct USDImportSettings
    {
        bool importCameras;
        bool importLights;
        bool importMeshes;
        bool importMaterials;
        bool importSkeletons;
        bool importAnimations;
    };

    struct USDExportSettings
    {
        bool exportAnimation;
        bool exportMaterials;
    };

    extern int USDGetVersion();

    // [[deprecated]]
    extern void USDInit(const std::string& path);

    extern bool USDImport(Horizon::Scene* scene, const char* filename, const struct USDImportSettings* settings, bool asyncTask);
    extern bool USDExport(Horizon::Scene* scene, const char* filename, const struct USDImportSettings* settings, bool asyncTask);
}