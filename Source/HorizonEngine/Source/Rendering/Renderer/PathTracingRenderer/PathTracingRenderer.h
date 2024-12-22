#pragma once

#include "PathTracingRendererCommon.h"

namespace Horizon
{
    class PathTracingRenderer// : public SceneRenderer
    {
    public:

        RenderGraphPersistentTexture* colorTexture;
        RenderGraphPersistentTexture* depthTexture;
        RenderGraphPersistentTexture* normalTexture;

        void DispatchPathTracing(
            RenderGraph& renderGraph,
            const SceneView& view);
    };
}