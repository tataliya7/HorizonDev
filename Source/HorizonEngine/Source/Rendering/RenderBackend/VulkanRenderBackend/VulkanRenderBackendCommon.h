#pragma once

#include "Rendering/RenderBackend/RenderBackendInterface.h"
#include "Rendering/RenderBackend/RenderBackendCommands.h"
#include "Rendering/RenderBackend/RenderBackendCommandList.h"

#if _WIN64
#define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <vma/vk_mem_alloc.h>