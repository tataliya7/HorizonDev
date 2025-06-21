#include "Rendering/RenderGraph/RenderGraphPass.h"

namespace Horizon
{
    void RenderGraphPass::Graphviz(std::stringstream& stream) const
    {
        stream << "\t\t" << name << " [color=orange];\n";

        for (RenderGraphNode* input : inputs)
        {
            stream << "\t\t" << input->GetName() << " [color=green];\n";
            stream << "\t\t" << input->GetName() << " -> " << name << "\n";
        }

        for (RenderGraphNode* output : outputs)
        {
            stream << "\t\t" << output->GetName() << " [color=green];\n";
            stream << "\t\t" << name << " -> " << output->GetName() << "\n";
        }
    }

    const RenderGraphRenderTargetBinding& RenderGraphPass::GetRenderTargetBinding(uint32 slot) const
    {
        return renderTargetBindings[slot];
    }

    const RenderGraphDepthStencilBinding& RenderGraphPass::GetDepthStencilBinding() const
    {
        return depthStencilBinding;
    }

    void RenderGraphPass::SetRenderTargetBinding(
        uint32 slot,
        RenderGraphTextureHandle handle,
        uint32 mipLevel,
        RenderBackendRenderPassLoadOperation loadOperation,
        RenderBackendRenderPassStoreOperation storeOperation)
    {
        assert(!renderTargetBindings[slot].texture);
        assert((slot == 0) || renderTargetBindings[slot - 1].texture);

        renderTargetBindings[slot] =
        {
            .texture = handle,
            .mipLevel = mipLevel,
            .loadOperation = loadOperation,
            .storeOperation = storeOperation,
        };
    }

    void RenderGraphPass::SetDepthStencilBinding(
        RenderGraphTextureHandle handle,
        RenderBackendRenderPassLoadOperation depthLoadOperation,
        RenderBackendRenderPassStoreOperation depthStoreOperation,
        RenderBackendRenderPassLoadOperation stencilLoadOperation,
        RenderBackendRenderPassStoreOperation stencilStoreOperation,
        RenderBackendDepthStencilAccessType depthStencilAccessType)
    {
        assert(!depthStencilBinding.texture);

        depthStencilBinding =
        {
            .texture = handle,
            .depthLoadOperation = depthLoadOperation,
            .depthStoreOperation = depthStoreOperation,
            .stencilLoadOperation = stencilLoadOperation,
            .stencilStoreOperation = stencilStoreOperation,
            .depthStencilAccessType = depthStencilAccessType,
        };
    }
}