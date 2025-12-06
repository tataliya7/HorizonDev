module;

#include "Rendering/RenderGraph/RenderGraphCommon.h"
#include "Rendering/RenderGraph/RenderGraphHandles.h"
#include "Rendering/RenderGraph/RenderGraphNode.h"
#include "Rendering/RenderGraph/RenderGraphPass.h"
#include "Rendering/RenderGraph/RenderGraphResources.h"
#include "Rendering/RenderGraph/RenderGraphBuilder.h"
#include "Rendering/RenderGraph/RenderGraphResourceRegistry.h"
#include "Rendering/RenderGraph/RenderGraphBlackboard.h"
#include "Rendering/RenderGraph/RenderGraph.h"

export module Horizon.Rendering.RenderGraph;

export namespace Horizon
{
    using Horizon::RenderGraph;
    using Horizon::RenderGraphBufferHandle;
    using Horizon::RenderGraphTextureHandle;
    using Horizon::RenderGraphPassType;
    using Horizon::RenderGraphPassFlags;
    using Horizon::RenderGraphLambdaPass;
    using Horizon::RenderGraphBuilder;
    using Horizon::RenderGraphResourceRegistry;
    using Horizon::RenderGraphPersistentBuffer;
    using Horizon::RenderGraphPersistentTexture;
}