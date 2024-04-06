#pragma once

#include "Rendering/RenderGraph/RenderGraphCommon.h"
#include "Rendering/RenderGraph/RenderGraphHandles.h"
#include "Rendering/RenderGraph/RenderGraphNode.h"
#include "Rendering/RenderGraph/RenderGraphPasses.h"
#include "Rendering/RenderGraph/RenderGraphResources.h"
#include "Rendering/RenderGraph/RenderGraphBuilder.h"
#include "Rendering/RenderGraph/RenderGraphRegistry.h"
#include "Rendering/RenderGraph/RenderGraphBlackboard.h"
#include "Rendering/RenderGraph/RenderGraphUtils.h"

namespace HE
{
    class RenderGraph;
    class RenderGraphPass;
    class RenderBackendGPUProfiler;

    struct RenderGraphExecuteContext
    {
        RenderBackend* renderBackend;
        std::vector<RenderBackendCommandList*> commandLists;

        RenderGraphExecuteContext() = default;
        ~RenderGraphExecuteContext()
        {
            for (RenderBackendCommandList* commandList : commandLists)
            {
                std::destroy_at(commandList);
            }
        }
    };

    struct RenderGraphEventScope
    {
        RenderGraphEventScope(RenderGraph& renderGraph, const std::string& name)
        {

        }

        ~RenderGraphEventScope()
        {

        }
    };

    class RenderGraph
    {
    public:

        RenderGraph(MemoryArena* arena, RenderBackendGPUProfiler* gpuProfiler);
        RenderGraph(const RenderGraph& other) = delete;
        virtual ~RenderGraph();

        template<typename SetupLambdaType>
        void AddPass(const std::string& name, RenderGraphPassFlags flags, SetupLambdaType setup);

        void Execute(RenderGraphExecuteContext* context);

        void Clear();

        /**
         * @brief Create a string using the Graphviz format.
         * @note Compile() should be called before calling this function.
         * @return std::string in the Graphviz format.
         */
        std::string Graphviz() const;

        RenderGraphTextureHandle CreateTexture(const RenderGraphTextureDesc& desc, const char* name);
        RenderGraphBufferHandle CreateBuffer(const RenderGraphBufferDesc& desc, const char* name);
        //RenderGraphTextureSRVHandle CreateTextureSRV(RenderGraphTextureHandle texture, const RenderGraphTextureSRVDesc& desc);
        //RenderGraphTextureUAVHandle CreateTextureUAV(RenderGraphTextureHandle texture, uint32 mipLevel);
        RenderGraphTextureHandle FindExternalTexture(RenderBackendTextureHandle renderBackendTexture);
        RenderGraphTextureHandle ImportExternalTexture(RenderBackendTextureHandle renderBackendTexture, const RenderBackendTextureDesc& desc, RenderBackendResourceState initialState, char const* name);
        RenderGraphBufferHandle FindExternalBuffer(RenderBackendBufferHandle renderBackendBuffer);
        RenderGraphBufferHandle ImportExternalBuffer(RenderBackendBufferHandle renderBackendBuffer, const RenderBackendBufferDesc& desc, RenderBackendResourceState initialState, char const* name);
        RenderGraphBufferHandle ImportExternalBuffer(const RenderGraphPersistentBuffer* persistentBuffer);
        void ExportTextureDeferred(RenderGraphTextureHandle handle, RenderGraphPersistentTexture* persistentTexture);
        void ExportBufferDeferred(RenderGraphBufferHandle handle, RenderGraphPersistentBuffer* persistentBuffer);

        RenderGraphBlackboard blackboard;

    private:

        friend class RenderGraphBuilder;
        friend class RenderGraphRegistry;

        bool Compile();

        void* Alloc(uint32 size)
        {
            return HE_ARENA_ALLOC(arena, size);
        }

        void* AlignedAlloc(uint32 size, uint32 alignment)
        {
            return HE_ARENA_ALIGNED_ALLOC(arena, size, alignment);
        }

        template <typename ObjectType, typename... Args>
        FORCEINLINE ObjectType* AllocObject(Args&&... args)
        {
            void* result = HE_ARENA_ALLOC(arena, sizeof(ObjectType));
            ASSERT(result);
            return new(result) ObjectType(std::forward<Args>(args)...);
            //return std::construct_at(reinterpret_cast<ObjectType*>(result), std::forward<Args>(args)...);
        }

        MemoryArena* arena;
        RenderBackendGPUProfiler* gpuProfiler;

        RenderGraphDAG dag;

        std::vector<RenderGraphPass*> passes;

        std::vector<RenderGraphTexture*> textures;
        std::vector<RenderGraphBuffer*> buffers;

        std::map<RenderBackendTextureHandle, RenderGraphTextureHandle> importedTextures;
        std::map<RenderBackendBufferHandle, RenderGraphBufferHandle> importedBuffers;

        struct RenderGraphExportedTexture
        {
            RenderGraphTexture* texture;
            RenderGraphPersistentTexture* persistentTexture;
        };
        std::vector<RenderGraphExportedTexture> exportedTextures;

        struct RenderGraphExportedBuffer
        {
            RenderGraphBuffer* buffer;
            RenderGraphPersistentBuffer* persistentBuffer;
        };
        std::vector<RenderGraphExportedBuffer> exportedBuffers;
    };

    template<typename SetupLambdaType>
    void RenderGraph::AddPass(const std::string& name, RenderGraphPassFlags flags, SetupLambdaType setup)
    {
        RenderGraphLambdaPass* pass = AllocObject<RenderGraphLambdaPass>(name, flags);
        RenderGraphBuilder builder(this, pass);
        const auto& execute = setup(builder);
        pass->SetExecuteCallback(std::move(execute));
        passes.emplace_back(pass);
        dag.RegisterNode(pass);
    }
}
