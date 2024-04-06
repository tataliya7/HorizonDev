#pragma once

#define RENDER_BACKEND_NUM_QUEUE_FAMILIES (4)

#define RENDER_BACKEND_REMAINING_ARRAY_LAYERS (~0U)
#define RENDER_BACKEND_REMAINING_MIP_LEVELS (~0U)
#define RENDER_BACKEND_WHOLE_SIZE (~0ULL)

#define RENDER_BACKEND_VERSION_MAJOR (1)
#define RENDER_BACKEND_VERSION_MINOR (0)
#define RENDER_BACKEND_VERSION_PATCH (0)

enum
{
    RenderBackendMaxNumGPUs = 16,
    RenderBackendMaxNumDevices = 64,
    RenderBackendMaxNumSwapChainBuffers = 16,
    RenderBackendMaxNumSimultaneousColorRenderTargets = 8,
    RenderBackendMaxNumViewports = 16,
    RenderBackendMaxNumShaderStages = 10,
    RenderBackendMaxNumTextureMipLevels = 16,
    RenderBackendMaxNumTimingQueryRegions = 128,
};