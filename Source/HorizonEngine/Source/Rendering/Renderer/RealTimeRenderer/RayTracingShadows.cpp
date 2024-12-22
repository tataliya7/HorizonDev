#include "RealTimeRenderer.h"

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

    void RealTimeRenderer::DispatchRayTracingShadows(
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
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::RayGen, "Shaders/RealTimeRenderer/HardwareRayTracing/RayTracingShadows.hsm", "RayTracingShadowsRayGen");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                shaderLibrary->LoadShader(ShaderID::RayTracingShadowsRayGen, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Miss, "Shaders/RealTimeRenderer/HardwareRayTracing/RayTracingShadows.hsm", "RayTracingShadowsMiss");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                shaderLibrary->LoadShader(ShaderID::RayTracingShadowsMiss, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/HardwareRayTracing/RayTracingShadows.hsm", "RayTracingShadowsInlineRayTracingCS");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                shaderDesc.shaderCompilerOptions.inlineRayTracing = true;
                shaderLibrary->LoadShader(ShaderID::RayTracingShadowsInlineRayTracing, shaderDesc);
            }

            RenderBackendRayTracingPipelineStateDesc rayTracingShadowsPipelineStateDesc =
            {
                .maxRayRecursionDepth = 1,
            };

            rayTracingShadowsPipelineStateDesc.shaders.push_back(shaderLibrary->GetShader(ShaderID::RayTracingShadowsRayGen));
            rayTracingShadowsPipelineStateDesc.shaders.push_back(shaderLibrary->GetShader(ShaderID::RayTracingShadowsMiss));

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
                    RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                    RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                    screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 threadGroupCountX = CeilDiv(renderResolution.width, 8);
                        uint32 threadGroupCountY = CeilDiv(renderResolution.height, 4);
                        uint32 threadGroupCountZ = 1;

                        RayTracingScene* rayTracingScene = view.scene->GetRayTracingScene();

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        shaderConstants.BindAccelerationStructure(1, renderBackend->GetAccelerationStructureSRVBindlessResourceDescriptorIndex(rayTracingScene->GetTLAS()));
                        shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture, 0));
                        shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(rayDistanceTexture, 0));
                        shaderConstants.BindScalar(5, light.GetDirection().x);
                        shaderConstants.BindScalar(6, light.GetDirection().y);
                        shaderConstants.BindScalar(7, light.GetDirection().z);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::RayTracingShadowsInlineRayTracing);

                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
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
                    RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                    RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                    screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 dispatchWidth = renderResolution.width;
                        uint32 dispatchHeight = renderResolution.height;
                        uint32 dispatchDepth = 1;

                        RayTracingScene* rayTracingScene = view.scene->GetRayTracingScene();

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        shaderConstants.BindAccelerationStructure(1, renderBackend->GetAccelerationStructureSRVBindlessResourceDescriptorIndex(rayTracingScene->GetTLAS()));
                        shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture, 0));
                        shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(rayDistanceTexture, 0));
                        shaderConstants.BindScalar(5, light.GetDirection().x);
                        shaderConstants.BindScalar(6, light.GetDirection().y);
                        shaderConstants.BindScalar(7, light.GetDirection().z);

                        commandList.DispatchRays(
                            rayTracingShadowsPipelineState,
                            rayTracingShadowsSBT,
                            shaderConstants,
                            dispatchWidth,
                            dispatchHeight,
                            dispatchDepth);
                    };
                });
        }
    }
}