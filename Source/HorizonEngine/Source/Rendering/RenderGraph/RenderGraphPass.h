#pragma once

#include "RenderGraphCommon.h"
#include "RenderGraphHandles.h"
#include "RenderGraphNode.h"
#include "RenderGraphResourceRegistry.h"

namespace Horizon
{
    enum class RenderGraphPassType
    {
        Copy,
        Compute,
        AsyncCompute,
        Graphics,
        MeshShading,
        RayTracing
    };

    enum class RenderGraphPassFlags : uint32
    {
        None           = 0,
        Copy           = (1 << 0),
        Compute        = (1 << 1),
        AsyncCompute   = (1 << 2),
        Graphics       = (1 << 3),
        MeshShading    = (1 << 4),
        RayTracing     = (1 << 5),
        NoCulling = (1 << 6),
        SkipRenderPass = (1 << 7),
        DebugLabelRegion_DEPRECATED = (1 << 8),
        Readback       = Copy | NoCulling,
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

    struct RenderGraphShaderConstantBinding
    {
        enum class Type : int8
        {
            Unknown               = 0,
            UntrackedResource     = 1,
            BufferSRV             = 2,
            BufferUAV             = 3,
            TextureSRV            = 4,
            TextureUAV            = 5,
            AccelerationStructure = 6,
            ScalarInt             = 7,
            ScalarUnit            = 8,
            ScalarFloat           = 9,
            Count                 = 8
        };

        Type type;

        union
        {
            int                         descriptorIndex;
            RenderGraphBufferHandle     bufferHandle;
            RenderGraphTextureHandle    textureHandle;
            int                         scalarTypeInt;
            unsigned int                scalarTypeUint;
            float                       scalarTypeFloat;
        };

        RenderGraphShaderConstantBinding() : type(Type::Unknown), descriptorIndex(0) {}
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
        virtual void Execute(RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry) = 0;

        const RenderGraphRenderTargetBinding& GetRenderTargetBinding(uint32 slot) const;

        const RenderGraphDepthStencilBinding& GetDepthStencilBinding() const;

        void SetRenderTargetBinding(
            uint32 slot,
            RenderGraphTextureHandle handle,
            uint32 mipLevel,
            RenderBackendRenderPassLoadOperation loadOperation,
            RenderBackendRenderPassStoreOperation storeOperation);

        void SetDepthStencilBinding(
            RenderGraphTextureHandle handle,
            RenderBackendRenderPassLoadOperation depthLoadOperation,
            RenderBackendRenderPassStoreOperation depthStoreOperation,
            RenderBackendRenderPassLoadOperation stencilLoadOperation,
            RenderBackendRenderPassStoreOperation stencilStoreOperation,
            RenderBackendDepthStencilAccessType depthStencilAccessType);

    protected:
        friend class RenderGraph;
        friend class RenderGraphBuilder;
        friend class RenderGraphResourceRegistry;

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
        RenderGraphShaderConstantBinding shaderConstantsBindings[RenderBackendPushConstantsSlotCount];

        void BindUntrackedResource(uint32 slot, int descriptorIndex)
        {
            shaderConstantsBindings[slot].type = RenderGraphShaderConstantBinding::Type::UntrackedResource;
            shaderConstantsBindings[slot].descriptorIndex = descriptorIndex;
        }

        void BindBufferSRV(uint32 slot, RenderGraphBufferHandle bufferHandle)
        {
            shaderConstantsBindings[slot].type = RenderGraphShaderConstantBinding::Type::BufferSRV;
            shaderConstantsBindings[slot].bufferHandle = bufferHandle;
        }

        void BindBufferUAV(uint32 slot, RenderGraphBufferHandle bufferHandle)
        {
            shaderConstantsBindings[slot].type = RenderGraphShaderConstantBinding::Type::BufferUAV;
            shaderConstantsBindings[slot].bufferHandle = bufferHandle;
        }

        void BindTextureSRV(uint32 slot, RenderGraphTextureHandle textureHandle)
        {
            shaderConstantsBindings[slot].type = RenderGraphShaderConstantBinding::Type::TextureSRV;
            shaderConstantsBindings[slot].textureHandle = textureHandle;
        }

        void BindTextureUAV(uint32 slot, RenderGraphTextureHandle textureHandle)
        {
            shaderConstantsBindings[slot].type = RenderGraphShaderConstantBinding::Type::TextureUAV;
            shaderConstantsBindings[slot].textureHandle = textureHandle;
        }

        void BindScalar(uint32 slot, int32 value)
        {
            shaderConstantsBindings[slot].type = RenderGraphShaderConstantBinding::Type::ScalarInt;
            shaderConstantsBindings[slot].scalarTypeInt = value;
        }

        void BindScalar(uint32 slot, uint32 value)
        {
            shaderConstantsBindings[slot].type = RenderGraphShaderConstantBinding::Type::ScalarUnit;
            shaderConstantsBindings[slot].scalarTypeUint = value;
        }

        void BindScalar(uint32 slot, float value)
        {
            shaderConstantsBindings[slot].type = RenderGraphShaderConstantBinding::Type::ScalarFloat;
            shaderConstantsBindings[slot].scalarTypeFloat = value;
        }
    };

    class RenderGraphLambdaPass : public RenderGraphPass
    {
    public:
        // @todo Maybe it would be faster not to use std::function.
        using Lambda = std::function<void(RenderBackendCommandList&, const RenderGraphResourceRegistry&)>;
        RenderGraphLambdaPass(const std::string& name, RenderGraphPassFlags flags) : RenderGraphPass(name, flags) {}
        ~RenderGraphLambdaPass() = default;
    private:
        friend class RenderGraph;
        void SetExecuteCallback(Lambda&& execute) { executeCallback = std::move(execute); }
        void Execute(RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry) override
        {
            assert(executeCallback);
            executeCallback(commandList, resourceRegistry);
        }
        Lambda executeCallback;
    };
}