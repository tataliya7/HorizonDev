#include "RenderBackendTypes.h"
#include "RenderBackendInterface.h"
#include "VulkanRenderBackend/VulkanRenderBackend.h"
#include "Direct3D12RenderBackend/Direct3D12RenderBackend.h"

namespace Horizon
{
    const uint64 RenderBackendBufferSubresourceRange::WholeSize = ~0ULL;
    const RenderBackendBufferSubresourceRange RenderBackendBufferSubresourceRange::Whole = RenderBackendBufferSubresourceRange(0, RenderBackendBufferSubresourceRange::WholeSize);

    const uint32 RenderBackendTextureSubresourceRange::RemainingMipLevels = ~0U;
    const uint32 RenderBackendTextureSubresourceRange::RemainingArrayLayers = ~0U;
    const RenderBackendTextureSubresourceRange RenderBackendTextureSubresourceRange::All = RenderBackendTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers);

    const RenderBackendTextureClearValue RenderBackendTextureClearValue::None      = RenderBackendTextureClearValue();
    const RenderBackendTextureClearValue RenderBackendTextureClearValue::Black     = RenderBackendTextureClearValue(0.0f, 0.0f, 0.0f, 1.0f);
    const RenderBackendTextureClearValue RenderBackendTextureClearValue::White     = RenderBackendTextureClearValue(1.0f, 1.0f, 1.0f, 1.0f);
    const RenderBackendTextureClearValue RenderBackendTextureClearValue::DepthOne  = RenderBackendTextureClearValue(1.0f, 0);
    const RenderBackendTextureClearValue RenderBackendTextureClearValue::DepthZero = RenderBackendTextureClearValue(0.0f, 0);

    const RenderBackendColorBlendAttachmentState RenderBackendColorBlendAttachmentState::Additive =
    {
        .blendEnable = true,
        .srcColorBlendFactor = RenderBackendBlendFactor::One,
        .dstColorBlendFactor = RenderBackendBlendFactor::One,
        .colorBlendOp = RenderBackendBlendOp::Add,
        .srcAlphaBlendFactor = RenderBackendBlendFactor::Zero,
        .dstAlphaBlendFactor = RenderBackendBlendFactor::One,
        .alphaBlendOp = RenderBackendBlendOp::Add,
        .writeMask = RenderBackendColorComponentFlags::RGBA,
    };

    const RenderBackendColorBlendAttachmentState RenderBackendColorBlendAttachmentState::AdditiveRGB =
    {
        .blendEnable = true,
        .srcColorBlendFactor = RenderBackendBlendFactor::One,
        .dstColorBlendFactor = RenderBackendBlendFactor::One,
        .colorBlendOp = RenderBackendBlendOp::Add,
        .writeMask = RenderBackendColorComponentFlags::RGB,
    };

    RenderBackend* RenderBackendCreateInstance(const RenderBackendDesc* desc)
    {
        RenderBackend* backend = nullptr;
        if (desc->type == RenderBackendType::Vulkan)
        {
            backend = RenderBackendCreateVulkan(desc);
        }
        else if (desc->type == RenderBackendType::Direct3D12)
        {
            backend = RenderBackendCreateDirect3D12(desc);
        }
        else
        {
            LogError(GLogger, std::format("Unknown RenderBackendType!"));
        }

        return backend;
    }

    void RenderBackendDestroyInstance(RenderBackend* backend)
    {

    }
}