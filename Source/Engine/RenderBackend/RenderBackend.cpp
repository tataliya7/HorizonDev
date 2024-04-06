#include "RenderBackendInterface.h"
#include "RenderBackendTypes.h"

namespace HE
{
    RenderBackend* GRenderBackend = nullptr;

    const RenderBackendBufferSubresourceRange RenderBackendBufferSubresourceRange::Whole = RenderBackendBufferSubresourceRange(0, RENDER_BACKEND_WHOLE_SIZE);
    const RenderBackendTextureSubresourceRange RenderBackendTextureSubresourceRange::All = RenderBackendTextureSubresourceRange(0, RENDER_BACKEND_REMAINING_MIP_LEVELS, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS);

    const RenderBackendTextureClearValue RenderBackendTextureClearValue::None      = RenderBackendTextureClearValue();
    const RenderBackendTextureClearValue RenderBackendTextureClearValue::Black     = RenderBackendTextureClearValue(0.0f, 0.0f, 0.0f, 1.0f);
    const RenderBackendTextureClearValue RenderBackendTextureClearValue::White     = RenderBackendTextureClearValue(1.0f, 1.0f, 1.0f, 1.0f);
    const RenderBackendTextureClearValue RenderBackendTextureClearValue::DepthOne  = RenderBackendTextureClearValue(1.0f, 0);
    const RenderBackendTextureClearValue RenderBackendTextureClearValue::DepthZero = RenderBackendTextureClearValue(0.0f, 0);
}