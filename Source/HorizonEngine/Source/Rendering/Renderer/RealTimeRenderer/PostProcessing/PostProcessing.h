#pragma once

#include "PostProcessingCommon.h"

namespace Horizon
{
    struct PostProcessingSceneColorMipChain
    {
        static const uint32 MaxMipCount = 4;

        /** Number of generated mip level count. */
        uint32 mipCount = 0;

        /** 1/1, 1/2, 1/4, 1/8 */
        RenderGraphTextureHandle textures[MaxMipCount] = {};
    };
}