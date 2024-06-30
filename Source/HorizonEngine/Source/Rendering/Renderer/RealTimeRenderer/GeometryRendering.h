#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    enum GeometryPassType : uint8
    {
        Opaque,
        Translucency,
        EditorSelection,
        EditorPickingProxy,
        Count
    };

    struct GeometryPassSetupJobData
    {
        const RenderScene* scene;
        GeometryPassType passType;
    };

    struct GeometryPassDrawCall
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

    class GeometryPassDrawCallList
    {
    public:

        void DispatchDraw(RenderBackendCommandList& commandList);

//    private:
        GeometryPassSetupJobData setupJobData;

        uint32 drawCallCount;

        std::vector<GeometryPassDrawCall> drawCalls;
    };
}