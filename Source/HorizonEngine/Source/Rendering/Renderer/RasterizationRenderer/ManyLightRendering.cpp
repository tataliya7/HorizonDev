#include "ManyLightRendering.h"
#include "RasterizationRenderer.h"

namespace Horizon
{
    static const uint32 GLightGridPixelCount = 64;
    static const uint32 GLightGridSizeZ = 32;
    static const uint32 GLightGridMaxLightCountPerCell = 64;
    static const uint32 GLocalLightCullingThreadGroupSize = 4; // TODO

    void RasterizationRenderer::DispatchLocalLightCulling(
       RenderGraph& renderGraph,
       const SceneView& view)
    {
        std::vector<GPUSceneLocalLightData> localLightData;

        const RenderScene* scene = view.scene;
        for (uint32 lightIndex = 0; lightIndex < uint32(scene->lights.size()); lightIndex++)
        {
            const LightRenderObject* lightRenderObject = scene->lights[lightIndex];

            if (lightRenderObject->IsLocalLight())
            {
                GPUSceneLocalLightData& localLightShaderParameters = localLightData.emplace_back();
            }
        }

        uint32 localLightCount = uint32(localLightData.size());

        if (localLightCount == 0)
        {
            return;
        }

        const uint32 lightGridSizeX = Math::CeilDiv(renderResolution.width, GLightGridPixelCount);
        const uint32 lightGridSizeY = Math::CeilDiv(renderResolution.height, GLightGridPixelCount);
        const uint32 lightGridSizeZ = GLightGridSizeZ;

        uint32 cellCount = lightGridSizeX * lightGridSizeY * lightGridSizeZ;

        RenderGraphBufferDescription cellDataBufferDesc = RenderGraphBufferDescription::CreateStructured(sizeof(uint32) * 2, cellCount);
        RenderGraphBufferHandle cellDataBuffer = renderGraph.CreateBuffer(cellDataBufferDesc, "LightGridCellDataBuffer");

        RenderGraphBufferDescription lightListBufferDesc = RenderGraphBufferDescription::CreateStructured(sizeof(uint32), cellCount * GLightGridMaxLightCountPerCell);
        RenderGraphBufferHandle lightListBuffer = renderGraph.CreateBuffer(lightListBufferDesc, "LightGridLightListBuffer");

        RenderGraphBufferDescription lightListStartOffsetBufferDesc = RenderGraphBufferDescription::CreateStructured(sizeof(uint32), 1);
        RenderGraphBufferHandle lightListStartOffsetBuffer = renderGraph.CreateBuffer(lightListStartOffsetBufferDesc, "LightGridLightListStartOffsetBuffer");

        GPUScene* gpuScene = view.scene->GetGPUScene();
        RenderGraphBufferHandle localLightDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentLocalLightDataBuffer, "GPUSceneLocalLightDataBuffer");

        renderGraph.AddPass(
            std::format("LightGridBufferInitialization (Compute, {} bytes)", lightListStartOffsetBufferDesc.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                lightListStartOffsetBuffer = builder.WriteBuffer(lightListStartOffsetBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindTextureUAV(0, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(lightListStartOffsetBuffer));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::LightGridBufferInitialization);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        1,
                        1,
                        1);
                };
            });

        renderGraph.AddPass(
            std::format("LightGridLocalLightCulling (Compute, {}x{}x{})", lightGridSizeX, lightGridSizeY, lightGridSizeZ),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                cellDataBuffer = builder.WriteBuffer(cellDataBuffer, RenderBackendResourceState::UnorderedAccess);
                lightListBuffer = builder.WriteBuffer(lightListBuffer, RenderBackendResourceState::UnorderedAccess);
                lightListStartOffsetBuffer = builder.WriteBuffer(lightListStartOffsetBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(lightGridSizeX, GLocalLightCullingThreadGroupSize);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(lightGridSizeY, GLocalLightCullingThreadGroupSize);
                    uint32 threadGroupCountZ = ComputeShaderThreadGroupCount(lightGridSizeZ, GLocalLightCullingThreadGroupSize);

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(localLightDataBuffer));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(cellDataBuffer));
                    pushConstantValues.BindTextureUAV(3, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(lightListBuffer));
                    pushConstantValues.BindTextureUAV(4, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(lightListStartOffsetBuffer));
                    pushConstantValues.OverrideShaderConstantValue(5, localLightCount);
                    pushConstantValues.OverrideShaderConstantValue(6, lightGridSizeX);
                    pushConstantValues.OverrideShaderConstantValue(7, lightGridSizeY);
                    pushConstantValues.OverrideShaderConstantValue(8, lightGridSizeZ);

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::LightGridLocalLightCulling);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        LightGridInfo lightGridInfo;
        lightGridInfo.localLightCount = localLightCount;
        lightGridInfo.cellCount = cellCount;
        lightGridInfo.lightGridSizeX = lightGridSizeX;
        lightGridInfo.lightGridSizeX = lightGridSizeX;
        lightGridInfo.lightGridSizeX = lightGridSizeX;
        lightGridInfo.maxLightCountPerCell = GLightGridMaxLightCountPerCell;

        RasterizationRendererLightGridData& lightGridData = renderGraph.blackboard.Create<RasterizationRendererLightGridData>();
        lightGridData.lightGridCellDataBuffer = cellDataBuffer;
        lightGridData.lightGridLightListBuffer = lightListBuffer;
        lightGridData.lightGridInfo = lightGridInfo;
    }
}