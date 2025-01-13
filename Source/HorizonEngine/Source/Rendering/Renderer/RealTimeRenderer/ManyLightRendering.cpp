#include "ManyLightRendering.h"
#include "RealTimeRenderer.h"

namespace Horizon
{
    static void SetupLocalLightShaderParameters(LocalLightShaderParameters& outParameters, const LightRenderObject& light)
    {
        outParameters.data0 = Vector4f(light.position, light.radius);
        outParameters.data1 = Vector4f(light.direction, 0.0f);
        outParameters.data2 = Vector4f(light.tangent, 0.0f);
        outParameters.data3 = Vector4f(light.color, 0.0f);
        outParameters.data4 = Vector4f(0.0f);
    }

    static const uint32 GLightGridPixelCount = 64;
    static const uint32 GLightGridSizeZ = 32;
    static const uint32 GLightGridMaxLightCountPerCell = 64;
    static const uint32 GLocalLightCullingThreadGroupSize = 4; // TODO

    void RealTimeRenderer::DispatchLocalLightCulling(
       RenderGraph& renderGraph,
       const SceneView& view)
    {
        std::vector<LocalLightShaderParameters> localLightData;

        const RenderScene* scene = view.scene;
        for (uint32 lightIndex = 0; lightIndex < uint32(scene->lights.size()); lightIndex++)
        {
            const LightRenderObject* lightRenderObject = scene->lights[lightIndex];

            if (lightRenderObject->IsLocalLight())
            {
                LocalLightShaderParameters& localLightShaderParameters = localLightData.emplace_back();
                SetupLocalLightShaderParameters(localLightShaderParameters, *lightRenderObject);
            }
        }

        uint32 localLightCount = uint32(localLightData.size());

        if (localLightCount == 0)
        {
            return;
        }

        // RenderGraphBufferDesc localLightDataBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(LocalLightShaderParameters), localLightCount);
        // RenderGraphBufferHandle localLightDataBuffer = renderGraph.CreateBuffer(localLightDataBufferDesc, "LocalLightDataBuffer");

        // renderGraph.UploadBufferDeferred();

        RenderBackendBufferHandle& localLightDataUploadBuffer = localLightDataUploadBuffers[currentPerFrameDataBufferIndex];
        RenderBackendBufferHandle& localLightDataBuffer = localLightDataBuffers[currentPerFrameDataBufferIndex];
        if (!localLightDataBuffer)
        {
            RenderBackendBufferDesc localLightDataUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(LocalLightShaderParameters) * localLightCount);
            localLightDataUploadBuffer = renderBackend->CreateBuffer(&localLightDataUploadBufferDesc, nullptr, "LocalLightDataUploadBuffer");
            RenderBackendBufferDesc localLightDataBufferDesc = RenderBackendBufferDesc::CreateStructured(sizeof(LocalLightShaderParameters), localLightCount);
            localLightDataBuffer = renderBackend->CreateBuffer(&localLightDataBufferDesc, nullptr, "LocalLightDataBuffer");
        }
        renderBackend->UpdateBuffer(localLightDataUploadBuffer, 0, localLightData.data(), sizeof(LocalLightShaderParameters) * localLightCount);

        renderGraph.AddPass(
            std::format("UpdateLocalLightDataBuffer (Copy, {} bytes)", sizeof(LocalLightShaderParameters) * localLightCount),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.CopyBuffer(
                        localLightDataUploadBuffer,
                        0,
                        localLightDataBuffer,
                        0,
                        sizeof(LocalLightShaderParameters) * localLightCount);
                    RenderBackendBarrier barrier[] =
                    {
                        RenderBackendBarrier(localLightDataBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource)
                    };
                    commandList.Barriers(barrier, 1);
                };
            });

        const uint32 lightGridSizeX = Math::CeilDiv(renderResolution.width, GLightGridPixelCount);
        const uint32 lightGridSizeY = Math::CeilDiv(renderResolution.height, GLightGridPixelCount);
        const uint32 lightGridSizeZ = GLightGridSizeZ;

        uint32 cellCount = lightGridSizeX * lightGridSizeY * lightGridSizeZ;

        RenderGraphBufferDesc cellDataBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(uint32) * 2, cellCount);
        RenderGraphBufferHandle cellDataBuffer = renderGraph.CreateBuffer(cellDataBufferDesc, "LightGridCellDataBuffer");

        RenderGraphBufferDesc lightListBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(uint32), cellCount * GLightGridMaxLightCountPerCell);
        RenderGraphBufferHandle lightListBuffer = renderGraph.CreateBuffer(lightListBufferDesc, "LightGridLightListBuffer");

        RenderGraphBufferDesc lightListStartOffsetBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(uint32), 1);
        RenderGraphBufferHandle lightListStartOffsetBuffer = renderGraph.CreateBuffer(lightListStartOffsetBufferDesc, "LightGridLightListStartOffsetBuffer");

        renderGraph.AddPass(
            std::format("LightGridBufferInitialization (Compute, {} bytes)", lightListStartOffsetBufferDesc.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                lightListStartOffsetBuffer = builder.WriteBuffer(lightListStartOffsetBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindTextureUAV(0, registry.GetBufferUAVBindlessResourceDescriptorIndex(lightListStartOffsetBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LightGridBufferInitialization);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
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
                //localLightDataBuffer = builder.ReadBuffer(localLightDataBuffer, RenderBackendResourceState::ShaderResource);
                cellDataBuffer = builder.WriteBuffer(cellDataBuffer, RenderBackendResourceState::UnorderedAccess);
                lightListBuffer = builder.WriteBuffer(lightListBuffer, RenderBackendResourceState::UnorderedAccess);
                lightListStartOffsetBuffer = builder.WriteBuffer(lightListStartOffsetBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(lightGridSizeX, GLocalLightCullingThreadGroupSize);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(lightGridSizeY, GLocalLightCullingThreadGroupSize);
                    uint32 threadGroupCountZ = ComputeShaderThreadGroupCount(lightGridSizeZ, GLocalLightCullingThreadGroupSize);

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferCBV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(localLightDataBuffer));
                    //shaderConstants.BindTextureSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(localLightDataBuffer));
                    shaderConstants.BindTextureUAV(2, registry.GetBufferUAVBindlessResourceDescriptorIndex(cellDataBuffer));
                    shaderConstants.BindTextureUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(lightListBuffer));
                    shaderConstants.BindTextureUAV(4, registry.GetBufferUAVBindlessResourceDescriptorIndex(lightListStartOffsetBuffer));
                    shaderConstants.BindScalar(5, localLightCount);
                    shaderConstants.BindScalar(6, lightGridSizeX);
                    shaderConstants.BindScalar(7, lightGridSizeY);
                    shaderConstants.BindScalar(8, lightGridSizeZ);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LightGridLocalLightCulling);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
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

        RealTimeRendererLightGridData& lightGridData = renderGraph.blackboard.Create<RealTimeRendererLightGridData>();
        lightGridData.lightGridCellDataBuffer = cellDataBuffer;
        lightGridData.lightGridLightListBuffer = lightListBuffer;
        lightGridData.lightGridInfo = lightGridInfo;
    }
}