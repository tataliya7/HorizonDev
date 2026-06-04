#pragma once

#include "PathTracingRendererCommon.h"
#include "PathTracingRendererUniformVariables.h"
#include "Rendering/Renderer/PostProcessing/PostProcessingPipeline.h"

namespace Horizon
{
    struct PathTracingRendererIntermediateResources
    {
        RenderGraphTextureHandle colorTexture;
        RenderGraphTextureHandle depthTexture;
        RenderGraphTextureHandle environmentMapTexture;
    };

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
        PostProcessingPipeline postProcessingPipeline;

        SceneView* sceneView;

        Vector2f cameraJitterOffset;

        Matrix4x4f reprojectionMatrix;
        Matrix4x4f inverseReprojectionMatrix;

        RasterizationRendererSettings rendererSettings;
        PostProcessingSettings finalPostProcessingSettings;

        PathTracingRendererUniformVariables uniformVariables = {};
        static constexpr uint32 MaxNumFramesInFlight = 3;
        int32 currentPerFrameDataBufferIndex = 0;
        RenderBackendBufferHandle currentPerFrameConstantBuffer;
        RenderBackendBufferHandle perFrameConstantUploadBuffers[MaxNumFramesInFlight];
        RenderBackendBufferHandle perFrameConstantBuffers[MaxNumFramesInFlight];

        float renderResolutionPercentage = 1.0f;

        Extent2D renderResolution;
        Extent2D targetResolution;
        Extent2D displayResolution;

        void UpdateUniformVariables();

        void DispatchPathTracing(
            RenderGraph& renderGraph,
            const SceneView& view);

        float materialTextureMipLodBias = 0.0f;
        float preExposure = 1.0f;

        struct HistoryFrame
        {
            Vector3f cameraPosition;
            Vector2f cameraJitterOffset;
            CameraTransformations transformations;
            float preExposure;
        };

        HistoryFrame historicalData;
    };
}