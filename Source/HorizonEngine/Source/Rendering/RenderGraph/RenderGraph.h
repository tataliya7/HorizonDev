#pragma once

#include "RenderGraphCommon.h"
#include "RenderGraphHandles.h"
#include "RenderGraphNode.h"
#include "RenderGraphPass.h"
#include "RenderGraphResources.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphBlackboard.h"

namespace Horizon
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

    //class RenderGraphScopedEvent
    //{
    //public:
    //    RenderGraphScopedEvent(RenderGraph& renderGraph, const std::string& name)
    //    {

    //    }

    //    ~RenderGraphScopedEvent()
    //    {

    //    }
    //private:
    //    RenderGraph& renderGraph;
    //};

    enum class RenderGraphSourceDataLifetimeHint
    {
        OnlyValidNow,
        ValidUntilExecution,
    };

    class RenderGraph
    {
    public:

        RenderGraph(MemoryArena* arena, RenderGraphResourcePool* pool, RenderBackendGPUProfiler* gpuProfiler);
        RenderGraph(const RenderGraph& other) = delete;
        virtual ~RenderGraph();

        RenderGraphBlackboard blackboard;

        RenderBackend* GetRenderBackend() const
        {
            return renderBackend;
        }

        /**
         * Adds a pass to the render graph with a lamda functions.
         *
         * TBD
         *
         * @param[in] name The pass name used for debugging/profiling.
         * @param[in] flags Describe the workload to the pass.
         * @param[in] setup The lambda function used to setup the resource dependencies, which is executed in the AddPass() function, must return a lambda function used to execute the pass.
         */
        template <typename SetupLambdaType>
        void AddPass(const std::string& name, RenderGraphPassFlags flags, SetupLambdaType setup);

        void Execute(RenderBackendCommandList& commandList);

        void Clear();

        /**
         * @brief Create a string using the Graphviz format.
         * @note Compile() should be called before calling this function.
         * @return std::string in the Graphviz format.
         */
        std::string Graphviz() const;

        /**
         * Create a render graph tracked texture.
         */
        RenderGraphTextureHandle CreateTexture(const RenderGraphTextureDescription& desc, const char* name);

        /**
         * Create a render graph tracked buffer.
         */
        RenderGraphBufferHandle CreateBuffer(const RenderGraphBufferDescription& desc, const char* name);

        //RenderGraphTextureSRVHandle CreateTextureSRV(RenderGraphTextureHandle texture, const RenderGraphTextureSRVDesc& desc);
        //RenderGraphTextureUAVHandle CreateTextureUAV(RenderGraphTextureHandle texture, uint32 mipLevel);

        void UploadBufferDeferred(RenderGraphBufferHandle buffer, const void* data, uint64 size, RenderGraphSourceDataLifetimeHint hint);

        /**
         * Find a render graph texture associated with the external texture, or returns null if none is found.
         */
        RenderGraphTextureHandle FindExternalTexture(RenderBackendTextureHandle renderBackendTexture);

        /**
          * Find a render graph buffer associated with the external buffer, or returns null if none is found.
          */
        RenderGraphBufferHandle FindExternalBuffer(RenderBackendBufferHandle renderBackendBuffer);

        /**
         * Import an external texture to be accessed by the render graph.
         * @note The external texture's state must be RenderBackendResourceState::ShaderResource.
         */
        RenderGraphTextureHandle ImportExternalTexture(RenderGraphPersistentTexture* externalTexture, char const* name);

        /**
         * Import an external buffer to be accessed by the render graph.
         * @note The external buffer's state must be RenderBackendResourceState::ShaderResource.
         */
        RenderGraphBufferHandle ImportExternalBuffer(RenderGraphPersistentBuffer* externalBuffer, char const* name);

        void ExportTextureDeferred(RenderGraphTextureHandle handle, RenderGraphPersistentTexture** persistentTexture);

        void ExportBufferDeferred(RenderGraphBufferHandle handle, RenderGraphPersistentBuffer** persistentBuffer);

        const RenderGraphTextureDescription& GetTextureDescription(RenderGraphTextureHandle handle) const;

        const RenderGraphBufferDescription& GetBufferDesc(RenderGraphBufferHandle handle) const;

        void BeginTimingQuery();

        void EndTimingQuery();

    private:

        friend class RenderGraphBuilder;
        friend class RenderGraphResourceRegistry;

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
            assert(result);
            return new(result) ObjectType(std::forward<Args>(args)...);
            //return std::construct_at(reinterpret_cast<ObjectType*>(result), std::forward<Args>(args)...);
        }

        void ExecuteBufferUploadJobs(RenderBackendCommandList& commandList);

        MemoryArena* arena;
        RenderBackend* renderBackend;
        RenderGraphResourcePool* resourcePool;
        RenderBackendGPUProfiler* gpuProfiler;

        RenderGraphDAG dag;

        uint32 asyncComputePassCount = 0;
        std::vector<RenderGraphPass*> passes;

        std::vector<RenderGraphTexture*> textures;
        std::vector<RenderGraphBuffer*> buffers;

        std::map<uint64, RenderGraphTextureHandle> importedTextures;
        std::map<uint64, RenderGraphBufferHandle> importedBuffers;

        struct RenderGraphExportedTexture
        {
            RenderGraphTexture* source;
            RenderGraphPersistentTexture** target;
        };
        std::vector<RenderGraphExportedTexture> exportedTextures;

        struct RenderGraphExportedBuffer
        {
            RenderGraphBuffer* source;
            RenderGraphPersistentBuffer** target;
        };
        std::vector<RenderGraphExportedBuffer> exportedBuffers;

        struct RenderGraphBufferUploadJobDescription
        {
            RenderGraphBuffer* buffer;
            const void* data;
            uint64 size;
        };
        std::vector<RenderGraphBufferUploadJobDescription> bufferUploadJobs;
    };

    template <typename SetupLambdaType>
    void RenderGraph::AddPass(const std::string& name, RenderGraphPassFlags flags, SetupLambdaType setup)
    {
        RenderGraphLambdaPass* pass = AllocObject<RenderGraphLambdaPass>(name, flags);
        RenderGraphBuilder builder(this, pass);
        const auto& execute = setup(builder);
        pass->SetExecuteCallback(std::move(execute));
        passes.emplace_back(pass);
        dag.RegisterNode(pass);
    }

    class RenderGraphDebugLabelRegion
    {
    public:
        RenderGraphDebugLabelRegion(RenderGraph& renderGraph, const char* name);
        ~RenderGraphDebugLabelRegion();
    private:
        RenderGraph* renderGraph;
        const char* name;
    };
}