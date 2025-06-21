#include "RenderGraph.h"

#include <optick.h>

namespace Horizon
{
    RenderGraph::RenderGraph(MemoryArena* arena, RenderGraphResourcePool* pool, RenderBackendGPUProfiler* gpuProfiler)
        : blackboard(arena)
        , arena(arena)
        , renderBackend(pool->backend)
        , resourcePool(pool)
        , gpuProfiler(gpuProfiler)
    {

    }

    RenderGraph::~RenderGraph()
    {
        Clear();
    }

    std::string RenderGraph::Graphviz() const
    {
        std::stringstream stream;
        stream << "digraph render_graph {\n\trankdir = LR;\n"
               << "\tsubgraph cluster_0 {\n"
               << "\t\tstyle=filled;\n"
               << "\t\tcolor = lightgrey;\n"
               << "\t\tlabel = \"Graphics\"\n"
               << "\t\tnode[shape = rect, style = filled, color = white];\n";
        for (const auto& pass : passes)
        {
            if (pass->IsCulled())
            {
                continue;
            }
            pass->Graphviz(stream);
        }
        stream << "\t}\n}\n";
        return std::move(stream.str());
    }

    RenderGraphTextureHandle RenderGraph::CreateTexture(const RenderGraphTextureDesc& desc, const char* name)
    {
        uint32 index = (uint32)textures.size();
        RenderGraphTextureHandle handle = RenderGraphTextureHandle(index, 0);
        RenderGraphTexture* texture = AllocObject<RenderGraphTexture>(name, desc);
        textures.push_back(texture);
        dag.RegisterNode(texture);
        return handle;
    }

    RenderGraphBufferHandle RenderGraph::CreateBuffer(const RenderGraphBufferDesc& desc, const char* name)
    {
        uint32 index = uint32(buffers.size());
        RenderGraphBufferHandle handle = RenderGraphBufferHandle(index, 0);
        RenderGraphBuffer* buffer = AllocObject<RenderGraphBuffer>(name, desc);
        buffers.push_back(buffer);
        dag.RegisterNode(buffer);
        return handle;
    }

    void RenderGraph::UploadBufferDeferred(RenderGraphBufferHandle buffer, const void* data, uint64 size, RenderGraphSourceDataLifetimeHint hint)
    {
        if ((data != nullptr) && (size > 0))
        {
            if ((hint != RenderGraphSourceDataLifetimeHint::ValidUntilExecution))
            {
                uint64 alignment = 16; // @todo
                void* dataCopy = arena->Alloc(size, alignment);
                if (dataCopy != nullptr)
                {
                    std::memcpy(dataCopy, data, size);
                    RenderGraphBufferUploadJobDescription& bufferUploadJob = bufferUploadJobs.emplace_back();
                    bufferUploadJob.buffer = buffers[buffer.GetIndex()];
                    bufferUploadJob.data = dataCopy;
                    bufferUploadJob.size = size;
                }
            }
            else
            {
                RenderGraphBufferUploadJobDescription& bufferUploadJob = bufferUploadJobs.emplace_back();
                bufferUploadJob.buffer = buffers[buffer.GetIndex()];
                bufferUploadJob.data = data;
                bufferUploadJob.size = size;
            }
        }
    }

    void RenderGraph::ExecuteBufferUploadJobs(RenderBackendCommandList& commandList)
    {
        commandList.BeginDebugLabel("UploadBuffers", Vector4f(1.0f, 1.0f, 1.0f, 1.0f));

        // @todo Adapt to UMA devices to avoid extra copying overhead.
        if (!bufferUploadJobs.empty())
        {
            for (RenderGraphBufferUploadJobDescription& bufferUploadJob : bufferUploadJobs)
            {
                RenderGraphBuffer* dstBuffer = bufferUploadJob.buffer;

                RenderGraphStagingBuffer* stagingBuffer = resourcePool->AllocateStagingBuffer(bufferUploadJob.size);
                renderBackend->UpdateBuffer(stagingBuffer->GetHandle(), 0, bufferUploadJob.data, bufferUploadJob.size);

                assert((bufferUploadJob.data != nullptr) && (bufferUploadJob.size > 0));

                commandList.CopyBuffer(
                    stagingBuffer->GetHandle(),
                    0,
                    dstBuffer->GetRenderBackendBufferHandle(),
                    0,
                    bufferUploadJob.size);

                RenderBackendBarrier barrier[] =
                {
                    RenderBackendBarrier(dstBuffer->GetRenderBackendBufferHandle(), RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource)
                };
                commandList.Barriers(barrier, 1);
            }

            bufferUploadJobs.clear();
        }

        commandList.EndDebugLabel();
    }

