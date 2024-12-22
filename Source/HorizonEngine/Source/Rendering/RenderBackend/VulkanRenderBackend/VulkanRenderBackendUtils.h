#pragma once

#include "VulkanRenderBackendCommon.h"

#define VK_CHECK(VkFunction) { const VkResult result = VkFunction; if (result != VK_SUCCESS) { VerifyVkResult(result, #VkFunction, __FILE__, __LINE__); } }
#define VK_CHECK_RESULT(result) { if (result != VK_SUCCESS) { VerifyVkResult(result, __FUNCTION__, __FILE__, __LINE__); } }

namespace Horizon
{
    static inline void VerifyVkResult(VkResult result, const char* vkFuntion, const char* filename, uint32 line)
    {
        if (result > 0)
        {
            LogError(GLogger, std::format("Unexpected result. Code: {}. Function : {}. File : {}. Line : {}.", (int)result, vkFuntion, filename, line));
        }
        else
        {
            LogError(GLogger, std::format("Vulkan function returns a runtime error. Code: {}. Function: {}. File: {}. Line: {}.", (int)result, vkFuntion, filename, line));
            std::unreachable();
        }
    }

    namespace VulkanHelper
    {
        void CreateTemporaryCommandBuffer(VkDevice device, uint32 queueFamilyIndex, VkCommandPool& tempCmdPool, VkCommandBuffer& tempCmdBuffer)
        {
            VkCommandPoolCreateInfo commandPoolInfo = {};
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, VULKAN_ALLOCATION_CALLBACKS, &tempCmdPool));
            VkCommandBufferAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocateInfo.commandPool = tempCmdPool;
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandBufferCount = 1;
            VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &tempCmdBuffer));
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VK_CHECK(vkBeginCommandBuffer(tempCmdBuffer, &beginInfo));
        }

        void FlushTemporaryCommandBuffer(VkDevice device, VkQueue queue, VkCommandPool tempCmdPool, VkCommandBuffer tempCmdBuffer)
        {
            VK_CHECK(vkEndCommandBuffer(tempCmdBuffer));
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &tempCmdBuffer;
            VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
            VK_CHECK(vkDeviceWaitIdle(device));
            vkDestroyCommandPool(device, tempCmdPool, nullptr);
        }
    }

    static inline VkBool32 ConvertToVkBool(bool b)
    {
        return b ? VK_TRUE : VK_FALSE;
    }

    static inline bool IsDepthOnlyFormat(VkFormat format)
    {
        return (format == VK_FORMAT_D32_SFLOAT) || (format == VK_FORMAT_D16_UNORM);
    }

    static inline bool IsStencilFormat(VkFormat format)
    {
        return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT) || (format == VK_FORMAT_D16_UNORM_S8_UINT);
    }

    static inline bool IsDepthStencilFormat(VkFormat format)
    {
        return (format == VK_FORMAT_D32_SFLOAT) || (format == VK_FORMAT_D16_UNORM) || (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT) || (format == VK_FORMAT_D16_UNORM_S8_UINT);
    }

    static inline uint32 AlignUp(uint32 size, uint32 alignment)
    {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    const VkFormat GVkFormatTable[] =
    {
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8_SNORM,
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R16_SNORM,
        VK_FORMAT_R8G8_UNORM,
        VK_FORMAT_R8G8_SNORM,
        VK_FORMAT_R16G16_UNORM,
        VK_FORMAT_R16G16_SNORM,
        VK_FORMAT_R16G16B16_UNORM,
        VK_FORMAT_R16G16B16_SNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SNORM,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R16_SFLOAT,
        VK_FORMAT_R16G16_SFLOAT,
        VK_FORMAT_R16G16B16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R8_SINT,
        VK_FORMAT_R8_UINT,
        VK_FORMAT_R16_SINT,
        VK_FORMAT_R16_UINT,
        VK_FORMAT_R32_SINT,
        VK_FORMAT_R32_UINT,
        VK_FORMAT_R8G8_SINT,
        VK_FORMAT_R8G8_UINT,
        VK_FORMAT_R16G16_SINT,
        VK_FORMAT_R16G16_UINT,
        VK_FORMAT_R32G32_SINT,
        VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R16G16B16_SINT,
        VK_FORMAT_R16G16B16_UINT,
        VK_FORMAT_R32G32B32_SINT,
        VK_FORMAT_R32G32B32_UINT,
        VK_FORMAT_R8G8B8A8_SINT,
        VK_FORMAT_R8G8B8A8_UINT,
        VK_FORMAT_R16G16B16A16_SINT,
        VK_FORMAT_R16G16B16A16_UINT,
        VK_FORMAT_R32G32B32A32_SINT,
        VK_FORMAT_R32G32B32A32_UINT,

        VK_FORMAT_B10G11R11_UFLOAT_PACK32,

        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_B8G8R8A8_SRGB,

        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D24_UNORM_S8_UINT,

        VK_FORMAT_A2B10G10R10_UNORM_PACK32,

        VK_FORMAT_BC1_RGB_UNORM_BLOCK,
        VK_FORMAT_BC1_RGB_SRGB_BLOCK,
        //VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
        //VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
        VK_FORMAT_BC2_UNORM_BLOCK,
        VK_FORMAT_BC2_SRGB_BLOCK,
        VK_FORMAT_BC3_UNORM_BLOCK,
        VK_FORMAT_BC3_SRGB_BLOCK,
        VK_FORMAT_BC4_UNORM_BLOCK,
        VK_FORMAT_BC4_SNORM_BLOCK,
        VK_FORMAT_BC5_UNORM_BLOCK,
        VK_FORMAT_BC5_SNORM_BLOCK,
        VK_FORMAT_BC6H_UFLOAT_BLOCK,
        VK_FORMAT_BC6H_SFLOAT_BLOCK,
        VK_FORMAT_BC7_UNORM_BLOCK,
        VK_FORMAT_BC7_SRGB_BLOCK,
    };
    static_assert(ArraySize(GVkFormatTable) == (uint32)RenderBackendTextureFormat::Count);

    static inline VkClearValue ConvertToVkClearValue(const RenderBackendTextureClearValue& clearValue)
    {
        VkClearValue value = {};
        switch (clearValue.clearOp)
        {
        case RenderBackendTextureClearValue::ClearOp::Color:
            value.color.float32[0] = clearValue.colorValue.float32[0];
            value.color.float32[1] = clearValue.colorValue.float32[1];
            value.color.float32[2] = clearValue.colorValue.float32[2];
            value.color.float32[3] = clearValue.colorValue.float32[3];
            break;
        case RenderBackendTextureClearValue::ClearOp::DepthStencil:
            value.depthStencil.depth = clearValue.depthStencilValue.depth;
            value.depthStencil.stencil = clearValue.depthStencilValue.stencil;
            break;
        }
        return value;
    }

    static inline VkIndexType ConverToVkIndexType(RenderBackendIndexType type)
    {
        switch (type)
        {
        case RenderBackendIndexType::UINT32: return VK_INDEX_TYPE_UINT32;
        case RenderBackendIndexType::UINT16: return VK_INDEX_TYPE_UINT16;
        case RenderBackendIndexType::UINT8: return VK_INDEX_TYPE_UINT8_KHR;
        default: std::unreachable(); return VK_INDEX_TYPE_MAX_ENUM;
        }
    }

    static inline VkPolygonMode ConvertToVkPolygonMode(RenderBackendRasterizationFillMode mode)
    {
        switch (mode)
        {
        case RenderBackendRasterizationFillMode::Wireframe: return VK_POLYGON_MODE_LINE;
        case RenderBackendRasterizationFillMode::Solid: return VK_POLYGON_MODE_FILL;
        default: std::unreachable(); return VK_POLYGON_MODE_MAX_ENUM;
        }
    }

    static inline VkCullModeFlags ConvertToVkCullModeFlags(RenderBackendRasterizationCullMode mode)
    {
        switch (mode)
        {
        case RenderBackendRasterizationCullMode::None: return VK_CULL_MODE_NONE;
        case RenderBackendRasterizationCullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case RenderBackendRasterizationCullMode::Back: return VK_CULL_MODE_BACK_BIT;
        default: std::unreachable(); return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
        }
    }

    static inline VkBlendFactor ConvertToVkBlendFactor(RenderBackendBlendFactor blendFactor)
    {
        switch (blendFactor)
        {
        case RenderBackendBlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
        case RenderBackendBlendFactor::One: return VK_BLEND_FACTOR_ONE;
        case RenderBackendBlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
        case RenderBackendBlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case RenderBackendBlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
        case RenderBackendBlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case RenderBackendBlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
        case RenderBackendBlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case RenderBackendBlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
        case RenderBackendBlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case RenderBackendBlendFactor::ConstantBlendFactor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case RenderBackendBlendFactor::OneMinusConstantBlendFactor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case RenderBackendBlendFactor::SrcAlphaSaturate: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case RenderBackendBlendFactor::Src1Color: return VK_BLEND_FACTOR_SRC1_COLOR;
        case RenderBackendBlendFactor::OneMinusSrc1Color: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case RenderBackendBlendFactor::Src1Alpha: return VK_BLEND_FACTOR_SRC1_ALPHA;
        case RenderBackendBlendFactor::OneMinusSrc1Alpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default: std::unreachable(); return VK_BLEND_FACTOR_MAX_ENUM;
        }
    }

    static inline VkBlendOp ConvertToVkBlendOp(RenderBackendBlendOp blendOp)
    {
        switch (blendOp)
        {
        case RenderBackendBlendOp::Add: return VK_BLEND_OP_ADD;
        case RenderBackendBlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
        case RenderBackendBlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case RenderBackendBlendOp::Min: return VK_BLEND_OP_MIN;
        case RenderBackendBlendOp::Max: return VK_BLEND_OP_MAX;
        default: std::unreachable(); return VK_BLEND_OP_MAX_ENUM;
        }
    }

    static inline VkStencilOp ConvertToVkStencilOp(RenderBackendStencilOp stencilOp)
    {
        switch (stencilOp)
        {
        case RenderBackendStencilOp::Keep: return VK_STENCIL_OP_KEEP;
        case RenderBackendStencilOp::Zero: return VK_STENCIL_OP_ZERO;
        case RenderBackendStencilOp::Replace: return VK_STENCIL_OP_REPLACE;
        case RenderBackendStencilOp::IncreaseAndClamp: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case RenderBackendStencilOp::DecreaseAndClamp: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case RenderBackendStencilOp::Invert: return VK_STENCIL_OP_INVERT;
        case RenderBackendStencilOp::IncreaseAndWrap: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case RenderBackendStencilOp::DecreaseAndWrap: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default: std::unreachable(); return VK_STENCIL_OP_MAX_ENUM;
        }
    }

    static inline VkFormat ConvertToVkFormat(RenderBackendTextureFormat format)
    {
        return GVkFormatTable[(uint32)format];
    }

    static inline VkImageType ConvertToVkImageType(RenderBackendTextureType type)
    {
        switch (type)
        {
        case RenderBackendTextureType::Texture1D:
            return VK_IMAGE_TYPE_1D;
        case RenderBackendTextureType::Texture2D:
        case RenderBackendTextureType::TextureCube:
            return VK_IMAGE_TYPE_2D;
        case RenderBackendTextureType::Texture3D:
            return VK_IMAGE_TYPE_3D;
        default:
            std::unreachable();
            return VK_IMAGE_TYPE_MAX_ENUM;
        }
    }

    static inline VkImageViewType ConvertToVkImageViewType(RenderBackendTextureType type, bool isArray)
    {
        switch (type)
        {
        case RenderBackendTextureType::Texture1D:
            return isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        case RenderBackendTextureType::Texture2D:
            return isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        case RenderBackendTextureType::Texture3D:
            assert(isArray == false);
            return VK_IMAGE_VIEW_TYPE_3D;
        case RenderBackendTextureType::TextureCube:
            return isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        default:
            std::unreachable();
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }

    static inline VkPrimitiveTopology ConvertToVkPrimitiveTopology(RenderBackendPrimitiveTopology topology)
    {
        switch (topology)
        {
        case RenderBackendPrimitiveTopology::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case RenderBackendPrimitiveTopology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case RenderBackendPrimitiveTopology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case RenderBackendPrimitiveTopology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case RenderBackendPrimitiveTopology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case RenderBackendPrimitiveTopology::TriangleFan:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        default:
            std::unreachable();
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }

    static inline VkShaderStageFlagBits ConvertToVkShaderStageFlagBits(RenderBackendShaderStage stage)
    {
        switch (stage)
        {
        case RenderBackendShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case RenderBackendShaderStage::Pixel:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case RenderBackendShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        case RenderBackendShaderStage::RayGen:
            return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case RenderBackendShaderStage::AnyHit:
            return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case RenderBackendShaderStage::ClosestHit:
            return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case RenderBackendShaderStage::Miss:
            return VK_SHADER_STAGE_MISS_BIT_KHR;
        case RenderBackendShaderStage::Intersection:
            return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        case RenderBackendShaderStage::Amplification:
            return VK_SHADER_STAGE_TASK_BIT_EXT;
        case RenderBackendShaderStage::Mesh:
            return VK_SHADER_STAGE_MESH_BIT_EXT;
        default:
            std::unreachable();
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    static inline VkAttachmentLoadOp ConvertToVkAttachmentLoadOp(RenderBackendRenderPassBeginningAccessType loadOp)
    {
        switch (loadOp)
        {
        case RenderBackendRenderPassBeginningAccessType::Discard: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case RenderBackendRenderPassBeginningAccessType::Preserve: return VK_ATTACHMENT_LOAD_OP_LOAD;
        case RenderBackendRenderPassBeginningAccessType::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
        default: std::unreachable(); return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        }
    }

    static inline VkAttachmentStoreOp ConvertToVkAttachmentStoreOp(RenderBackendRenderPassEndingAccessType storeOp)
    {
        switch (storeOp)
        {
        case RenderBackendRenderPassEndingAccessType::Discard: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case RenderBackendRenderPassEndingAccessType::Preserve: return VK_ATTACHMENT_STORE_OP_STORE;
        default: std::unreachable(); return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        }
    }

    static inline VkImageAspectFlags GetVkImageAspectFlags(VkFormat format)
    {
        if (IsDepthOnlyFormat(format))
        {
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (IsDepthStencilFormat(format))
        {
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    static inline VkImageUsageFlags GetVkImageUsageFlags(RenderBackendTextureCreateFlags flags)
    {
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (EnumClassHasFlags(flags, RenderBackendTextureCreateFlags::UnorderedAccess))
        {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendTextureCreateFlags::ShaderResource))
        {
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendTextureCreateFlags::DepthStencil))
        {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendTextureCreateFlags::RenderTarget))
        {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        return usage;
    }

    static inline VkBufferUsageFlags GetVkBufferUsageFlags(RenderBackendBufferCreateFlags flags)
    {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::CopySrc))
        {
            usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::CopyDst))
        {
            usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::VertexBuffer))
        {
            usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::IndexBuffer))
        {
            usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::UniformBuffer))
        {
            usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::IndirectArguments))
        {
            usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::UnorderedAccess))
        {
            usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::RayTracingAccelerationStructure))
        {
            usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::ShaderBindingTable))
        {
            usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
        }
        return usage;
    }

    static inline VmaAllocationCreateFlags GetVmaAllocationCreateFlags(RenderBackendBufferCreateFlags flags)
    {
        VmaAllocationCreateFlags result = 0;
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::Readback))
        {
            result |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::Upload))
        {
            result |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::CreateMapped))
        {
            result |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }
        return result;
    }

    static inline VmaMemoryUsage GetVmaMemoryUsage(RenderBackendBufferCreateFlags flags)
    {
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::CpuOnly))
        {
            return VMA_MEMORY_USAGE_CPU_ONLY;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::GpuOnly))
        {
            return VMA_MEMORY_USAGE_GPU_ONLY;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::CpuToGpu))
        {
            return VMA_MEMORY_USAGE_CPU_TO_GPU;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::GpuToCpu))
        {
            return VMA_MEMORY_USAGE_GPU_TO_CPU;
        }
        return VMA_MEMORY_USAGE_AUTO;
    }

    static inline VkSamplerAddressMode ConvertToVkSamplerAddressMode(RenderBackendTextureAddressMode addressMode)
    {
        switch (addressMode)
        {
        case RenderBackendTextureAddressMode::Warp:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case RenderBackendTextureAddressMode::Mirror:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case RenderBackendTextureAddressMode::Clamp:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case RenderBackendTextureAddressMode::Border:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default:
            std::unreachable();
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }

    static inline VkBuildAccelerationStructureFlagsKHR ConvertToVkBuildAccelerationStructureFlagsKHR(RenderBackendRayTracingAccelerationStructureBuildFlags flags)
    {
        VkBuildAccelerationStructureFlagsKHR result = 0;
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::AllowUpdate))
        {
            result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        }
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::AllowCompaction))
        {
            result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        }
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastTrace))
        {
            result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        }
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastBuild))
        {
            result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
        }
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::MinimizeMemory))
        {
            result |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
        }
        return result;
    }

    static inline VkGeometryInstanceFlagsKHR ConvertToVkGeometryInstanceFlagsKHR(RenderBackendRayTracingInstanceFlags flags)
    {
        VkGeometryInstanceFlagsKHR result = 0;
        switch (flags)
        {
        case RenderBackendRayTracingInstanceFlags::TriangleFacingCullDisable:
            result |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            break;
        case RenderBackendRayTracingInstanceFlags::TriangleFrontCounterclockwise:
            result |= VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
            break;
        case RenderBackendRayTracingInstanceFlags::ForceOpaque:
            result |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
            break;
        case RenderBackendRayTracingInstanceFlags::ForceNoOpaque:
            result |= VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
            break;
        default:
            std::unreachable();
            break;
        }
        return result;
    }

    static inline VkGeometryFlagsKHR ConvertToVkGeometryFlagsKHR(RenderBackendRayTracingGeometryFlags flags)
    {
        VkGeometryFlagsKHR result = 0;
        switch (flags)
        {
        case RenderBackendRayTracingGeometryFlags::Opaque:
            result |= VK_GEOMETRY_OPAQUE_BIT_KHR;
            break;
        case RenderBackendRayTracingGeometryFlags::NoDuplicateAnyHitInvocation:
            result |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
            break;
        default:
            std::unreachable();
            break;
        }
        return result;
    }

    static inline VkCompareOp ConvertToVkCompareOp(RenderBackendCompareOp compareOp)
    {
        return (VkCompareOp)compareOp;
    }

    void GetVkFilterAndVkSamplerMipmapMode(
        RenderBackendTextureFilter filter,
        VkFilter* outMinFilter,
        VkFilter* outMagFilter,
        VkSamplerMipmapMode* outMipmapMode,
        bool* outAnisotropyEnable,
        bool* outCompareEnable)
    {
        switch (filter)
        {
        case RenderBackendTextureFilter::MinMagMipPoint:
        case RenderBackendTextureFilter::MinimumMinMagMipPoint:
        case RenderBackendTextureFilter::MaximumMinMagMipPoint:
            *outMinFilter = VK_FILTER_NEAREST;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = false;
            break;
        case RenderBackendTextureFilter::MinMagPointMipLinear:
        case RenderBackendTextureFilter::MinimumMinMagPointMipLinear:
        case RenderBackendTextureFilter::MaximumMinMagPointMipLinear:
            *outMinFilter = VK_FILTER_NEAREST;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            *outAnisotropyEnable = false;
            *outCompareEnable = false;
            break;
        case RenderBackendTextureFilter::MinPointMagLinearMipPoint:
        case RenderBackendTextureFilter::MinimumMinPointMagLinearMipPoint:
        case RenderBackendTextureFilter::MaximumMinPointMagLinearMipPoint:
            *outMinFilter = VK_FILTER_NEAREST;
            *outMagFilter = VK_FILTER_LINEAR;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = false;
            break;
        case RenderBackendTextureFilter::MinPointMagMipLinear:
        case RenderBackendTextureFilter::MinimumMinPointMagMipLinear:
        case RenderBackendTextureFilter::MaximumMinPointMagMipLinear:
            *outMinFilter = VK_FILTER_NEAREST;
            *outMagFilter = VK_FILTER_LINEAR;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            *outAnisotropyEnable = false;
            *outCompareEnable = false;
            break;
        case RenderBackendTextureFilter::MinLinearMagMipPoint:
        case RenderBackendTextureFilter::MinimumMinLinearMagMipPoint:
        case RenderBackendTextureFilter::MaximumMinLinearMagMipPoint:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = false;
            break;
        case RenderBackendTextureFilter::MinLinearMagPointMipLinear:
        case RenderBackendTextureFilter::MinimumMinLinearMagPointMipLinear:
        case RenderBackendTextureFilter::MaximumMinLinearMagPointMipLinear:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            *outAnisotropyEnable = false;
            *outCompareEnable = false;
            break;
        case RenderBackendTextureFilter::MinMagLinearMipPoint:
        case RenderBackendTextureFilter::MinimumMinMagLinearMipPoint:
        case RenderBackendTextureFilter::MaximumMinMagLinearMipPoint:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_LINEAR;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = false;
            break;
        case RenderBackendTextureFilter::MinMagMipLinear:
        case RenderBackendTextureFilter::MinimumMinMagMipLinear:
        case RenderBackendTextureFilter::MaximumMinMagMipLinear:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_LINEAR;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            *outAnisotropyEnable = false;
            *outCompareEnable = false;
            break;
        case RenderBackendTextureFilter::Anisotropic:
        case RenderBackendTextureFilter::MinimumAnisotropic:
        case RenderBackendTextureFilter::MaximumAnisotropic:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_LINEAR;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            *outAnisotropyEnable = true;
            *outCompareEnable = false;
            break;
        case RenderBackendTextureFilter::ComparisonMinMagMipPoint:
            *outMinFilter = VK_FILTER_NEAREST;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = true;
            break;
        case RenderBackendTextureFilter::ComparisonMinMagPointMipLinear:
            *outMinFilter = VK_FILTER_NEAREST;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            *outAnisotropyEnable = false;
            *outCompareEnable = true;
            break;
        case RenderBackendTextureFilter::ComparisonMinPointMagLinearMipPoint:
            *outMinFilter = VK_FILTER_NEAREST;
            *outMagFilter = VK_FILTER_LINEAR;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = true;
            break;
        case RenderBackendTextureFilter::ComparisonMinPointMagMipLinear:
            *outMinFilter = VK_FILTER_NEAREST;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = true;
            break;
        case RenderBackendTextureFilter::ComparisonMinLinearMagMipPoint:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = true;
            break;
        case RenderBackendTextureFilter::ComparisonMinLinearMagPointMipLinear:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            *outAnisotropyEnable = false;
            *outCompareEnable = true;
            break;
        case RenderBackendTextureFilter::ComparisonMinMagLinearMipPoint:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_LINEAR;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = true;
            break;
        case RenderBackendTextureFilter::ComparisonMinMagMipLinear:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_LINEAR;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            *outAnisotropyEnable = false;
            *outCompareEnable = true;
            break;
        case RenderBackendTextureFilter::ComparisonAnisotropic:
            *outMinFilter = VK_FILTER_LINEAR;
            *outMagFilter = VK_FILTER_LINEAR;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            *outAnisotropyEnable = true;
            *outCompareEnable = true;
            break;
        default:
            *outMinFilter = VK_FILTER_NEAREST;
            *outMagFilter = VK_FILTER_NEAREST;
            *outMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            *outAnisotropyEnable = false;
            *outCompareEnable = false;
            break;
        }
    }

    void GetBarrierInfo2(
        RenderBackendResourceState srcState,
        RenderBackendResourceState dstState,
        VkImageLayout* outOldLayout,
        VkImageLayout* outNewLayout,
        VkPipelineStageFlags2* outSrcStageMask,
        VkPipelineStageFlags2* outDstStageMask,
        VkAccessFlags2* outSrcAccessMask,
        VkAccessFlags2* outDstAccessMask)
    {
        switch (srcState)
        {
        case RenderBackendResourceState::Undefined:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            *outSrcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outSrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;
        case RenderBackendResourceState::VertexBuffer:
        case RenderBackendResourceState::IndexBuffer:
        case RenderBackendResourceState::ShaderResource:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            *outSrcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outSrcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
            break;
        case RenderBackendResourceState::Present:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            *outSrcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            *outSrcAccessMask = VK_ACCESS_2_NONE;
            break;
        case RenderBackendResourceState::RenderTarget:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
            *outSrcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            *outSrcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case RenderBackendResourceState::DepthStencilReadOnly:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            }
            *outSrcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outSrcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;
        case RenderBackendResourceState::DepthStencil:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            *outSrcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            *outSrcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case RenderBackendResourceState::CopySrc:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            }
            *outSrcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            *outSrcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            break;
        case RenderBackendResourceState::CopyDst:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            }
            *outSrcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            *outSrcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;
        case RenderBackendResourceState::UnorderedAccess:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
            *outSrcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outSrcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
            break;
        case RenderBackendResourceState::IndirectArgument:
            if (outOldLayout)
            {
                *outOldLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
            //*outSrcStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
            //*outSrcAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
            *outSrcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outSrcAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
            break;
        default: std::unreachable(); return;
        }

        switch (dstState)
        {
        case RenderBackendResourceState::Undefined:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            *outDstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outDstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;
        case RenderBackendResourceState::VertexBuffer:
        case RenderBackendResourceState::IndexBuffer:
        case RenderBackendResourceState::ShaderResource:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            *outDstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outDstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
            break;
        case RenderBackendResourceState::Present:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            }
            *outDstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            *outDstAccessMask = VK_ACCESS_2_NONE;
            break;
        case RenderBackendResourceState::RenderTarget:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
            *outDstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            *outDstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case RenderBackendResourceState::DepthStencil:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            *outDstStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            *outDstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case RenderBackendResourceState::DepthStencilReadOnly:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            }
            *outDstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outDstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;
        case RenderBackendResourceState::CopySrc:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            }
            *outDstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            *outDstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            break;
        case RenderBackendResourceState::CopyDst:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            }
            *outDstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            *outDstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;
        case RenderBackendResourceState::UnorderedAccess:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
            *outDstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outDstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
            break;
        case RenderBackendResourceState::IndirectArgument:
            if (outNewLayout)
            {
                *outNewLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
            //*outDstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
            //*outDstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
            *outDstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            *outDstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
            break;
        default: std::unreachable();
            return;
        }
    }
}