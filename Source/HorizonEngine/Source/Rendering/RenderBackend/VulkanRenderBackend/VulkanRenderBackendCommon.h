#pragma once

#include "Rendering/RenderBackend/RenderBackendInterface.h"
#include "Rendering/RenderBackend/RenderBackendCommands.h"
#include "Rendering/RenderBackend/RenderBackendCommandList.h"

#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