    //RenderGraphTextureSRVHandle RenderGraph::CreateSRV(RenderGraphTextureHandle texture, const RenderGraphTextureSRVDesc& desc)
    //{
    //    uint32 index = (uint32)textureSRVs.size();
    //    RenderGraphTextureSRVHandle handle = RenderGraphTextureSRVHandle(index, 0);
    //    RenderGraphTextureSRV* srv = AllocObject<RenderGraphTextureSRV>(texture, desc);
    //    textureSRVs.push_back(srv);
    //    return handle;
    //}
    //
    //RenderGraphTextureUAVHandle RenderGraph::CreateUAV(RenderGraphTextureHandle texture, uint32 mipLevel)
    //{
    //    uint32 index = (uint32)textureUAVs.size();
    //    RenderGraphTextureUAVHandle handle = RenderGraphTextureUAVHandle(index, 0);
    //    RenderGraphTextureUAV* uav = AllocObject<RenderGraphTextureUAV>(texture, mipLevel);
    //    textureUAVs.push_back(uav);
    //    return handle;
    //}
    //
    //RenderGraphBufferUAVHandle RenderGraph::CreateUAV(RenderGraphBufferHandle buffer)
    //{
    //    uint32 index = (uint32)bufferUAVs.size();
    //    RenderGraphBufferUAVHandle handle = RenderGraphBufferUAVHandle(index, 0);
    //    RenderGraphBufferUAV* uav = AllocObject<RenderGraphBufferUAV>(buffer);
    //    bufferUAVs.push_back(uav);
    //    return handle;
    //}

    RenderGraphTextureHandle RenderGraph::FindExternalTexture(RenderBackendTextureHandle renderBackendTexture)
    {
        auto found = importedTextures.find(renderBackendTexture.ToUnit64());
        if (found != importedTextures.end())
        {
            return found->second;
        }
        return RenderGraphTextureHandle::Null;
    }

    RenderGraphBufferHandle RenderGraph::FindExternalBuffer(RenderBackendBufferHandle renderBackendBuffer)
    {
        auto found = importedBuffers.find(renderBackendBuffer.ToUnit64());
        if (found != importedBuffers.end())
        {
            return found->second;
        }
        return RenderGraphBufferHandle::Null;
    }

    RenderGraphTextureHandle RenderGraph::ImportExternalTexture(RenderGraphPersistentTexture* externalTexture, char const* name)
    {
        if (!externalTexture)
        {
            return RenderGraphTextureHandle::Null;
        }

        RenderBackendTextureHandle renderBackendTextureHandle = externalTexture->GetHandle();
        RenderGraphTextureHandle found = FindExternalTexture(renderBackendTextureHandle);
        if (found)
        {
            return found;
        }

        uint32 index = (uint32)textures.size();
        RenderGraphTextureHandle handle = RenderGraphTextureHandle(index, 0);

        RenderGraphTexture* texture = AllocObject<RenderGraphTexture>(name, externalTexture->GetDesc());
        texture->imported = true;
        texture->initialState = RenderBackendResourceState::ShaderResource;
        texture->finalState = RenderBackendResourceState::ShaderResource;
        texture->intermediateState = RenderBackendResourceState::ShaderResource;
        texture->SetInternalTexture(externalTexture, RenderBackendResourceState::ShaderResource);

        dag.RegisterNode(texture);
        textures.push_back(texture);
        importedTextures.emplace(renderBackendTextureHandle.ToUnit64(), handle);

        return handle;
    }

