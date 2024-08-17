#include "ManyLightRendering.h"
#include "RealTimeRenderer.h"

namespace Horizon
{
    static void SetupLocalLightShaderParameters(
        LocalLightShaderParameters& outParameters,
        const LocalLightRenderData& lightRenderData)
    {
        outParameters.data0 = Vector4f(lightRenderData.position, lightRenderData.inverseRadius);
        outParameters.data1 = Vector4f(lightRenderData.direction, 0.0f);
        outParameters.data2 = Vector4f(lightRenderData.tangent, 0.0f);
        outParameters.data3 = Vector4f(lightRenderData.color, 0.0f);
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
                LocalLightRenderData localLightRenderData;
                lightRenderObject->SetupLocalLightRenderData(localLightRenderData);

                LocalLightShaderParameters& localLightShaderParameters = localLightData.emplace_back();

                SetupLocalLightShaderParameters(localLightShaderParameters, localLightRenderData);
            }
        }

        uint32 localLightCount = uint32(localLightData.size());

        RenderGraphBufferDesc localLightDataBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(LocalLightShaderParameters), localLightCount);
        RenderGraphBufferHandle localLightDataBuffer = renderGraph.CreateBuffer(localLightDataBufferDesc, "LocalLightDataBuffer");

        //renderGraph.UploadBufferDeferred();

        const uint32 lightGridSizeX = CeilDiv(renderResolution.width, GLightGridPixelCount);
        const uint32 lightGridSizeY = CeilDiv(renderResolution.height, GLightGridPixelCount);
        const uint32 lightGridSizeZ = GLightGridSizeZ;

        uint32 cellCount = lightGridSizeX * lightGridSizeY * lightGridSizeZ;

        RenderGraphBufferDesc cellDataBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(uint32) * 2, cellCount);
        RenderGraphBufferHandle cellDataBuffer = renderGraph.CreateBuffer(cellDataBufferDesc, "LightGridCellDataBuffer");

        RenderGraphBufferDesc lightListBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(uint32), cellCount * GLightGridMaxLightCountPerCell);
        RenderGraphBufferHandle lightListBuffer = renderGraph.CreateBuffer(lightListBufferDesc, "LightGridLightListBuffer");

        RenderGraphBufferDesc lightListStartOffsetBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(uint32), 1);
        RenderGraphBufferHandle lightListStartOffsetBuffer = renderGraph.CreateBuffer(lightListStartOffsetBufferDesc, "LightGridLightListStartOffsetBuffer");

        renderGraph.AddPass(
            std::format("LocalLightCullingClearBuffer ({} bytes)", lightListStartOffsetBufferDesc.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                lightListStartOffsetBuffer = builder.WriteBuffer(lightListStartOffsetBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindTextureUAV(0, registry.GetBufferUAVBindlessResourceDescriptorIndex(lightListStartOffsetBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalLightCullingClearBuffer);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        1,
                        1,
                        1);
                };
            });

        renderGraph.AddPass(
            std::format("LocalLightCulling (Compute, {}x{}x{})", lightGridSizeX, lightGridSizeY, lightGridSizeZ),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                localLightDataBuffer = builder.ReadBuffer(localLightDataBuffer, RenderBackendResourceState::ShaderResource);
                cellDataBuffer = builder.WriteBuffer(cellDataBuffer, RenderBackendResourceState::UnorderedAccess);
                lightListBuffer = builder.WriteBuffer(lightListBuffer, RenderBackendResourceState::UnorderedAccess);
                lightListStartOffsetBuffer = builder.WriteBuffer(lightListStartOffsetBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(lightGridSizeX, GLocalLightCullingThreadGroupSize);
                    uint32 threadGroupCountY = CeilDiv(lightGridSizeY, GLocalLightCullingThreadGroupSize);
                    uint32 threadGroupCountZ = CeilDiv(lightGridSizeZ, GLocalLightCullingThreadGroupSize);

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(localLightDataBuffer));
                    shaderConstants.BindTextureUAV(2, registry.GetBufferUAVBindlessResourceDescriptorIndex(cellDataBuffer));
                    shaderConstants.BindTextureUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(lightListBuffer));
                    shaderConstants.BindTextureUAV(4, registry.GetBufferUAVBindlessResourceDescriptorIndex(lightListStartOffsetBuffer));
                    shaderConstants.BindScalar(5, localLightCount);
                    shaderConstants.BindScalar(6, lightGridSizeX);
                    shaderConstants.BindScalar(7, lightGridSizeY);
                    shaderConstants.BindScalar(8, lightGridSizeZ);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalLightCulling);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });
    }
}