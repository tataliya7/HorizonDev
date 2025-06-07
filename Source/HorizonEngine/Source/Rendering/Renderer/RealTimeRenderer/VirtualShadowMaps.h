#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    struct VirtualShadowMapShaderParameters
    {
        uint32 maximumVirtualPageCount;
        uint32 physicalPageCount;
        Matrix4x4f worldToClipMatrix;
        //uint32 shadowMapSpaceRayTracingSampleCount;
    };

    struct VirtualShadowMapEntryShaderParameters
    {
        Matrix4x4f worldToClipMatrix;
        Vector4f worldSpaceOrigin;
        uint32 lightType;
        uint32 level; // Mipmap level or clipmap level
        uint32 virtualShadowMapEntryIndex;
        uint32 padding0;
    };
    static_assert(sizeof(VirtualShadowMapEntryShaderParameters) == 96);

    class VirtualShadowMap
    {
    public:

        //VirtualShadowMap(uint32 virtualShadowMapIndex);
        //virtual ~VirtualShadowMap();
        uint32 GetVirtualShadowMapIndex() const
        {
            return virtualShadowMapIndex;
        }

    private:

        friend class VirtualShadowMapManager;

        uint32 virtualShadowMapIndex;
    };

    class VirtualShadowMapMipmap
    {
    public:
    private:
        friend class VirtualShadowMapManager;
    };

    class VirtualShadowMapClipmap : public VirtualShadowMap
    {
    public:

        uint32 GetFirstLevel() const
        {
            return firstLevel;
        }

        uint32 GetLevelCount() const
        {
            return levelCount;
        }

    private:

        friend class VirtualShadowMapManager;

        Vector3f lightDirection;

        uint32 firstLevel;
        uint32 levelCount;

        Sphere boundingSphere;

        uint32 virtualShadowMapEntryIDBase;

        struct ClipmapLevelData
        {
            Vector3f origin;
            Matrix4x4f viewToClipMatrix;
        };
        std::vector<ClipmapLevelData> levels;
    };

    class VirtualShadowMapManager
    {
    public:

        VirtualShadowMapClipmap* CreateVirtualShadowMapClipmap(
            const SceneView& view,
            const LightRenderObject& light);

        void Clear();

        uint32 GetVirtualShadowMapCount() const
        {
            return virtualShadowMapCount;
        }

        uint32 GetVirtualShadowMapEntryCount() const
        {
            return virtualShadowMapEntryCount;
        }

    //private:

        uint32 virtualShadowMapCount = 0;
        uint32 virtualShadowMapEntryCount = 0;

        std::vector<VirtualShadowMap*> virtualShadowMaps;

        std::vector<VirtualShadowMapEntryShaderParameters> virtualShadowMapEntries;
    };
}