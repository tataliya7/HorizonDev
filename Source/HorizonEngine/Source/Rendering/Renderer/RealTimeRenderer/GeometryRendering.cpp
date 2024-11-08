#include "GeometryRendering.h"
#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::SetupGeometryPasses()
    {
        GeometryPassDrawCommandList& opaqueGeometryPassDrawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::Opaque)];
        opaqueGeometryPassDrawCommandList.Clear();

        GeometryPassDrawCommandList& virtualShadowMapPassDrawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::VirtualShadowMap)];
        virtualShadowMapPassDrawCommandList.Clear();

        GeometryPassDrawCommandList& cascadedShadowMapPassDrawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::CascadedShadowMap)];
        cascadedShadowMapPassDrawCommandList.Clear();

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

            opaqueGeometryPassDrawCommandList.AddDrawCommand(drawCommand);
            virtualShadowMapPassDrawCommandList.AddDrawCommand(drawCommand);
            cascadedShadowMapPassDrawCommandList.AddDrawCommand(drawCommand);
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