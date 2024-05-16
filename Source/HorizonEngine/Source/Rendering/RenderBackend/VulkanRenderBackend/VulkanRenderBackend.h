#pragma once

namespace Horizon
{
    class RenderBackend;

    enum VulkanRenderBackendCreateFlags
    {
        VULKAN_RENDER_BACKEND_CREATE_FLAGS_NONE = 0,
        VULKAN_RENDER_BACKEND_CREATE_FLAGS_VALIDATION_LAYERS = (1 << 1),
        VULKAN_RENDER_BACKEND_CREATE_FLAGS_SURFACE = (1 << 2),
        VULKAN_RENDER_BACKEND_CREATE_FLAGS_RAY_TRACING = (1 << 3),
    };

    typedef RenderBackend* (__stdcall* PFN_VulkanRenderBackendCreateBackend)(int flags);
    typedef void(__stdcall* PFN_VulkanRenderBackendDestroyBackend)(RenderBackend* backend);

    RenderBackend* RenderBackendCreateVulkan(int flags);
    void RenderBackendDestroyVulkan(RenderBackend* backend);
}
