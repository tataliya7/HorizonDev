#pragma once

#include "RenderBackend/RenderBackendInterface.h"
#include "RenderBackend/RenderBackendCommands.h"
#include "RenderBackend/RenderBackendCommandList.h"

#ifdef HE_PLATFORM_WINDOWS
#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>
