#include "RenderGraphBuilder.h"
#include "RenderGraph.h"

namespace Horizon
{
    RenderGraphTextureHandle RenderGraphBuilder::CreateTransientTexture(const RenderGraphTextureDescription& desc, const char* name)
    {
        return RenderGraphTextureHandle();
    }

    RenderGraphBufferHandle RenderGraphBuilder::CreateTransientBuffer(const RenderGraphBufferDescription& desc, const char* name)
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

    void RenderGraphBuilder::SetBindlessResourceSRV(uint32 slot, RenderBackendBufferHandle buffer)
    {
        pass->BindUntrackedResource(slot, renderGraph->renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(buffer));
    }

    void RenderGraphBuilder::SetBindlessResourceUAV(uint32 slot, RenderBackendBufferHandle buffer)
    {
        pass->BindUntrackedResource(slot, renderGraph->renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(buffer));
    }

    void RenderGraphBuilder::SetBindlessResourceSRV(uint32 slot, RenderGraphBufferHandle buffer)
    {
        pass->bufferStates.push_back(RenderGraphPass::BufferState{
            .buffer = renderGraph->buffers[buffer.GetIndex()],
            .state = RenderBackendResourceState::ShaderResource });
        pass->inputs.push_back(renderGraph->buffers[buffer.GetIndex()]);
        pass->referenceCount++;

        pass->BindBufferSRV(slot, buffer);
    }

    void RenderGraphBuilder::SetBindlessResourceUAV(uint32 slot, RenderGraphBufferHandle buffer)
    {
        pass->bufferStates.push_back(RenderGraphPass::BufferState{
            .buffer = renderGraph->buffers[buffer.GetIndex()],
            .state = RenderBackendResourceState::UnorderedAccess });
        pass->outputs.push_back(renderGraph->buffers[buffer.GetIndex()]);
        pass->referenceCount++;

        pass->BindBufferUAV(slot, buffer);
    }

    void RenderGraphBuilder::SetBindlessResourceSRV(uint32 slot, RenderGraphTextureHandle texture)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[texture.GetIndex()],
            .initialState = RenderBackendResourceState::ShaderResource,
            .finalState = RenderBackendResourceState::ShaderResource });
        pass->inputs.push_back(renderGraph->textures[texture.GetIndex()]);
        renderGraph->textures[texture.GetIndex()]->referenceCount++;

        pass->BindTextureSRV(slot, texture);
    }

    void RenderGraphBuilder::SetBindlessResourceUAV(uint32 slot, RenderGraphTextureHandle texture, uint32 mipLevel)
    {
        pass->textureStates.push_back(RenderGraphPass::TextureState{
            .texture = renderGraph->textures[texture.GetIndex()],
            .initialState = RenderBackendResourceState::UnorderedAccess,
            .finalState = RenderBackendResourceState::UnorderedAccess });
        pass->outputs.push_back(renderGraph->textures[texture.GetIndex()]);
        pass->referenceCount++;

        pass->BindTextureUAV(slot, texture);
    }

    void RenderGraphBuilder::SetShaderConstantValue(uint32 slot, int32 value)
    {
        pass->BindScalar(slot, value);
    }

    void RenderGraphBuilder::SetShaderConstantValue(uint32 slot, uint32 value)
    {
        pass->BindScalar(slot, value);
    }

    void RenderGraphBuilder::SetShaderConstantValue(uint32 slot, float value)
    {
        pass->BindScalar(slot, value);
    }

    void RenderGraphBuilder::SetRenderTargetBinding(
        uint32 slot,
        RenderGraphTextureHandle handle,
        RenderBackendRenderPassLoadOperation loadOperation,
        RenderBackendRenderPassStoreOperation storeOperation,
        uint32 mipLevel)
    {
        pass->SetRenderTargetBinding(slot, handle, mipLevel, loadOperation, storeOperation);
    }

    void RenderGraphBuilder::SetDepthStencilBinding(
        RenderGraphTextureHandle handle,
        RenderBackendRenderPassLoadOperation depthLoadOperation,
        RenderBackendRenderPassStoreOperation depthStoreOperation,
        RenderBackendRenderPassLoadOperation stencilLoadOperation,
        RenderBackendRenderPassStoreOperation stencilStoreOperation,
        RenderBackendDepthStencilAccessType depthStencilAccessType)
    {
        pass->SetDepthStencilBinding(handle, depthLoadOperation, depthStoreOperation, stencilLoadOperation, stencilStoreOperation, depthStencilAccessType);
    }

    void RenderGraphBuilder::SetRenderArea(int32 x, int32 y, uint32 width, uint32 height)
    {
        assert(EnumClassHasFlags(pass->GetFlags(), RenderGraphPassFlags::Graphics));

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
        assert(EnumClassHasFlags(pass->GetFlags(), RenderGraphPassFlags::Graphics));

        pass->allowUAVWrites = value;
    }
}