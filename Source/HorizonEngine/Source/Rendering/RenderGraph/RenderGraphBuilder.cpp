#include "RenderGraphBuilder.h"
#include "RenderGraph.h"

namespace Horizon
{
    RenderGraphTextureHandle RenderGraphBuilder::CreateTransientTexture(const RenderGraphTextureDesc& desc, const char* name)
    {
        return RenderGraphTextureHandle();
    }

    RenderGraphBufferHandle RenderGraphBuilder::CreateTransientBuffer(const RenderGraphBufferDesc& desc, const char* name)
    {
        return RenderGraphBufferHandle();
    }

    RenderGraphTextureHandle RenderGraphBuilder::ReadTexture(RenderGraphTextureHandle handle, RenderBackendResourceState finalState)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[handle.GetIndex()],
            .state = finalState,
        });
        pass->inputs.push_back(renderGraph->textures[handle.GetIndex()]);
        renderGraph->textures[handle.GetIndex()]->referenceCount++;
        return handle;
    }

    RenderGraphTextureHandle RenderGraphBuilder::WriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState finalState)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[handle.GetIndex()],
            .state = finalState,
        });
        pass->outputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->referenceCount++;
        return handle;
    }

    RenderGraphTextureHandle RenderGraphBuilder::ReadWriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState finalState)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[handle.GetIndex()],
            .state = finalState,
        });
        pass->inputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->outputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->referenceCount++;
        return handle;
    }

    RenderGraphBufferHandle RenderGraphBuilder::ReadBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initalState)
    {
        return handle;
    }

    RenderGraphBufferHandle RenderGraphBuilder::WriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState state)
    {
        pass->bufferStates.push_back(RenderGraphPass::BufferState{
            .buffer = renderGraph->buffers[handle.GetIndex()],
            .state = state,
        });
        pass->outputs.push_back(renderGraph->buffers[handle.GetIndex()]);
        pass->referenceCount++;
        return handle;
    }

    RenderGraphBufferHandle RenderGraphBuilder::ReadWriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initalState)
    {
        return handle;
    }

    void RenderGraphBuilder::BindRenderTarget(uint32 slot, RenderGraphTextureHandle handle, RenderBackendRenderPassBeginningAccessType loadOp, RenderBackendRenderPassEndingAccessType storeOp, uint32 mipLevel, uint32 arraylayer)
    {
        pass->renderTargets[slot] =
        {
            .texture = handle,
            .mipLevel = mipLevel,
            .arrayLayer = arraylayer,
            .loadOp = loadOp,
            .storeOp = storeOp,
        };
    }

    void RenderGraphBuilder::BindDepthStencil(RenderGraphTextureHandle handle, RenderBackendRenderPassBeginningAccessType depthLoadOp, RenderBackendRenderPassEndingAccessType depthStoreOp, uint32 mipLevel, uint32 arraylayer)
    {
        pass->depthStencil =
        {
            .texture = handle,
            .mipLevel = mipLevel,
            .arrayLayer = arraylayer,
            .depthLoadOp = depthLoadOp,
            .depthStoreOp = depthStoreOp,
            .stencilLoadOp = RenderBackendRenderPassBeginningAccessType::Discard,
            .stencilStoreOp = RenderBackendRenderPassEndingAccessType::Discard,
        };
    }

    void RenderGraphBuilder::BindDepthStencilTarget(RenderGraphTextureHandle handle, RenderBackendRenderPassBeginningAccessType depthLoadOp, RenderBackendRenderPassEndingAccessType depthStoreOp, RenderBackendRenderPassBeginningAccessType stencilLoadOp, RenderBackendRenderPassEndingAccessType stencilStoreOp, uint32 mipLevel, uint32 arraylayer)
    {
        pass->depthStencil =
        {
            .texture = handle,
            .mipLevel = mipLevel,
            .arrayLayer = arraylayer,
            .depthLoadOp = depthLoadOp,
            .depthStoreOp = depthStoreOp,
            .stencilLoadOp = stencilLoadOp,
            .stencilStoreOp = stencilStoreOp,
        };
    }

    void RenderGraphBuilder::SetRenderArea(int32 x, int32 y, uint32 width, uint32 height)
    {
        pass->renderArea =
        {
            .x = x,
            .y = y,
            .width = width,
            .height = height
        };
    }
}