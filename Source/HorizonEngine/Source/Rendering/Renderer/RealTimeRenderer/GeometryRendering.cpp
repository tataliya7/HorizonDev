#include "GeometryRendering.h"
#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::DispatchGeometryOpaquePassDrawCalls(RenderBackendCommandList& commandList)
    {
        const GeometryPassDrawCallList& drawCallList = geometryPassDrawCallLists[GeometryPassType::Opaque];
        const GPUScene* gpuScene = drawCallList.setupJobData.scene->GetGPUScene();

        RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferVS);
        RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferPS);

        for (uint32 drawCallIndex = 0; drawCallIndex < drawCallList.drawCallCount; drawCallIndex++)
        {
            const GeometryPassDrawCall& drawCall = drawCallList.drawCalls[drawCallIndex];

            RenderBackendGraphicsPipelineState graphicsPipelineState = {};
            graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
            graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
            graphicsPipelineState.depthStencilState.depthTestEnable = true;
            graphicsPipelineState.depthStencilState.depthWriteEnable = true;
            graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

            RenderBackendShaderArguments shaderArguments = {};
            shaderArguments.BindBufferCBV(0, this->GetCurrentPerFrameConstantBuffer());
            shaderArguments.BindBufferSRV(1, gpuScene->geometryBuffer);
            shaderArguments.BindBufferSRV(2, gpuScene->geometryInstanceBuffer);
            shaderArguments.BindScalar(3, drawCall.geometryID);

            commandList.DrawIndexed(
                vertexShader,
                pixelShader,
                graphicsPipelineState,
                shaderArguments,
                drawCall.indexBuffer,
                drawCall.indexCount,
                drawCall.instanceCount,
                drawCall.firstIndex,
                0, // TODO
                drawCall.firstInstance,
                drawCall.topology);
        }
    }
}