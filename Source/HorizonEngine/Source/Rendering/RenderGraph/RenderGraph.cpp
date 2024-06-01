#include "RenderGraph.h"

#include <optick.h>

namespace Horizon
{
    RenderGraph::RenderGraph(MemoryArena* arena, RenderGraphResourcePool* pool, RenderBackendGPUProfiler* gpuProfiler)
        : blackboard(arena)
        , arena(arena)
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
        uint32 index = (uint32)buffers.size();
        RenderGraphBufferHandle handle = RenderGraphBufferHandle(index, 0);
        RenderGraphBuffer* buffer = AllocObject<RenderGraphBuffer>(name, desc);
        buffers.push_back(buffer);
        dag.RegisterNode(buffer);
        return handle;
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
        RenderBackendBufferHandle renderBackendBufferHandle = externalBuffer->GetHandle();
        RenderGraphBufferHandle found = FindExternalBuffer(renderBackendBufferHandle);
        if (found)
        {
            return found;
        }

        uint32 index = (uint32)buffers.size();
        RenderGraphBufferHandle handle = RenderGraphBufferHandle(index, 0);

        RenderGraphBuffer* buffer = AllocObject<RenderGraphBuffer>(name, externalBuffer->GetDesc());
        buffer->imported = true;
        buffer->initialState = RenderBackendResourceState::ShaderResource;
        buffer->finalState = RenderBackendResourceState::ShaderResource;
        buffer->intermediateState = RenderBackendResourceState::ShaderResource;
        buffer->SetInternalBuffer(externalBuffer, RenderBackendResourceState::ShaderResource);

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

        RenderGraphExportedTexture exportedTexture = {
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

        RenderGraphExportedBuffer exportedBuffer = {
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

        if (!Compile())
        {
            return;
        }

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

        for (RenderGraphPass* pass : passes)
        {
            //if (pass->IsCulled())
            //{
            //    continue;
            //}

            RenderGraphPassFlags passFlags = pass->GetFlags();
            RenderGraphRegistry registry(this, pass);

            uint32 currentPassTimingQueryRegion = gpuProfiler->BeginRegion(&commandList, pass->GetName());

            commandList.BeginDebugLabel(pass->GetName(), Vector4(1.0f, 1.0f, 1.0f, 1.0f));

            for (auto& state : pass->textureStates)
            {
                RenderGraphTexture* texture = state.texture;
                if (state.state != texture->intermediateState)
                {
                    RenderBackendBarrier barrier = RenderBackendBarrier(
                        texture->GetRenderBackendTextureHandle(),
                        RenderBackendTextureSubresourceRange::All,
                        texture->intermediateState,
                        state.state);
                    texture->intermediateState = state.state;
                    pass->barriers.push_back(barrier);
                }
            }

            if (!pass->barriers.empty())
            {
                commandList.Transitions(pass->barriers.data(), (uint32)pass->barriers.size());
            }

            if (EnumClassHasFlags(passFlags, RenderGraphPassFlags::Graphics) && !EnumClassHasFlags(passFlags, RenderGraphPassFlags::SkipRenderPass))
            {
                RenderBackendRenderPassInfo renderPass = {};
                for (uint32 i = 0; i < RenderBackendMaxRenderTargetCount; i++)
                {
                    if (pass->renderTargets[i].texture)
                    {
                        renderPass.renderTargets[i] =
                        {
                            .texture = registry.GetRenderBackendTextureHandle(pass->renderTargets[i].texture),
                            .mipLevel = pass->renderTargets[i].mipLevel,
                            .arrayLayer = pass->renderTargets[i].arrayLayer,
                            .loadOp = pass->renderTargets[i].loadOp,
                            .storeOp = pass->renderTargets[i].storeOp,
                        };
                    }
                }
                if (pass->depthStencil.texture)
                {
                    renderPass.depthStencil =
                    {
                        .texture = registry.GetRenderBackendTextureHandle(pass->depthStencil.texture),
                        .mipLevel = pass->depthStencil.mipLevel,
                        .arrayLayer = pass->depthStencil.arrayLayer,
                        .depthLoadOp = pass->depthStencil.depthLoadOp,
                        .depthStoreOp = pass->depthStencil.depthStoreOp,
                        .stencilLoadOp = pass->depthStencil.stencilLoadOp,
                        .stencilStoreOp = pass->depthStencil.stencilStoreOp,
                    };
                }
                commandList.BeginRenderPass(renderPass);
            }

            pass->Execute(registry, commandList);

            if (EnumClassHasFlags(passFlags, RenderGraphPassFlags::Graphics) && !EnumClassHasFlags(passFlags, RenderGraphPassFlags::SkipRenderPass))
            {
                commandList.EndRenderPass();
            }

            gpuProfiler->EndRegion(currentPassTimingQueryRegion);

            commandList.EndDebugLabel();
        }

        for (auto& texture : textures)
        {
            if (texture->intermediateState != texture->finalState)
            {
                std::array<RenderBackendBarrier, 1> barriers =
                {
                    RenderBackendBarrier(texture->GetRenderBackendTextureHandle(), RenderBackendTextureSubresourceRange::All, texture->intermediateState, texture->finalState)
                };
                commandList.Transitions(barriers.data(), (uint32)barriers.size());
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
}