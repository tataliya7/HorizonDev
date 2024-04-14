#include "Rendering/RenderGraph/RenderGraph.h"

#include <optick.h>

namespace HE
{
    struct RenderBackendTransientResource
    {

    };

    RenderGraph::RenderGraph(MemoryArena* arena, RenderBackendGPUProfiler* gpuProfiler)
        : blackboard(arena)
        , arena(arena)
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
        RenderGraphTexture* texture = AllocObject<RenderGraphTexture>(name, desc, RenderGraphTextureFlags::None);
        textures.push_back(texture);
        dag.RegisterNode(texture);
        return handle;
    }

    RenderGraphBufferHandle RenderGraph::CreateBuffer(const RenderGraphBufferDesc& desc, const char* name)
    {
        uint32 index = (uint32)buffers.size();
        RenderGraphBufferHandle handle = RenderGraphBufferHandle(index, 0);
        RenderGraphBuffer* buffer = AllocObject<RenderGraphBuffer>(name, desc, RenderGraphBufferFlags::None);
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
        auto found = importedTextures.find(renderBackendTexture);
        if (found != importedTextures.end())
        {
            return found->second;
        }
        return RenderGraphTextureHandle::Null;
    }

    RenderGraphTextureHandle RenderGraph::ImportExternalTexture(RenderBackendTextureHandle renderBackendTexture, const RenderBackendTextureDesc& desc, RenderBackendResourceState initialState, char const* name)
    {
        uint32 index = (uint32)textures.size();
        RenderGraphTextureHandle handle = RenderGraphTextureHandle(index, 0);
        RenderGraphTexture* texture = AllocObject<RenderGraphTexture>(name, desc, RenderGraphTextureFlags::None);
        texture->imported = true;
        texture->SetRenderBackendTexture(renderBackendTexture, initialState);
        textures.push_back(texture);
        dag.RegisterNode(texture);
        importedTextures.emplace(renderBackendTexture, handle);

        return handle;
    }

    RenderGraphTextureHandle RenderGraph::ImportExternalTexture(const RenderGraphPersistentTexture& externalTexture, RenderGraphTextureFlags flags, char const* name)
    {
        RenderBackendTextureHandle renderBackendTextureHandle = externalTexture.GetRenderBackendTextureHandle();
        RenderGraphTextureHandle found = FindExternalTexture(renderBackendTextureHandle);
        if (found)
        {
            return found;
        }

        uint32 index = (uint32)textures.size();
        RenderGraphTextureHandle handle = RenderGraphTextureHandle(index, 0);
        RenderGraphTexture* texture = AllocObject<RenderGraphTexture>(name, externalTexture.desc, flags);
        texture->imported = true;
        texture->SetRenderBackendTexture(renderBackendTextureHandle, externalTexture.initialState);
        textures.push_back(texture);
        dag.RegisterNode(texture);
        importedTextures.emplace(renderBackendTextureHandle, handle);

        return handle;
    }

    RenderGraphBufferHandle RenderGraph::FindExternalBuffer(RenderBackendBufferHandle renderBackendBuffer)
    {
        auto found = importedBuffers.find(renderBackendBuffer);
        if (found != importedBuffers.end())
        {
            return found->second;
        }
        return RenderGraphBufferHandle::Null;
    }

    RenderGraphBufferHandle RenderGraph::ImportExternalBuffer(RenderBackendBufferHandle renderBackendBuffer, const RenderBackendBufferDesc& desc, RenderBackendResourceState initialState, char const* name)
    {
        uint32 index = (uint32)buffers.size();
        RenderGraphBufferHandle handle = RenderGraphBufferHandle(index, 0);
        RenderGraphBuffer* buffer = AllocObject<RenderGraphBuffer>(name, desc, RenderGraphBufferFlags::None);
        buffer->imported = true;
        buffer->SetRenderBackendBuffer(renderBackendBuffer, initialState);
        buffers.push_back(buffer);
        dag.RegisterNode(buffer);
        importedBuffers.emplace(renderBackendBuffer, handle);
        return handle;
    }

    RenderGraphBufferHandle RenderGraph::ImportExternalBuffer(const RenderGraphPersistentBuffer* persistentBuffer)
    {
        return ImportExternalBuffer(persistentBuffer->buffer, persistentBuffer->desc, persistentBuffer->initialState, persistentBuffer->name.c_str());
    }

    void RenderGraph::ExportTextureDeferred(RenderGraphTextureHandle handle, RenderGraphPersistentTexture* persistentTexture)
    {
        RenderGraphTexture* texture = textures[handle.GetIndex()];
        texture->exported = true;

        RenderGraphExportedTexture exportedTexture = {
            .texture = texture,
            .persistentTexture = persistentTexture,
        };
        exportedTextures.emplace_back(exportedTexture);
    }

    void RenderGraph::ExportBufferDeferred(RenderGraphBufferHandle handle, RenderGraphPersistentBuffer* persistentBuffer)
    {
        RenderGraphBuffer* buffer = buffers[handle.GetIndex()];
        buffer->exported = true;

        RenderGraphExportedBuffer exportedBuffer = {
            .buffer = buffer,
            .persistentBuffer = persistentBuffer,
        };
        exportedBuffers.emplace_back(exportedBuffer);
    }

    bool RenderGraph::Compile()
    {
        if (passes.empty())
        {
            return false;
        }

        auto& nodes = dag.nodes;

        // Push nodes with a 0 reference count on a stack.
        std::vector<RenderGraphNode*> nodesToCull;
        for (RenderGraphNode* node : nodes)
        {
            if (node->GetRefCount() == 0)
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
                input->refCount--;
                if (input->GetRefCount() == 0)
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

    void RenderGraph::Execute(RenderGraphExecuteContext* context)
    {
        OPTICK_EVENT();

        Compile();

        //std::string temp = Graphviz();
        //LogInfo(GLogger, std::format(("{}", temp)));

        RenderBackend* renderBackend = context->renderBackend;
        RenderBackendCommandList* commandList = AllocObject<RenderBackendCommandList>(arena);

        for (auto& texture : textures)
        {
            if (!texture->IsImported() && !texture->HasRenderBackendTexture())
                // if (!texture->IsCulled())
            {
                RenderBackendTextureHandle handle = GRenderGraphResourcePool->FindOrCreateTexture(renderBackend, &texture->GetDesc(), texture->GetName());
                texture->SetRenderBackendTexture(handle, texture->GetDesc().initialState);
            }
        }

        for (auto& buffer : buffers)
        {
            if (!buffer->IsImported() && !buffer->HasRenderBackendBuffer())
            {
                RenderBackendBufferHandle handle = GRenderGraphResourcePool->FindOrCreateBuffer(renderBackend, &buffer->GetDesc(), buffer->GetName());
                buffer->SetRenderBackendBuffer(handle, RenderBackendResourceState::Undefined);
            }
        }

        for (auto& pass : passes)
        {
            /*if (pass->IsCulled())
            {
                continue;
            }*/
            for (auto& state : pass->textureStates)
            {
                RenderGraphTexture* texture = state.texture;
                if (state.state != texture->tempState && !EnumClassHasFlags(texture->desc.flags, RenderBackendTextureCreateFlags::Readback))
                {
                    RenderBackendBarrier barrier = RenderBackendBarrier(
                        texture->GetRenderBackendTexture(),
                        RenderBackendTextureSubresourceRange::All,
                        texture->tempState,
                        state.state);
                    texture->tempState = state.state;
                    pass->barriers.push_back(barrier);
                }
            }
            int a = 1;
        }

        uint32 passIndex = 0;
        for (auto& pass : passes)
        {
            /*
            if (pass->IsCulled())
            {
                continue;
            }
            */
            RenderGraphPassFlags flags = pass->GetFlags();
            RenderGraphRegistry registry(this, pass);

            uint32 currentPassTimingQueryRegion = gpuProfiler->BeginRegion(commandList, pass->GetName());

            commandList->BeginDebugLabel(pass->GetName(), Vector4(1.0f, 1.0f, 1.0f, 1.0f));

            if (!pass->barriers.empty())
            {
                commandList->Transitions(pass->barriers.data(), (uint32)pass->barriers.size());
            }

            if (EnumClassHasFlags(flags, RenderGraphPassFlags::Graphics) && !EnumClassHasFlags(flags, RenderGraphPassFlags::SkipRenderPass))
            {
                RenderBackendRenderPassInfo renderPass = {};
                for (uint32 i = 0; i < RenderBackendMaxNumSimultaneousColorRenderTargets; i++)
                {
                    if (pass->colorTargets[i].texture)
                    {
                        renderPass.colorRenderTargets[i] = {
                            .texture = registry.GetRenderBackendTexture(pass->colorTargets[i].texture),
                            .mipLevel = pass->colorTargets[i].mipLevel,
                            .arrayLayer = pass->colorTargets[i].arrayLayer,
                            .loadOp = pass->colorTargets[i].loadOp,
                            .storeOp = pass->colorTargets[i].storeOp,
                        };
                    }
                }
                if (pass->depthStentcilTarget.texture)
                {
                    renderPass.depthStencilRenderTarget = {
                        .texture = registry.GetRenderBackendTexture(pass->depthStentcilTarget.texture),
                        .mipLevel = pass->depthStentcilTarget.mipLevel,
                        .arrayLayer = pass->depthStentcilTarget.arrayLayer,
                        .depthLoadOp = pass->depthStentcilTarget.depthLoadOp,
                        .depthStoreOp = pass->depthStentcilTarget.depthStoreOp,
                        .stencilLoadOp = pass->depthStentcilTarget.stencilLoadOp,
                        .stencilStoreOp = pass->depthStentcilTarget.stencilStoreOp,
                    };
                }
                commandList->BeginRenderPass(renderPass);
            }

            pass->Execute(registry, *commandList);

            if (EnumClassHasFlags(flags, RenderGraphPassFlags::Graphics) && !EnumClassHasFlags(flags, RenderGraphPassFlags::SkipRenderPass))
            {
                commandList->EndRenderPass();
            }

            gpuProfiler->EndRegion(currentPassTimingQueryRegion);

            commandList->EndDebugLabel();

            passIndex++;
        }

        for (auto& texture : textures)
        {
            if (texture->tempState != texture->finalState)
            {
                std::array<RenderBackendBarrier, 1> barriers = {
                    RenderBackendBarrier(texture->GetRenderBackendTexture(), RenderBackendTextureSubresourceRange::All, texture->tempState, texture->finalState)
                };
                commandList->Transitions(barriers.data(), (uint32)barriers.size());
            }
        }

        context->commandLists.push_back(commandList);

        for (auto& exportTexture : exportedTextures)
        {
            if (!exportTexture.texture->IsImported())
            {
                GRenderGraphResourcePool->ReleaseTexture(exportTexture.texture->texture);
            }
            GRenderGraphResourcePool->CacheTexture(*exportTexture.persistentTexture);
            exportTexture.persistentTexture->name = exportTexture.texture->name;
            exportTexture.persistentTexture->desc = exportTexture.texture->desc;
            exportTexture.persistentTexture->texture = exportTexture.texture->texture;
            exportTexture.persistentTexture->initialState = exportTexture.texture->finalState;
        }

        for (auto& exportBuffer : exportedBuffers)
        {
            if (!exportBuffer.buffer->IsImported())
            {
                GRenderGraphResourcePool->ReleaseBuffer(exportBuffer.buffer->buffer);
            }
            GRenderGraphResourcePool->CacheBuffer(*exportBuffer.persistentBuffer);
            exportBuffer.persistentBuffer->name = exportBuffer.buffer->name;
            exportBuffer.persistentBuffer->desc = exportBuffer.buffer->desc;
            exportBuffer.persistentBuffer->buffer = exportBuffer.buffer->buffer;
            exportBuffer.persistentBuffer->initialState = exportBuffer.buffer->finalState;
        }
    }
}