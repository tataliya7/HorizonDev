#pragma once

#include "PathTracingRendererCommon.h"

namespace Horizon
{
    class PathTracingRenderer : public SceneRenderer
    {
    public:

        PathTracingRenderer(
            RenderBackend* renderBackend,
            RenderGraphResourcePool* resourcePool,
            ShaderRepository* shaderRepository,
            RendererDefaultResources* defaultResources);

        virtual ~PathTracingRenderer();

        void Tick(float deltaTimeInSeconds) override;

        void InitializeSceneView(SceneView* sceneView) override;

        void Render(RenderGraph& renderGraph) override;

    private:

        RenderBackend* renderBackend;
        RenderGraphResourcePool* resourcePool;
        ShaderRepository* shaderCollection;
        RendererDefaultResources* defaultResources;

        SceneView* sceneView;

        RenderGraphPersistentTexture* colorTexture;
        RenderGraphPersistentTexture* depthTexture;
        RenderGraphPersistentTexture* normalTexture;

        float renderResolutionPercentage = 1.0f;

        Extent2D renderResolution;
        Extent2D targetResolution;
        Extent2D displayResolution;

        void DispatchPathTracing(
            RenderGraph& renderGraph,
            const SceneView& view);
    };
}