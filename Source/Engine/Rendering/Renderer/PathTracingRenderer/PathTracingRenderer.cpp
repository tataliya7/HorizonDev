#include "PathTracingRenderer.h"

namespace HE
{
    bool PathTracingRenderer::LoadShaders()
    {
        uint32 deviceMask = ~0u;
        ShaderIntermediateLanguage il = (GRenderBackend->GetType() == RenderBackendType::Vulkan) ? ShaderIntermediateLanguage::SPIRV : ShaderIntermediateLanguage::DXIL;

        std::vector<uint8> source;
        std::vector<std::wstring> includeDirs;
        std::vector<std::wstring> defines;
        includeDirs.push_back(HE_TEXT("../../../Shaders"));
        includeDirs.push_back(HE_TEXT("../../../Shaders/PathTracingRenderer"));
        defines.push_back(HE_TEXT("RAY_TRACING_ENABLED=1"));

        LoadShaderSourceFromFile("../../../Shaders/PathTracingRenderer/PathTracing.hsf", source);

        RenderBackendRayTracingPipelineStateDesc pathTracingPipelineStateDesc = {
            .maxRayRecursionDepth = 1,
        };
        pathTracingPipelineStateDesc.shaders.push_back(RenderBackendRayTracingShaderDesc{
            .stage = RenderBackendShaderStage::RayGen,
            .entry = "PathTracingRayGen",
            });
        shaderLibrary->shaderCompiler->CompileShader(
            source,
            HE_TEXT("PathTracingRayGen"),
            RenderBackendShaderStage::RayGen,
            il,
            includeDirs,
            defines,
            &pathTracingPipelineStateDesc.shaders[0].code);

        pathTracingPipelineStateDesc.shaders.push_back(RenderBackendRayTracingShaderDesc{
            .stage = RenderBackendShaderStage::Miss,
            .entry = "PathTracingDefaultMiss",
            });
        shaderLibrary->shaderCompiler->CompileShader(
            source,
            HE_TEXT("PathTracingDefaultMiss"),
            RenderBackendShaderStage::Miss,
            il,
            includeDirs,
            defines,
            &pathTracingPipelineStateDesc.shaders[1].code);

        pathTracingPipelineStateDesc.shaders.push_back(RenderBackendRayTracingShaderDesc{
          .stage = RenderBackendShaderStage::Miss,
          .entry = "PathTracingDefaultMiss",
            });
        shaderLibrary->shaderCompiler->CompileShader(
            source,
            HE_TEXT("PathTracingDefaultMiss"),
            RenderBackendShaderStage::Miss,
            il,
            includeDirs,
            defines,
            &pathTracingPipelineStateDesc.shaders[1].code);

        pathTracingPipelineStateDesc.shaders.push_back(RenderBackendRayTracingShaderDesc{
          .stage = RenderBackendShaderStage::Miss,
          .entry = "PathTracingDefaultMiss",
            });
        shaderLibrary->shaderCompiler->CompileShader(
            source,
            HE_TEXT("PathTracingDefaultMiss"),
            RenderBackendShaderStage::Miss,
            il,
            includeDirs,
            defines,
            &pathTracingPipelineStateDesc.shaders[1].code);

        pathTracingPipelineStateDesc.shaderGroupDescs.resize(3);
        pathTracingPipelineStateDesc.shaderGroupDescs[0] = RenderBackendRayTracingShaderGroupDesc::CreateRayGen(0);
        pathTracingPipelineStateDesc.shaderGroupDescs[1] = RenderBackendRayTracingShaderGroupDesc::CreateMiss(1);
        //pathTracingPipelineStateDesc.shaderGroupDescs[2] = RenderBackendRayTracingShaderGroupDesc::CreateTrianglesHitGroup(1);

        pathTracingPipelineState = renderBackend->CreateRayTracingPipelineState(deviceMask, &pathTracingPipelineStateDesc, "PathTracingPipelineState");

        RenderBackendRayTracingShaderBindingTableDesc pathTracingSBTDesc = {
            .rayTracingPipelineState = pathTracingPipelineState,
            .numShaderRecords = 0,
        };
        pathTracingSBT = renderBackend->CreateRayTracingShaderBindingTable(deviceMask, &pathTracingSBTDesc, "PathTracingSBT");

        return true;
    }

    void PathTracingRenderer::SetupRenderGraph(RenderGraph& renderGraph, const SceneView& view)
    {
        /*renderGraph.AddPass("PathTracing", RenderGraphPassFlags::RayTracing,
            [&](RenderGraphBuilder& builder)
            {
                shadowMask = builder.WriteTexture(shadowMask, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 width = sceneViewShaderParameters.renderResolutionX;
                    uint32 height = sceneViewShaderParameters.renderResolutionY;

                    RenderBackendRayTracingAccelerationStructureHandle rayTracingScene = scene->GetRayTracingScene();

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindAS(1, rayTracingScene);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepth)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(shadowMask), 0));
                    shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(rayDistance), 0));
                    shaderArguments.PushConstants(0, light.GetDirection().x);
                    shaderArguments.PushConstants(1, light.GetDirection().y);
                    shaderArguments.PushConstants(2, light.GetDirection().z);

                    commandList.TraceRays(
                        pathTracingPipelineState,
                        pathTracingSBT,
                        shaderArguments,
                        width,
                        height,
                        1);
                };
            });*/
    }
}