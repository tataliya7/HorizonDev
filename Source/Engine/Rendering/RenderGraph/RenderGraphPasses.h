#pragma once

#include "Rendering/RenderGraph/RenderGraphCommon.h"
#include "Rendering/RenderGraph/RenderGraphHandles.h"
#include "Rendering/RenderGraph/RenderGraphNode.h"
#include "Rendering/RenderGraph/RenderGraphRegistry.h"
#include "Rendering/RenderGraph/RenderGraphResources.h"

namespace HE
{
    enum class RenderGraphPassFlags : uint8
    {
        None           = 0,
        Copy           = (1 << 0),
        Graphics       = (1 << 1),
        Compute        = (1 << 2),
        AsyncCompute   = (1 << 3),
        RayTracing     = (1 << 4),
        NeverGetCulled = (1 << 5),
        SkipRenderPass = (1 << 6),
        Readback = Copy | NeverGetCulled,
    };
    HE_ENUM_CLASS_OPERATORS(RenderGraphPassFlags);

    class RenderGraphPass : public RenderGraphNode
    {
    public:
        RenderGraphPass(const std::string& name, RenderGraphPassFlags flags)
            : RenderGraphNode(name, RenderGraphNodeType::Pass)
            , flags(flags) {}
        virtual ~RenderGraphPass() = default;
        bool IsAsyncCompute() const
        {
            return HAS_ANY_FLAGS(flags, RenderGraphPassFlags::AsyncCompute);
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

#if HE_MGPU
        RenderBackendGpuMask gpuMask;
#endif

        struct TextureState
        {
            RenderGraphTexture* texture;
            RenderBackendResourceState state;
            RenderGraphTextureSubresourceRange subresourceRange;
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
            RenderBackendRenderTargetLoadOp loadOp;
            RenderBackendRenderTargetStoreOp storeOp;
        };
        struct DepthStencilRenderTarget
        {
            RenderGraphTextureHandle texture;
            uint32 mipLevel;
            uint32 arrayLayer;
            RenderBackendRenderTargetLoadOp depthLoadOp;
            RenderBackendRenderTargetStoreOp depthStoreOp;
            RenderBackendRenderTargetLoadOp stencilLoadOp;
            RenderBackendRenderTargetStoreOp stencilStoreOp;
        };
        ColorRenderTarget colorTargets[RenderBackendMaxNumSimultaneousColorRenderTargets];
        DepthStencilRenderTarget depthStentcilTarget;
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
            ASSERT(executeCallback);
            executeCallback(registry, commandList);
        }
        Lambda executeCallback;
    };
}