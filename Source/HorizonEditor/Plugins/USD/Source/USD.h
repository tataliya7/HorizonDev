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
    };

    struct USDExportSettings
    {
        bool exportAnimation;
        bool exportMaterials;
    };

    int USDGetVersion();

    [[deprecated]]
    void USDInit(const std::string& path);

    bool USDImport(const char* filename, const struct USDImportSettings* settings, bool asyncTask);
    bool USDExport(const char* filename, const struct USDImportSettings* settings, bool asyncTask);
}