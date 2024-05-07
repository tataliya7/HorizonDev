#pragma once

#include "PostProcessingCommon.h"

namespace Horizon
{
    struct PostProcessingSceneColorMipChain
    {
        static const uint32 MipCount = 6;

        // 1/2, 1/4, 1/8, 1/16, 1/32, 1/64
        RenderGraphTextureHandle textures[MipCount];
    };
}