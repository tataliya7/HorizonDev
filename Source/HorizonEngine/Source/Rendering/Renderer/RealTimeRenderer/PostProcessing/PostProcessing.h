#pragma once

#include "../RealTimeRendererCommon.h"

namespace Horizon
{
    enum
    {
        PostProcessingThreadGroupSizeX = 8,
        PostProcessingThreadGroupSizeY = 8,
    };

    struct PostProcessingSceneColorMipChain
    {
        static constexpr uint32 MaxMipCount = 4;

        /** Number of generated mip level count. */
        uint32 mipCount = 0;

        /** 1/2, 1/4, 1/8, 1/16 */
        RenderGraphTextureHandle textures[MaxMipCount] = {};
    };
}