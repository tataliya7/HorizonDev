#pragma once

#include "RenderGraphCommon.h"
#include "RenderGraphHandles.h"
#include "RenderGraphNode.h"
#include "RenderGraphRegistry.h"

namespace Horizon
{
    enum class RenderGraphPassFlags : uint8
    {
        None           = 0,
        Copy           = (1 << 0),
        Compute        = (1 << 1),
        AsyncCompute   = (1 << 2),
        Graphics       = (1 << 3),
        MeshShading    = (1 << 4),
        RayTracing     = (1 << 5),
        NeverGetCulled = (1 << 6),
        SkipRenderPass = (1 << 7),
        Readback       = Copy | NeverGetCulled,
    };
    HORIZON_OVERLOAD_ENUM_CLASS_OPERATORS(RenderGraphPassFlags);

    struct RenderGraphRenderTargetBinding
    {
        RenderGraphTextureHandle texture;
        uint32 mipLevel;
        RenderBackendRenderPassLoadOperation loadOperation;
        RenderBackendRenderPassStoreOperation storeOperation;
    };

    struct RenderGraphDepthStencilBinding
    {
        RenderGraphTextureHandle texture;
        RenderBackendRenderPassLoadOperation depthLoadOperation;
        RenderBackendRenderPassStoreOperation depthStoreOperation;
        RenderBackendRenderPassLoadOperation stencilLoadOperation;
        RenderBackendRenderPassStoreOperation stencilStoreOperation;
        RenderBackendDepthStencilAccessType depthStencilAccessType;
    };

    class RenderGraphPass : public RenderGraphNode
    {
    public:
        RenderGraphPass(const std::string& name, RenderGraphPassFlags flags)
            : RenderGraphNode(name, RenderGraphNodeType::Pass)
            , flags(flags) {}
        virtual ~RenderGraphPass() = default;
        bool IsAsyncCompute() const
        {
            return EnumClassHasFlags(flags, RenderGraphPassFlags::AsyncCompute);
        }
        RenderGraphPassFlags GetFlags() const
        {
            return flags;
        }
        void Graphviz(std::stringstream& stream) const;
        virtual void Execute(RenderGraphRegistry& registry, RenderBackendCommandList& commandList) = 0;

        const RenderGraphRenderTargetBinding& GetRenderTargetBinding(uint32 slot) const;

        const RenderGraphDepthStencilBinding& GetDepthStencilBinding() const;

        void SetRenderTargetBinding(
            uint32 slot,
            RenderGraphTextureHandle handle,
            uint32 mipLevel,
            RenderBackendRenderPassLoadOperation loadOperation,
            RenderBackendRenderPassStoreOperation storeOperation);

        void SetDepthTargetBinding(
            RenderGraphTextureHandle handle,
            RenderBackendRenderPassLoadOperation depthLoadOperation,
            RenderBackendRenderPassStoreOperation depthStoreOperation,
            RenderBackendRenderPassLoadOperation stencilLoadOperation,
            RenderBackendRenderPassStoreOperation stencilStoreOperation,
            RenderBackendDepthStencilAccessType depthStencilAccessType);

    protected:
        friend class RenderGraph;
        friend class RenderGraphBuilder;

        RenderGraphPassFlags flags;

        struct TextureState
        {
            RenderGraphTexture* texture;
            RenderBackendResourceState initialState;
            RenderBackendResourceState finalState;
        };

        struct BufferState
        {
            RenderGraphBuffer* buffer;
            RenderBackendResourceState state;
        };

        std::vector<TextureState> textureStates;
        std::vector<BufferState> bufferStates;

        std::vector<RenderBackendBarrier> barriers;

        bool allowUAVWrites = false;
        Rect renderArea;

        RenderGraphRenderTargetBinding renderTargetBindings[RenderBackendMaxRenderTargetCount];
        RenderGraphDepthStencilBinding depthStencilBinding;
    };

    class RenderGraphLambdaPass : public RenderGraphPass
    {
    public:
        // @todo Maybe it would be faster not to use std::function.
        using Lambda = std::function<void(RenderGraphRegistry&, RenderBackendCommandList&)>;
        RenderGraphLambdaPass(const std::string& name, RenderGraphPassFlags flags) : RenderGraphPass(name, flags) {}
        ~RenderGraphLambdaPass() = default;
    private:
        friend class RenderGraph;
        void SetExecuteCallback(Lambda&& execute) { executeCallback = std::move(execute); }
        void Execute(RenderGraphRegistry& registry, RenderBackendCommandList& commandList) override
        {
            assert(executeCallback);
            executeCallback(registry, commandList);
        }
        Lambda executeCallback;
    };
}