#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::DispatchPathTracing(RenderGraph& renderGraph, const SceneView& view)
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

            pathTracingPipelineStateDesc.shaders.push_back(shaderLibrary->GetShader(ShaderID::PathTracingRayGen));
            pathTracingPipelineStateDesc.shaders.push_back(shaderLibrary->GetShader(ShaderID::PathTracingDefaultMiss));
            pathTracingPipelineStateDesc.shaders.push_back(shaderLibrary->GetShader(ShaderID::PathTracingShadowRayMiss));
            pathTracingPipelineStateDesc.shaders.push_back(shaderLibrary->GetShader(ShaderID::PathTracingDefaultOpaqueClosestHit));

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

        uint32 spp = 1024;

        //RenderGraphTextureHandle colorTexture = renderGraph.ImportExternalTexture(colorTexture, "PathTracingColorTexture");
        //RenderGraphTextureHandle depthTexture = renderGraph.ImportExternalTexture(depthTexture, "PathTracingDepthTexture");
        //RenderGraphTextureHandle normalTexture = renderGraph.ImportExternalTexture(normalTexture, "PathTracingNormalTexture");

        RenderGraphTextureDesc depthTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle depthTexture = renderGraph.CreateTexture(depthTextureDesc, "PathTracingDepthTexture");

        renderGraph.AddPass(
            std::format("PathTracing (RayTracing, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::RayTracing,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                RenderGraphTextureHandle environmentMapTexture = builder.ReadTexture(sceneTextures.environmentMapTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle colorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::UnorderedAccess);
                depthTexture = builder.WriteTexture(depthTexture, RenderBackendResourceState::UnorderedAccess);
                //RenderGraphTextureHandle normalTexture = builder.WriteTexture(normalTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchWidth = renderResolution.width;
                    uint32 dispatchHeight = renderResolution.height;
                    uint32 dispatchDepth = 1;

                    RayTracingScene* rayTracingScene = view.scene->GetRayTracingScene();

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
                    shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                    shaderConstants.BindAccelerationStructure(3, renderBackend->GetAccelerationStructureSRVBindlessResourceDescriptorIndex(rayTracingScene->GetTLAS()));
                    shaderConstants.BindBufferSRV(4, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(view.scene->distantLightDataBuffer));
                    shaderConstants.BindTextureUAV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(environmentMapTexture));
                    shaderConstants.BindTextureUAV(6, registry.GetTextureUAVBindlessResourceDescriptorIndex(colorTexture, 0));
                    shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndex(depthTexture, 0));
                    //shaderConstants.BindTextureUAV(8, registry.GetTextureUAVBindlessResourceDescriptorIndex(normalTexture, 0));

                    commandList.DispatchRays(
                        pathTracingPipelineState,
                        pathTracingSBT,
                        shaderConstants,
                        dispatchWidth,
                        dispatchHeight,
                        dispatchDepth);
                };
            });
    }
}