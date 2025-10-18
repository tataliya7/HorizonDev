#include "RasterizationRenderer.h"

namespace Horizon
{
    bool ShouldRenderRayTracingShadowsForLight(const LightRenderObject& light)
    {
        //if (light.CastDynamicShadows())
        //{
        //    return true;
        //}
        return false;
    }

    void RasterizationRenderer::DispatchRayTracingShadows(
        RenderGraph& renderGraph,
        const SceneView& view,
        const LightRenderObject& light,
        RenderGraphTextureHandle& screenSpaceShadowMaskTexture,
        RenderGraphTextureHandle& rayDistanceTexture)
    {
        static RenderBackendRayTracingPipelineStateHandle rayTracingShadowsPipelineState;
        static RenderBackendBufferHandle rayTracingShadowsSBT;

        static bool firstTime = 1;
        if (firstTime)
        {
            firstTime = 0;

            RenderBackendRayTracingPipelineStateDesc rayTracingShadowsPipelineStateDesc =
            {
                .maxRayRecursionDepth = 1,
            };

            rayTracingShadowsPipelineStateDesc.shaders.push_back(shaderRepository->GetShader(ShaderID::RayTracingShadowsRayGen));
            rayTracingShadowsPipelineStateDesc.shaders.push_back(shaderRepository->GetShader(ShaderID::RayTracingShadowsMiss));

            rayTracingShadowsPipelineStateDesc.shaderGroupDescs.resize(2);
            rayTracingShadowsPipelineStateDesc.shaderGroupDescs[0] = RenderBackendRayTracingShaderGroupDesc::CreateRayGen(0);
            rayTracingShadowsPipelineStateDesc.shaderGroupDescs[1] = RenderBackendRayTracingShaderGroupDesc::CreateMiss(1);

            rayTracingShadowsPipelineState = renderBackend->CreateRayTracingPipelineState(&rayTracingShadowsPipelineStateDesc, "RayTracingShadowsPipelineState");

            RenderBackendRayTracingShaderBindingTableDesc rayTracingShadowsSBTDesc =
            {
                .rayTracingPipelineState = rayTracingShadowsPipelineState,
                .shaderRecordCount = 0,
            };
            rayTracingShadowsSBT = renderBackend->CreateRayTracingShaderBindingTable(&rayTracingShadowsSBTDesc, "RayTracingShadowsSBT");
        }

        const bool inlineRayTracing = true;
        if (inlineRayTracing)
        {
            renderGraph.AddPass(
                std::format("RayTracingShadows (Compute-InlineRayTracing, {}x{})", renderResolution.width, renderResolution.height),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();
                    RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                    screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 4);
                        uint32 threadGroupCountZ = 1;

                        RayTracingScene* rayTracingScene = view.GetRenderScene()->GetRayTracingScene();

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindAccelerationStructure(1, renderBackend->GetAccelerationStructureSRVBindlessResourceDescriptorIndex(rayTracingScene->GetRayTracingTLAS()));
                        pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        pushConstantValues.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture, 0));
                        pushConstantValues.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(rayDistanceTexture, 0));
                        pushConstantValues.OverrideShaderConstantValue(5, light.GetDirection().x);
                        pushConstantValues.OverrideShaderConstantValue(6, light.GetDirection().y);
                        pushConstantValues.OverrideShaderConstantValue(7, light.GetDirection().z);

                        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::RayTracingShadowsInlineRayTracing);

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
                });
        }
        else
        {
            renderGraph.AddPass(
                std::format("RayTracingShadows (RayTracing, {}x{})", renderResolution.width, renderResolution.height),
                RenderGraphPassFlags::RayTracing,
                [&](RenderGraphBuilder& builder)
                {
                    RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();
                    RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                    screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        uint32 dispatchWidth = renderResolution.width;
                        uint32 dispatchHeight = renderResolution.height;
                        uint32 dispatchDepth = 1;

                        RayTracingScene* rayTracingScene = view.GetRenderScene()->GetRayTracingScene();

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindAccelerationStructure(1, renderBackend->GetAccelerationStructureSRVBindlessResourceDescriptorIndex(rayTracingScene->GetRayTracingTLAS()));
                        pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        pushConstantValues.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture, 0));
                        pushConstantValues.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(rayDistanceTexture, 0));
                        pushConstantValues.OverrideShaderConstantValue(5, light.GetDirection().x);
                        pushConstantValues.OverrideShaderConstantValue(6, light.GetDirection().y);
                        pushConstantValues.OverrideShaderConstantValue(7, light.GetDirection().z);

                        commandList.DispatchRays(
                            rayTracingShadowsPipelineState,
                            rayTracingShadowsSBT,
                            pushConstantValues,
                            dispatchWidth,
                            dispatchHeight,
                            dispatchDepth);
                    };
                });
        }
    }
}