#include "Rendering/RenderGraph/RenderGraphBuilder.h"
#include "Rendering/RenderGraph/RenderGraph.h"

namespace HE
{
    RenderGraphTextureHandle RenderGraphBuilder::CreateTransientTexture(const RenderGraphTextureDesc& desc, const char* name)
    {
        return RenderGraphTextureHandle();
    }

    RenderGraphBufferHandle RenderGraphBuilder::CreateTransientBuffer(const RenderGraphBufferDesc& desc, const char* name)
    {
        return RenderGraphBufferHandle();
    }

    RenderGraphTextureHandle RenderGraphBuilder::ReadTexture(RenderGraphTextureHandle handle, RenderBackendResourceState finalState, const RenderGraphTextureSubresourceRange& range)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[handle.GetIndex()],
            .state = finalState,
            .subresourceRange = range,
        });
        pass->inputs.push_back(renderGraph->textures[handle.GetIndex()]);
        renderGraph->textures[handle.GetIndex()]->refCount++;
        return handle;
    }

    RenderGraphTextureHandle RenderGraphBuilder::WriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState finalState, const RenderGraphTextureSubresourceRange& range)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[handle.GetIndex()],
            .state = finalState,
            .subresourceRange = range,
        });
        pass->outputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->refCount++;
        return handle;
    }

    RenderGraphTextureHandle RenderGraphBuilder::ReadWriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState finalState, const RenderGraphTextureSubresourceRange& range)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[handle.GetIndex()],
            .state = finalState,
            .subresourceRange = range,
        });
        pass->inputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->outputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->refCount++;
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
        pass->refCount++;
        return handle;
    }

    RenderGraphBufferHandle RenderGraphBuilder::ReadWriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initalState)
    {
        return handle;
    }

    void RenderGraphBuilder::BindColorTarget(uint32 slot, RenderGraphTextureHandle handle, RenderBackendRenderTargetLoadOp loadOp, RenderBackendRenderTargetStoreOp storeOp, uint32 mipLevel, uint32 arraylayer)
    {
        pass->colorTargets[slot] = {
            .texture = handle,
            .mipLevel = mipLevel,
            .arrayLayer = arraylayer,
            .loadOp = loadOp,
            .storeOp = storeOp,
        };
    }

    void RenderGraphBuilder::BindDepthTarget(RenderGraphTextureHandle handle, RenderBackendRenderTargetLoadOp depthLoadOp, RenderBackendRenderTargetStoreOp depthStoreOp, uint32 mipLevel, uint32 arraylayer)
    {
        pass->depthStentcilTarget = {
            .texture = handle,
            .mipLevel = mipLevel,
            .arrayLayer = arraylayer,
            .depthLoadOp = depthLoadOp,
            .depthStoreOp = depthStoreOp,
            .stencilLoadOp = RenderBackendRenderTargetLoadOp::DontCare,
            .stencilStoreOp = RenderBackendRenderTargetStoreOp::DontCare,
        };
    }

    void RenderGraphBuilder::BindDepthStencilTarget(RenderGraphTextureHandle handle, RenderBackendRenderTargetLoadOp depthLoadOp, RenderBackendRenderTargetStoreOp depthStoreOp, RenderBackendRenderTargetLoadOp stencilLoadOp, RenderBackendRenderTargetStoreOp stencilStoreOp, uint32 mipLevel, uint32 arraylayer)
    {
        pass->depthStentcilTarget = {
            .texture = handle,
            .mipLevel = mipLevel,
            .arrayLayer = arraylayer,
            .depthLoadOp = depthLoadOp,
            .depthStoreOp = depthStoreOp,
            .stencilLoadOp = stencilLoadOp,
            .stencilStoreOp = stencilStoreOp,
        };
    }
}