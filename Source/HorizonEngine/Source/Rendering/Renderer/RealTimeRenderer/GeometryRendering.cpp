#include "GeometryRendering.h"
#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::SetupGeometryPasses()
    {
        const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[GeometryPassType::Opaque];

        //DynamicMeshCommandBuildRequests
    }

    void RealTimeRenderer::DispatchGeometryOpaquePassDrawCommands(RenderBackendCommandList& commandList)
    {
        const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[GeometryPassType::Opaque];
        const GPUScene* gpuScene = drawCommandList.setupJobData.scene->GetGPUScene();

        RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferVS);
        RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferPS);

        for (uint32 drawCommandIndex = 0; drawCommandIndex < drawCommandList.drawCommandCount; drawCommandIndex++)
        {
            const GeometryPassDrawCommand& drawCommand = drawCommandList.commands[drawCommandIndex];

            RenderBackendGraphicsPipelineState graphicsPipelineState = {};
            graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
            graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
            graphicsPipelineState.depthStencilState.depthTestEnable = true;
            graphicsPipelineState.depthStencilState.depthWriteEnable = true;
            graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

            commandList.SetStencilReference(drawCommand.stencilReference);

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
            shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryBuffer));
            shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceBuffer));
            shaderConstants.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->materialBuffer));

            commandList.DrawIndexed(
                vertexShader,
                pixelShader,
                graphicsPipelineState,
                shaderConstants,
                drawCommand.indexBuffer,
                drawCommand.indexCount,
                drawCommand.instanceCount,
                drawCommand.firstIndex,
                0, // TODO
                drawCommand.firstInstance,
                drawCommand.topology);
        }
    }
}