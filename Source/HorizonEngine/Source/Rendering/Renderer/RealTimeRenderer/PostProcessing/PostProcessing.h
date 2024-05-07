#pragma once

#include "Rendering/Renderer/RealTimeRenderer/PostProcessing/PostProcessingCommon.h"

namespace Horizon
{
    struct PostProcessingSceneColorMipChain
    {
        // 1/2, 1/4, 1/8, 1/16, 1/32, 1/64
        RenderGraphTextureHandle textures[6];
    };
}