    RenderGraphBufferHandle RenderGraph::ImportExternalBuffer(RenderGraphPersistentBuffer* externalBuffer, char const* name)
    {
        if (!externalBuffer)
        {
            return RenderGraphBufferHandle::Null;
        }

        RenderBackendBufferHandle renderBackendBufferHandle = externalBuffer->GetHandle();
        RenderGraphBufferHandle found = FindExternalBuffer(renderBackendBufferHandle);
        if (found)
        {
            return found;
        }

        uint32 index = (uint32)buffers.size();
        RenderGraphBufferHandle handle = RenderGraphBufferHandle(index, 0);

        const RenderBackendBufferDesc& bufferDesc = externalBuffer->GetDesc();

        RenderBackendResourceState state = RenderBackendResourceState::ShaderResource;
        if (EnumClassHasFlags(bufferDesc.flags, RenderBackendBufferCreateFlags::Readback))
        {
            state = RenderBackendResourceState::CopyDst;
        }

        RenderGraphBuffer* buffer = AllocObject<RenderGraphBuffer>(name, bufferDesc);
        buffer->imported = true;
        buffer->initialState = state;
        buffer->finalState = state;
        buffer->intermediateState = state;
        buffer->SetInternalBuffer(externalBuffer, state);

        dag.RegisterNode(buffer);
        buffers.push_back(buffer);
        importedBuffers.emplace(renderBackendBufferHandle.ToUnit64(), handle);

        return handle;
    }

    void RenderGraph::ExportTextureDeferred(RenderGraphTextureHandle handle, RenderGraphPersistentTexture** persistentTexture)
    {
        RenderGraphTexture* texture = textures[handle.GetIndex()];
        texture->exported = true;
        texture->NeverCull();

        assert(!texture->IsImported());

        RenderGraphExportedTexture exportedTexture =
        {
            .source = texture,
            .target = persistentTexture,
        };
        exportedTextures.emplace_back(exportedTexture);
    }

    void RenderGraph::ExportBufferDeferred(RenderGraphBufferHandle handle, RenderGraphPersistentBuffer** persistentBuffer)
    {
        RenderGraphBuffer* buffer = buffers[handle.GetIndex()];
        buffer->exported = true;
        buffer->NeverCull();

        assert(!buffer->IsImported());

        RenderGraphExportedBuffer exportedBuffer =
        {
            .source = buffer,
            .target = persistentBuffer,
        };
        exportedBuffers.emplace_back(exportedBuffer);
    }

    const RenderGraphTextureDesc& RenderGraph::GetTextureDesc(RenderGraphTextureHandle handle) const
    {
        return textures[handle.GetIndex()]->GetDesc();
    }

    const RenderGraphBufferDesc& RenderGraph::GetBufferDesc(RenderGraphBufferHandle handle) const
    {
        return buffers[handle.GetIndex()]->GetDesc();
    }

    bool RenderGraph::Compile()
    {
        if (passes.empty())
        {
            return false;
        }

        // Push nodes with a 0 reference count on a stack.
        std::vector<RenderGraphNode*> nodesToCull;
        for (RenderGraphNode* node : dag.nodes)
        {
            if (node->GetReferenceCount() == 0)
            {
                nodesToCull.push_back(node);
            }
        }

        while (!nodesToCull.empty())
        {
            RenderGraphNode* node = nodesToCull.back();
            nodesToCull.pop_back();
            for (RenderGraphNode* input : node->inputs)
            {
                input->referenceCount--;
                if (input->GetReferenceCount() == 0)
                {
                    nodesToCull.push_back(input);
                }
            }
        }

        return true;
    }

    void RenderGraph::Clear()
    {
        for (auto& pass : passes)
        {
            std::destroy_at(pass);
        }

        for (auto& texture : textures)
        {
            std::destroy_at(texture);
        }

        for (auto& buffer : buffers)
        {
            std::destroy_at(buffer);
        }

        dag.Clear();
        passes.clear();
        textures.clear();
        buffers.clear();
        importedTextures.clear();
        importedBuffers.clear();
    }

