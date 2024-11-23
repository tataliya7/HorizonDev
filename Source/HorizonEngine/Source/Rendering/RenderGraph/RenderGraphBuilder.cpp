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

    RenderGraphTextureHandle RenderGraphBuilder::ReadTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initialState)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[handle.GetIndex()],
            .initialState = initialState,
            .finalState = initialState,
        });
        pass->inputs.push_back(renderGraph->textures[handle.GetIndex()]);
        renderGraph->textures[handle.GetIndex()]->referenceCount++;
        return handle;
    }

    RenderGraphTextureHandle RenderGraphBuilder::WriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initialState)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[handle.GetIndex()],
            .initialState = initialState,
            .finalState = initialState,
        });
        pass->outputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->referenceCount++;
        return handle;
    }

    RenderGraphTextureHandle RenderGraphBuilder::WriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initialState, RenderBackendResourceState finalState)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
           .texture = renderGraph->textures[handle.GetIndex()],
           .initialState = initialState,
           .finalState = finalState,
        });

        pass->outputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->referenceCount++;
        return handle;
    }

    RenderGraphTextureHandle RenderGraphBuilder::ReadWriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initialState)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[handle.GetIndex()],
            .initialState = initialState,
            .finalState = initialState,
        });
        pass->inputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->outputs.push_back(renderGraph->textures[handle.GetIndex()]);
        pass->referenceCount++;
        return handle;
    }

    RenderGraphBufferHandle RenderGraphBuilder::ReadBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initialState)
    {
        pass->bufferStates.push_back(RenderGraphPass::BufferState{
            .buffer = renderGraph->buffers[handle.GetIndex()],
            .state = initialState,
        });
        pass->inputs.push_back(renderGraph->buffers[handle.GetIndex()]);
        pass->referenceCount++;
        return handle;
    }

    RenderGraphBufferHandle RenderGraphBuilder::WriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initialState)
    {
        pass->bufferStates.push_back(RenderGraphPass::BufferState{
            .buffer = renderGraph->buffers[handle.GetIndex()],
            .state = initialState,
        });
        pass->outputs.push_back(renderGraph->buffers[handle.GetIndex()]);
        pass->referenceCount++;
        return handle;
    }

    RenderGraphBufferHandle RenderGraphBuilder::ReadWriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initialState)
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

    void RenderGraphBuilder::BindDepthStencil(
        RenderGraphTextureHandle handle,
        RenderBackendRenderPassBeginningAccessType depthLoadOp,
        RenderBackendRenderPassEndingAccessType depthStoreOp,
        bool depthReadOnly,
        uint32 mipLevel,
        uint32 arrayLayer)
    {
        pass->depthStencil =
        {
            .texture = handle,
            .mipLevel = mipLevel,
            .arrayLayer = arrayLayer,
            .depthReadOnly = depthReadOnly,
            .stencilReadOnly = false,
            .depthLoadOp = depthLoadOp,
            .depthStoreOp = depthStoreOp,
            .stencilLoadOp = RenderBackendRenderPassBeginningAccessType::Discard,
            .stencilStoreOp = RenderBackendRenderPassEndingAccessType::Discard,
        };
    }

    void RenderGraphBuilder::BindDepthStencil(
        RenderGraphTextureHandle handle,
        RenderBackendRenderPassBeginningAccessType depthLoadOp,
        RenderBackendRenderPassEndingAccessType depthStoreOp,
        bool depthReadOnly,
        RenderBackendRenderPassBeginningAccessType stencilLoadOp,
        RenderBackendRenderPassEndingAccessType stencilStoreOp,
        bool stencilReadOnly,
        uint32 mipLevel,
        uint32 arrayLayer)
    {
        pass->depthStencil =
        {
            .texture = handle,
            .mipLevel = mipLevel,
            .arrayLayer = arrayLayer,
            .depthReadOnly = depthReadOnly,
            .stencilReadOnly = stencilReadOnly,
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

    void RenderGraphBuilder::SetAllowUAVWrites(bool value)
    {
        pass->allowUAVWrites = value;
    }
}