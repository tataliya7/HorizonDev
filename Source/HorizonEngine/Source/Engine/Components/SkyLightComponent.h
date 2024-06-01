#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    class SkyLightComponent
    {

    };
}

//#pragma once
//
//namespace Horizon
//{
//    struct EnvironmentLightComponent
//    {
//        EnvironmentLightComponent()
//        {
//
//        }
//
//        std::string cubemap;
//        uint32 cubemapResolution;
//
//        const std::string& GetCubemap() const
//        {
//            return cubemap;
//        }
//
//        uint32 GetCubemapResolution() const
//        {
//            return cubemapResolution;
//        }
//
//        void SetCubemap(std::string newCubemap)
//        {
//            if (cubemap != newCubemap)
//            {
//                cubemap = newCubemap;
//                SetDirty(true);
//            }
//        }
//
//        void SetDirty(bool value)
//        {
//            dirty = value;
//        }
//
//        bool IsDirty()
//        {
//            return dirty;
//        }
//
//        RenderBackendTextureHandle GetEnvironmentMap() const
//        {
//            return environmentMap;
//        }
//
//        RenderBackendTextureHandle GetIrradianceEnvironmentMap() const
//        {
//            return irradianceEnvironmentMap;
//        }
//
//        RenderBackendBufferHandle GetIrradianceEnvironmentMapSH() const
//        {
//            return irradianceEnvironmentMapSH;
//        }
//
//        RenderBackendTextureHandle GetFilteredEnvironmentMap() const
//        {
//            return filteredEnvironmentMap;
//        }
//
//        bool dirty;
//
//        RenderBackendTextureHandle environmentMap;
//        RenderBackendTextureHandle irradianceEnvironmentMap;
//        RenderBackendTextureHandle filteredEnvironmentMap;
//        RenderBackendBufferHandle irradianceEnvironmentMapSH;
//    };
//}