    void RenderGraph::Execute(RenderBackendCommandList& commandList)
    {
        OPTICK_EVENT();

        //if (!Compile())
        //{
        //    return;
        //}

        for (RenderGraphTexture* texture : textures)
        {
            if (/* !texture->IsCulled() && */!texture->IsImported() && !texture->HasInternalTexture())
            {
                RenderGraphPersistentTexture* newTexture = resourcePool->AllocateTexture(texture->GetDesc(), texture->GetName());
                texture->SetInternalTexture(newTexture, RenderBackendResourceState::Undefined);
            }
        }

        for (RenderGraphBuffer* buffer : buffers)
        {
            if (/* !buffer->IsCulled() && */!buffer->IsImported() && !buffer->HasInternalBuffer())
            {
                RenderGraphPersistentBuffer* newBuffer = resourcePool->AllocateBuffer(buffer->GetDesc(), buffer->GetName());
                buffer->SetInternalBuffer(newBuffer, RenderBackendResourceState::Undefined);
            }
        }

        ExecuteBufferUploadJobs(commandList);

        for (RenderGraphPass* pass : passes)
        {
            //if (pass->IsCulled())
            //{
            //    continue;
            //}

            RenderGraphPassFlags passFlags = pass->GetFlags();
            RenderGraphResourceRegistry resourceRegistry(this, pass);

            uint32 currentPassTimingQueryRegion = 0;

            if (!EnumClassHasFlags(passFlags, RenderGraphPassFlags::DebugLabelRegion_DEPRECATED))
            {
                commandList.BeginDebugLabel(pass->GetName(), Vector4f(1.0f, 1.0f, 1.0f, 1.0f));
                currentPassTimingQueryRegion = gpuProfiler->BeginRegion(&commandList, pass->GetName());
            }

            for (RenderGraphPass::TextureState& state : pass->textureStates)
            {
                RenderGraphTexture* texture = state.texture;
                if (state.initialState != texture->intermediateState)
                {
                    RenderBackendBarrier barrier = RenderBackendBarrier(
                        texture->GetRenderBackendTextureHandle(),
                        RenderBackendTextureSubresourceRange::All,
                        texture->intermediateState,
                        state.initialState);
                    texture->intermediateState = state.initialState;
                    pass->barriers.push_back(barrier);

                    // LogVerbose(GLogger, std::format("Render Graph: Texture State Transition: {}, initial state: {}, state before: {}, state after: {}",
                    //     texture->GetName(),
                    //     int(texture->initialState),
                    //     int(texture->intermediateState),
                    //     int(state.initialState)));
                }
                // TODO: mark write/read
                else if ((state.initialState == RenderBackendResourceState::UnorderedAccess) && (texture->intermediateState == RenderBackendResourceState::UnorderedAccess))
                {
                    RenderBackendBarrier barrier = RenderBackendBarrier(
                        texture->GetRenderBackendTextureHandle(),
                        RenderBackendTextureSubresourceRange::All,
                        RenderBackendResourceState::UnorderedAccess,
                        RenderBackendResourceState::UnorderedAccess);
                    texture->intermediateState = state.initialState;
                    pass->barriers.push_back(barrier);
                }

                // Work around
                if (texture->finalState != state.initialState && state.finalState != texture->intermediateState)
                {
                    texture->intermediateState = state.finalState;
                }
            }

            for (RenderGraphPass::BufferState& state : pass->bufferStates)
            {
                RenderGraphBuffer* buffer = state.buffer;
                if (state.state != buffer->intermediateState)
                {
                    RenderBackendBarrier barrier = RenderBackendBarrier(
                        buffer->GetRenderBackendBufferHandle(),
                        RenderBackendBufferSubresourceRange::Whole,
                        buffer->intermediateState,
                        state.state);
                    buffer->intermediateState = state.state;
                    pass->barriers.push_back(barrier);
                }
                // TODO: mark write/read
                else if ((state.state == RenderBackendResourceState::UnorderedAccess) && (buffer->intermediateState == RenderBackendResourceState::UnorderedAccess))
                {
                    RenderBackendBarrier barrier = RenderBackendBarrier(
                        buffer->GetRenderBackendBufferHandle(),
                        RenderBackendBufferSubresourceRange::Whole,
                        RenderBackendResourceState::UnorderedAccess,
                        RenderBackendResourceState::UnorderedAccess);
                    buffer->intermediateState = state.state;
                    pass->barriers.push_back(barrier);
                }
            }

            if (!pass->barriers.empty())
            {
                commandList.Barriers(pass->barriers.data(), (uint32)pass->barriers.size());
            }

            if (EnumClassHasFlags(passFlags, RenderGraphPassFlags::Graphics) && !EnumClassHasFlags(passFlags, RenderGraphPassFlags::SkipRenderPass))
            {
                RenderBackendRenderPassInfo renderPass = {};

                renderPass.renderArea = pass->renderArea;
                renderPass.renderPassFlags.allowUAVWrites = pass->allowUAVWrites;

                for (uint32 slot = 0; slot < RenderBackendMaxRenderTargetCount; slot++)
                {
                    const RenderGraphRenderTargetBinding& renderTargetBinding = pass->GetRenderTargetBinding(slot);

                    if (renderTargetBinding.texture)
                    {
                        RenderGraphTexture* renderTargetTexture = textures[renderTargetBinding.texture.GetIndex()];
                        RenderBackendTextureViewHandle renderTargetView = renderTargetTexture->internalTexture->FindOrCreateRenderTargetView(renderTargetBinding.mipLevel);

                        renderPass.renderTargets[slot] =
                        {
                            .texture = resourceRegistry.GetRenderBackendTextureHandle(renderTargetBinding.texture),
                            .mipLevel = renderTargetBinding.mipLevel,
                            .loadOperation = renderTargetBinding.loadOperation,
                            .storeOperation = renderTargetBinding.storeOperation,
                        };
                    }
                }

                const RenderGraphDepthStencilBinding& depthStencilBinding = pass->GetDepthStencilBinding();

                if (depthStencilBinding.texture)
                {
                    renderPass.depthStencil =
                    {
                        .texture = resourceRegistry.GetRenderBackendTextureHandle(depthStencilBinding.texture),
                        .mipLevel = 0,
                        .depthLoadOperation = depthStencilBinding.depthLoadOperation,
                        .depthStoreOperation = depthStencilBinding.depthStoreOperation,
                        .stencilLoadOperation = depthStencilBinding.stencilLoadOperation,
                        .stencilStoreOperation = depthStencilBinding.stencilStoreOperation,
                        .depthStencilAccessType = depthStencilBinding.depthStencilAccessType,
                    };
                }

                commandList.BeginRenderPass(renderPass);
            }

            pass->Execute(commandList, resourceRegistry);

            if (EnumClassHasFlags(passFlags, RenderGraphPassFlags::Graphics) && !EnumClassHasFlags(passFlags, RenderGraphPassFlags::SkipRenderPass))
            {
                commandList.EndRenderPass();
            }

            if (!EnumClassHasFlags(passFlags, RenderGraphPassFlags::DebugLabelRegion_DEPRECATED))
            {
                gpuProfiler->EndRegion(currentPassTimingQueryRegion);
                commandList.EndDebugLabel();
            }
        }

        for (RenderGraphTexture* texture : textures)
        {
            if (texture->intermediateState != texture->finalState)
            {
                std::array<RenderBackendBarrier, 1> barriers =
                {
                    RenderBackendBarrier(texture->GetRenderBackendTextureHandle(), RenderBackendTextureSubresourceRange::All, texture->intermediateState, texture->finalState)
                };
                commandList.Barriers(barriers.data(), (uint32)barriers.size());

                // LogVerbose(GLogger, std::format("Render Graph: Texture State Transition: {}, initial state: {}, state before: {}, state after: {}",
                //     texture->GetName(),
                //     int(texture->initialState),
                //     int(texture->intermediateState),
                //     int(texture->finalState)));
            }
        }

        for (RenderGraphExportedTexture& textureToExport : exportedTextures)
        {
            if (*textureToExport.target != nullptr)
            {
                resourcePool->ReleaseTexture(*textureToExport.target);
            }
            *textureToExport.target = textureToExport.source->internalTexture;
            textureToExport.source->internalTexture = nullptr;
        }

        for (RenderGraphExportedBuffer& bufferToExport : exportedBuffers)
        {
            if (*bufferToExport.target != nullptr)
            {
                resourcePool->ReleaseBuffer(*bufferToExport.target);
            }
            *bufferToExport.target = bufferToExport.source->internalBuffer;
            bufferToExport.source->internalBuffer = nullptr;
        }
    }

    RenderGraphDebugLabelRegion::RenderGraphDebugLabelRegion(RenderGraph& renderGraph, const char* name)
       : renderGraph(&renderGraph)
       , name(name)
    {
        this->renderGraph->AddPass(
            std::format("DebugLabelRegionBegin"),
            RenderGraphPassFlags::DebugLabelRegion_DEPRECATED | RenderGraphPassFlags::NeverGetCulled,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.BeginDebugLabel(name, Vector4f(1.0f, 1.0f, 1.0f, 1.0f));
                };
            });
    }

    RenderGraphDebugLabelRegion::~RenderGraphDebugLabelRegion()
    {
        this->renderGraph->AddPass(
            std::format("DebugLabelRegionEnd"),
            RenderGraphPassFlags::DebugLabelRegion_DEPRECATED | RenderGraphPassFlags::NeverGetCulled,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.EndDebugLabel();
                };
            });
    }
}