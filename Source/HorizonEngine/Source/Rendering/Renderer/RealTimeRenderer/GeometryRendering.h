#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    enum class GeometryPassType : uint8
    {
        Opaque,
        Translucency,
        VirtualShadowMap,
        CascadedShadowMap,
        EditorSelection,
        EditorPickingProxy,
        Count
    };

    struct GeometryPassSetupJobData
    {
        const RenderScene* scene;
        GeometryPassType passType;
    };

    struct GeometryPassDrawCommand
    {
        uint32 geometryID;

        RenderBackendBufferHandle indexBuffer;

        uint32 firstIndex;
        uint32 indexCount;
        uint32 firstInstance;
        uint32 instanceCount;

        uint8 stencilReference;

        RenderBackendPrimitiveTopology topology;
    };

    class GeometryPassDrawCommandList
    {
    public:

        void Clear();

        void DispatchDraw(RenderBackendCommandList& commandList);

        void AddDrawCommand(const GeometryPassDrawCommand& command);

//    private:
        GeometryPassSetupJobData setupJobData;

        uint32 drawCommandCount;

        std::vector<GeometryPassDrawCommand> commands;
    };
}