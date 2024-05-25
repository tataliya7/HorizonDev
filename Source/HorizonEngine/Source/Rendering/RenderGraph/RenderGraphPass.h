#pragma once

#include "RenderGraphCommon.h"
#include "RenderGraphHandles.h"
#include "RenderGraphNode.h"
#include "RenderGraphRegistry.h"
#include "RenderGraphResources.h"

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
        Readback = Copy | NeverGetCulled,
    };
    HORIZON_ENUM_CLASS_OPERATORS(RenderGraphPassFlags);

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
    protected:
        friend class RenderGraph;
        friend class RenderGraphBuilder;

        RenderGraphPassFlags flags;

        struct TextureState
        {
            RenderGraphTexture* texture;
            RenderBackendResourceState state;
        };

        struct BufferState
        {
            RenderGraphBuffer* buffer;
            RenderBackendResourceState state;
        };

        std::vector<TextureState> textureStates;
        std::vector<BufferState> bufferStates;

        std::vector<RenderBackendBarrier> barriers;

        struct ColorRenderTarget
        {
            RenderGraphTextureHandle texture;
            uint32 mipLevel;
            uint32 arrayLayer;
            RenderBackendRenderPassBeginningAccessType loadOp;
            RenderBackendRenderPassEndingAccessType storeOp;
        };
        struct DepthStencilTarget
        {
            RenderGraphTextureHandle texture;
            uint32 mipLevel;
            uint32 arrayLayer;
            RenderBackendRenderPassBeginningAccessType depthLoadOp;
            RenderBackendRenderPassEndingAccessType depthStoreOp;
            RenderBackendRenderPassBeginningAccessType stencilLoadOp;
            RenderBackendRenderPassEndingAccessType stencilStoreOp;
        };
        ColorRenderTarget renderTargets[RenderBackendMaxRenderTargetCount];
        DepthStencilTarget depthStentcil;
    };

    class RenderGraphLambdaPass : public RenderGraphPass
    {
    public:
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