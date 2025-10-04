#include "Rendering/Renderer/RasterizationRenderer/RasterizationRenderer.h"

namespace Horizon
{
    void RasterizationRenderer::DispatchPathTracing(RenderGraph& renderGraph, const SceneView& view)
    {
        static RenderBackendRayTracingPipelineStateHandle pathTracingPipelineState;
        static RenderBackendBufferHandle pathTracingSBT;

        static bool firstTime = 1;
        if (firstTime)
        {
            firstTime = 0;

            RenderBackendRayTracingPipelineStateDesc pathTracingPipelineStateDesc =
            {
                .maxRayRecursionDepth = 8,
            };

            pathTracingPipelineStateDesc.shaders.push_back(shaderCollection->GetShader(ShaderID::PathTracingRayGen));
            pathTracingPipelineStateDesc.shaders.push_back(shaderCollection->GetShader(ShaderID::PathTracingDefaultMiss));
            pathTracingPipelineStateDesc.shaders.push_back(shaderCollection->GetShader(ShaderID::PathTracingShadowRayMiss));
            pathTracingPipelineStateDesc.shaders.push_back(shaderCollection->GetShader(ShaderID::PathTracingDefaultOpaqueClosestHit));

            pathTracingPipelineStateDesc.shaderGroupDescs.resize(4);
            pathTracingPipelineStateDesc.shaderGroupDescs[0] = RenderBackendRayTracingShaderGroupDesc::CreateRayGen(0);
            pathTracingPipelineStateDesc.shaderGroupDescs[1] = RenderBackendRayTracingShaderGroupDesc::CreateMiss(1);
            pathTracingPipelineStateDesc.shaderGroupDescs[2] = RenderBackendRayTracingShaderGroupDesc::CreateMiss(2);
            pathTracingPipelineStateDesc.shaderGroupDescs[3] = RenderBackendRayTracingShaderGroupDesc::CreateTrianglesHitGroup(3, RenderBackendRayTracingShaderGroupDesc::ShaderUnused, RenderBackendRayTracingShaderGroupDesc::ShaderUnused);

            pathTracingPipelineState = renderBackend->CreateRayTracingPipelineState(&pathTracingPipelineStateDesc, "PathTracingPipelineStateObject");

            RenderBackendRayTracingShaderBindingTableDesc pathTracingSBTDesc =
            {
                .rayTracingPipelineState = pathTracingPipelineState,
                .shaderRecordCount = 0,
            };
            pathTracingSBT = renderBackend->CreateRayTracingShaderBindingTable(&pathTracingSBTDesc, "PathTracingSBT");
        }

        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();

        uint32 spp = 4096;
        uint32 maxIteration = spp;

        static int32 iteration = 0;
        float accumulationFactor = 1.0f / float(maxIteration);

        //RenderGraphTextureHandle colorTexture = renderGraph.ImportExternalTexture(colorTexture, "PathTracingColorTexture");
        //RenderGraphTextureHandle depthTexture = renderGraph.ImportExternalTexture(depthTexture, "PathTracingDepthTexture");
        //RenderGraphTextureHandle normalTexture = renderGraph.ImportExternalTexture(normalTexture, "PathTracingNormalTexture");

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        RenderGraphTextureDescription colorTextureDesc = renderGraph.GetTextureDesc(intermediateResources.colorTexture);
        RenderGraphTextureHandle colorTexture = renderGraph.CreateTexture(colorTextureDesc, "PathTracingColorTexture");

        RenderGraphTextureDescription depthTextureDesc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle depthTexture = renderGraph.CreateTexture(depthTextureDesc, "PathTracingDepthTexture");

        if (view.transformations.worldToClipMatrix != historyFrame.transformations.worldToClipMatrix)
        {
            iteration = 0;

            renderGraph.AddPass(
                std::format("ClearSceneTextures"),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    colorTexture = builder.WriteTexture(colorTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::Black;

                        RenderBackendTextureUAVDesc sceneColorTextureUAV = RenderBackendTextureUAVDesc::Create(resourceRegistry.GetRenderBackendTextureHandle(colorTexture), 0);
                        commandList.ClearTextureUAV(sceneColorTextureUAV, clearValue);
                    };
                });
        }

        renderGraph.AddPass(
            std::format("PathTracing (RayTracing, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::RayTracing,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle environmentMapTexture = builder.ReadTexture(intermediateResources.environmentMapTexture, RenderBackendResourceState::ShaderResource);
                colorTexture = builder.WriteTexture(colorTexture, RenderBackendResourceState::UnorderedAccess);
                depthTexture = builder.WriteTexture(depthTexture, RenderBackendResourceState::UnorderedAccess);
                //RenderGraphTextureHandle normalTexture = builder.WriteTexture(normalTexture, RenderBackendResourceState::UnorderedAccess);
                intermediateResources.colorTexture = colorTexture;

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 dispatchWidth = renderResolution.width;
                    uint32 dispatchHeight = renderResolution.height;
                    uint32 dispatchDepth = 1;

                    RayTracingScene* rayTracingScene = view.scene->GetRayTracingScene();

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
                    pushConstantValues.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                    pushConstantValues.BindAccelerationStructure(3, renderBackend->GetAccelerationStructureSRVBindlessResourceDescriptorIndex(rayTracingScene->GetRayTracingTLAS()));
                    pushConstantValues.BindBufferSRV(4, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(view.scene->distantLightDataBuffer));
                    pushConstantValues.BindTextureUAV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(environmentMapTexture));
                    pushConstantValues.BindTextureUAV(6, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(colorTexture, 0));
                    pushConstantValues.BindTextureUAV(7, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(depthTexture, 0));
                    //pushConstantValues.BindTextureUAV(8, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(normalTexture, 0));
                    pushConstantValues.OverrideShaderConstantValue(8, iteration);
                    pushConstantValues.OverrideShaderConstantValue(9, accumulationFactor);

                    commandList.DispatchRays(
                        pathTracingPipelineState,
                        pathTracingSBT,
                        pushConstantValues,
                        dispatchWidth,
                        dispatchHeight,
                        dispatchDepth);
                };
            });


        iteration++;
    }
}