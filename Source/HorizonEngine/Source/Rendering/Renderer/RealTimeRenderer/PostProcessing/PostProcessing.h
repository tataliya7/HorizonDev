#pragma once

#include "../RealTimeRendererCommon.h"

namespace Horizon
{
    enum
    {
        PostProcessingThreadGroupSizeX = 8,
        PostProcessingThreadGroupSizeY = 8,
    };

    struct PostProcessingColorPyramid
    {
        static constexpr uint32 MaxMipLevelCount = 5;

        /** Number of generated mip level count. */
        uint32 mipLevelCount = 0;

        /** 1/2, 1/4, 1/8, 1/16, 1/32 */
        RenderGraphTextureDesc textureDescs[MaxMipLevelCount] = {};
        RenderGraphTextureHandle textures[MaxMipLevelCount] = {};
    };

    struct PostProcessingColorTransformLUTSettings
    {
        bool initialized = false;
        float whiteBalance;

        bool Update(const SceneView& view, const PostProcessingSettings& postProcessingSettings);
    };
}