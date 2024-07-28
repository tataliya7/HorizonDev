#include "GeometryRendering.h"
#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::SetupGeometryPasses()
    {
        GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[GeometryPassType::Opaque];
        drawCommandList.Clear();

        RenderScene* scene = sceneView->scene;
        for (uint32 index = 0; index < scene->meshes.size(); index++)
        {
            MeshRenderObject* mesh = scene->meshes[index];

            GeometryPassDrawCommand drawCommand;
            drawCommand.geometryID = index;
            drawCommand.indexBuffer = mesh->indexBuffer;
            drawCommand.firstIndex = 0;
            drawCommand.indexCount = mesh->indexCount;
            drawCommand.firstInstance = 0;
            drawCommand.instanceCount = 1;
            drawCommand.stencilReference = 0x00;
            drawCommand.topology = RenderBackendPrimitiveTopology::TriangleList;

            drawCommandList.AddDrawCommand(drawCommand);
        }
    }

    void RealTimeRenderer::DispatchOpaqueGeometryPassDrawCommands(RenderBackendCommandList& commandList)
    {
        const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[GeometryPassType::Opaque];
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();//drawCommandList.setupJobData.scene->GetGPUScene();

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
            shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
            shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));

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

    void GeometryPassDrawCommandList::AddDrawCommand(const GeometryPassDrawCommand& command)
    {
        commands.emplace_back(command);
        drawCommandCount++;
    }

    void GeometryPassDrawCommandList::Clear()
    {
        drawCommandCount = 0;
        commands.clear();
    }
}