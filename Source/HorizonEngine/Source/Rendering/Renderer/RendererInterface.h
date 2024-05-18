#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    static constexpr float NearClippingPlaneDepthValue = 1.0f;
    static constexpr float FarClippingPlaneDepthValue = 0.0f;

    enum class RendererType
    {
        RealTime,
        PathTracing,
    };

    /**
     * The renderer implements the process of generating visual images.
     */
    class SceneRenderer
    {
    public:
        virtual ~SceneRenderer() = default;
        virtual void Render(RenderGraph& renderGraph) = 0;
    };
}