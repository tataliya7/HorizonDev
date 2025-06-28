#pragma once

#include "VulkanRenderBackendCommon.h"

namespace Horizon
{
    class VulkanDevice;
    class VulkanRenderBackend;
    class VulkanCommandBufferManager;
    struct VulkanBufferView;
    struct VulkanTextureView;

// Instance functions
#define VULKAN_INSTANCE_FUNCTION_LIST(EXPAND_OPERATION) \
    EXPAND_OPERATION(vkCreateInstance) \
    EXPAND_OPERATION(vkDestroyInstance) \
    EXPAND_OPERATION(vkEnumeratePhysicalDevices) \
    EXPAND_OPERATION(vkGetPhysicalDeviceFeatures) \
    EXPAND_OPERATION(vkGetPhysicalDeviceFormatProperties) \
    EXPAND_OPERATION(vkGetPhysicalDeviceImageFormatProperties) \
    EXPAND_OPERATION(vkGetPhysicalDeviceProperties) \
    EXPAND_OPERATION(vkGetPhysicalDeviceQueueFamilyProperties) \
    EXPAND_OPERATION(vkGetPhysicalDeviceMemoryProperties) \
    EXPAND_OPERATION(vkCreateDevice) \
    EXPAND_OPERATION(vkDestroyDevice) \
    EXPAND_OPERATION(vkGetDeviceProcAddr) \
    EXPAND_OPERATION(vkEnumerateInstanceExtensionProperties) \
    EXPAND_OPERATION(vkEnumerateDeviceExtensionProperties) \
    EXPAND_OPERATION(vkEnumerateInstanceLayerProperties) \
    EXPAND_OPERATION(vkEnumerateDeviceLayerProperties) \
    EXPAND_OPERATION(vkGetPhysicalDeviceFeatures2) \
    EXPAND_OPERATION(vkGetPhysicalDeviceProperties2) \
    EXPAND_OPERATION(vkGetPhysicalDeviceFormatProperties2) \
    EXPAND_OPERATION(vkGetPhysicalDeviceImageFormatProperties2) \
    EXPAND_OPERATION(vkGetPhysicalDeviceQueueFamilyProperties2) \
    EXPAND_OPERATION(vkGetPhysicalDeviceMemoryProperties2) \
    EXPAND_OPERATION(vkGetPhysicalDeviceSparseImageFormatProperties2) \
    EXPAND_OPERATION(vkCreateWin32SurfaceKHR) \
    EXPAND_OPERATION(vkDestroySurfaceKHR) \
    EXPAND_OPERATION(vkGetPhysicalDeviceSurfaceSupportKHR) \
    EXPAND_OPERATION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    EXPAND_OPERATION(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    EXPAND_OPERATION(vkGetPhysicalDeviceSurfacePresentModesKHR)

// Device functions
#define VULKAN_DEVICE_FUNCTION_LIST(EXPAND_OPERATION) \
    EXPAND_OPERATION(vkGetDeviceQueue) \
    EXPAND_OPERATION(vkQueueSubmit) \
    EXPAND_OPERATION(vkQueueWaitIdle) \
    EXPAND_OPERATION(vkDeviceWaitIdle) \
    EXPAND_OPERATION(vkAllocateMemory) \
    EXPAND_OPERATION(vkFreeMemory) \
    EXPAND_OPERATION(vkMapMemory) \
    EXPAND_OPERATION(vkUnmapMemory) \
    EXPAND_OPERATION(vkFlushMappedMemoryRanges) \
    EXPAND_OPERATION(vkInvalidateMappedMemoryRanges) \
    EXPAND_OPERATION(vkGetDeviceMemoryCommitment) \
    EXPAND_OPERATION(vkBindBufferMemory) \
    EXPAND_OPERATION(vkBindImageMemory) \
    EXPAND_OPERATION(vkGetBufferMemoryRequirements) \
    EXPAND_OPERATION(vkGetImageMemoryRequirements) \
    EXPAND_OPERATION(vkGetImageSparseMemoryRequirements) \
    EXPAND_OPERATION(vkQueueBindSparse) \
    EXPAND_OPERATION(vkCreateFence) \
    EXPAND_OPERATION(vkDestroyFence) \
    EXPAND_OPERATION(vkResetFences) \
    EXPAND_OPERATION(vkGetFenceStatus) \
    EXPAND_OPERATION(vkWaitForFences) \
    EXPAND_OPERATION(vkCreateSemaphore) \
    EXPAND_OPERATION(vkDestroySemaphore) \
    EXPAND_OPERATION(vkCreateEvent) \
    EXPAND_OPERATION(vkDestroyEvent) \
    EXPAND_OPERATION(vkGetEventStatus) \
    EXPAND_OPERATION(vkSetEvent) \
    EXPAND_OPERATION(vkResetEvent) \
    EXPAND_OPERATION(vkCreateQueryPool) \
    EXPAND_OPERATION(vkDestroyQueryPool) \
    EXPAND_OPERATION(vkGetQueryPoolResults) \
    EXPAND_OPERATION(vkCreateBuffer) \
    EXPAND_OPERATION(vkDestroyBuffer) \
    EXPAND_OPERATION(vkCreateBufferView) \
    EXPAND_OPERATION(vkDestroyBufferView) \
    EXPAND_OPERATION(vkCreateImage) \
    EXPAND_OPERATION(vkDestroyImage) \
    EXPAND_OPERATION(vkGetImageSubresourceLayout) \
    EXPAND_OPERATION(vkCreateImageView) \
    EXPAND_OPERATION(vkDestroyImageView) \
    EXPAND_OPERATION(vkCreateShaderModule) \
    EXPAND_OPERATION(vkDestroyShaderModule) \
    EXPAND_OPERATION(vkCreateGraphicsPipelines) \
    EXPAND_OPERATION(vkCreateComputePipelines) \
    EXPAND_OPERATION(vkDestroyPipeline) \
    EXPAND_OPERATION(vkCreatePipelineLayout) \
    EXPAND_OPERATION(vkDestroyPipelineLayout) \
    EXPAND_OPERATION(vkCreateSampler) \
    EXPAND_OPERATION(vkDestroySampler) \
    EXPAND_OPERATION(vkCreateDescriptorSetLayout) \
    EXPAND_OPERATION(vkDestroyDescriptorSetLayout) \
    EXPAND_OPERATION(vkCreateDescriptorPool) \
    EXPAND_OPERATION(vkDestroyDescriptorPool) \
    EXPAND_OPERATION(vkResetDescriptorPool) \
    EXPAND_OPERATION(vkAllocateDescriptorSets) \
    EXPAND_OPERATION(vkFreeDescriptorSets) \
    EXPAND_OPERATION(vkUpdateDescriptorSets) \
    EXPAND_OPERATION(vkCreateFramebuffer) \
    EXPAND_OPERATION(vkDestroyFramebuffer) \
    EXPAND_OPERATION(vkCreateRenderPass) \
    EXPAND_OPERATION(vkDestroyRenderPass) \
    EXPAND_OPERATION(vkGetRenderAreaGranularity) \
    EXPAND_OPERATION(vkCreateCommandPool) \
    EXPAND_OPERATION(vkDestroyCommandPool) \
    EXPAND_OPERATION(vkResetCommandPool) \
    EXPAND_OPERATION(vkAllocateCommandBuffers) \
    EXPAND_OPERATION(vkFreeCommandBuffers) \
    EXPAND_OPERATION(vkBeginCommandBuffer) \
    EXPAND_OPERATION(vkEndCommandBuffer) \
    EXPAND_OPERATION(vkResetCommandBuffer) \
    EXPAND_OPERATION(vkCmdBindPipeline) \
    EXPAND_OPERATION(vkCmdSetViewport) \
    EXPAND_OPERATION(vkCmdSetScissor) \
    EXPAND_OPERATION(vkCmdSetStencilReference) \
    EXPAND_OPERATION(vkCmdBindDescriptorSets) \
    EXPAND_OPERATION(vkCmdBindIndexBuffer) \
    EXPAND_OPERATION(vkCmdBindVertexBuffers) \
    EXPAND_OPERATION(vkCmdDraw) \
    EXPAND_OPERATION(vkCmdDrawIndexed) \
    EXPAND_OPERATION(vkCmdDrawIndirect) \
    EXPAND_OPERATION(vkCmdDrawIndexedIndirect) \
    EXPAND_OPERATION(vkCmdDispatch) \
    EXPAND_OPERATION(vkCmdDispatchIndirect) \
    EXPAND_OPERATION(vkCmdCopyBuffer) \
    EXPAND_OPERATION(vkCmdCopyImage) \
    EXPAND_OPERATION(vkCmdBlitImage) \
    EXPAND_OPERATION(vkCmdCopyBufferToImage) \
    EXPAND_OPERATION(vkCmdCopyImageToBuffer) \
    EXPAND_OPERATION(vkCmdUpdateBuffer) \
    EXPAND_OPERATION(vkCmdFillBuffer) \
    EXPAND_OPERATION(vkCmdClearColorImage) \
    EXPAND_OPERATION(vkCmdPipelineBarrier) \
    EXPAND_OPERATION(vkCmdBeginQuery) \
    EXPAND_OPERATION(vkCmdEndQuery) \
    EXPAND_OPERATION(vkCmdResetQueryPool) \
    EXPAND_OPERATION(vkCmdWriteTimestamp) \
    EXPAND_OPERATION(vkCmdCopyQueryPoolResults) \
    EXPAND_OPERATION(vkCmdPushConstants) \
    EXPAND_OPERATION(vkCmdBeginRenderPass) \
    EXPAND_OPERATION(vkCmdEndRenderPass) \
    EXPAND_OPERATION(vkCmdExecuteCommands) \
    EXPAND_OPERATION(vkCmdBeginRendering) \
    EXPAND_OPERATION(vkCmdEndRendering) \
    EXPAND_OPERATION(vkCreateSwapchainKHR) \
    EXPAND_OPERATION(vkDestroySwapchainKHR) \
    EXPAND_OPERATION(vkGetSwapchainImagesKHR) \
    EXPAND_OPERATION(vkAcquireNextImageKHR) \
    EXPAND_OPERATION(vkQueuePresentKHR) \
    EXPAND_OPERATION(vkGetDeviceGroupPresentCapabilitiesKHR) \
    EXPAND_OPERATION(vkGetDeviceGroupSurfacePresentModesKHR) \
    EXPAND_OPERATION(vkAcquireNextImage2KHR) \
    EXPAND_OPERATION(vkGetBufferDeviceAddress) \
    EXPAND_OPERATION(vkQueueSubmit2) \
    EXPAND_OPERATION(vkCmdPipelineBarrier2) \
    EXPAND_OPERATION(vkSetDebugUtilsObjectNameEXT) \
    EXPAND_OPERATION(vkCmdBeginDebugUtilsLabelEXT) \
    EXPAND_OPERATION(vkCmdEndDebugUtilsLabelEXT) \
    EXPAND_OPERATION(vkResetQueryPoolEXT) \
    EXPAND_OPERATION(vkCreateAccelerationStructureKHR) \
    EXPAND_OPERATION(vkDestroyAccelerationStructureKHR) \
    EXPAND_OPERATION(vkCmdBuildAccelerationStructuresKHR) \
    EXPAND_OPERATION(vkCmdBuildAccelerationStructuresIndirectKHR) \
    EXPAND_OPERATION(vkBuildAccelerationStructuresKHR) \
    EXPAND_OPERATION(vkGetAccelerationStructureDeviceAddressKHR) \
    EXPAND_OPERATION(vkGetAccelerationStructureBuildSizesKHR) \
    EXPAND_OPERATION(vkCmdTraceRaysKHR) \
    EXPAND_OPERATION(vkCreateRayTracingPipelinesKHR) \
    EXPAND_OPERATION(vkCmdTraceRaysIndirectKHR) \
    EXPAND_OPERATION(vkGetRayTracingShaderGroupHandlesKHR)

#define DEFINE_VULKAN_FUNCTION(function) PFN_##function function;

    struct VulkanInstanceSpecificFunctionTable
    {
        VULKAN_INSTANCE_FUNCTION_LIST(DEFINE_VULKAN_FUNCTION)
    };

    struct VulkanDeviceSpecificFunctionTable
    {
        VULKAN_DEVICE_FUNCTION_LIST(DEFINE_VULKAN_FUNCTION)
    };

    enum class VulkanDeviceExtensionType
    {
        KHR,      // Khronos (KHR) extensions
        EXT,      // Multi-vendor extensions
        Vendor    // Vendor-specific extensions
    };

    enum class VulkanDeviceExtensionRequirement
    {
        Required,
        Optional
    };

    // class VulkanDeviceExtension
    // {
    // public:
    //
    //     VulkanDeviceExtension(const char* name, VulkanDeviceExtensionType type, VulkanDeviceExtensionRequirement requirement)
    //         : name(name)
    //         , type(type)
    //         , requirement(requirement)
    //     {
    //
    //     }
    //
    //     bool IsRequired() const
    //     {
    //         return requirement == VulkanDeviceExtensionRequirement::Required;
    //     }
    //
    //     bool IsOptional() const
    //     {
    //         return requirement == VulkanDeviceExtensionRequirement::Optional;
    //     }
    //
    //     bool IsKhronosExtension() const
    //     {
    //         return type == VulkanDeviceExtensionType::KHR;
    //     }
    //
    //     bool IsMultiVendorExtension() const
    //     {
    //         return type == VulkanDeviceExtensionType::EXT;
    //     }
    //
    //     bool IsVendorSpecificExtension() const
    //     {
    //         return type == VulkanDeviceExtensionType::Vendor;
    //     }
    //
    //     virtual void OnProcessVkDeviceCreateInfo(VkDeviceCreateInfo& deviceCreateInfo) {}
    //
    // protected:
    //     const char* name;
    //     VulkanDeviceExtensionType type;
    //     VulkanDeviceExtensionRequirement requirement;
    // };
    //
    // class VulkanDeviceExtension_VK_KHR_synchronization2 : public VulkanDeviceExtension
    // {
    // public:
    //     VulkanDeviceExtension_VK_KHR_synchronization2(VulkanDeviceExtensionType type, VulkanDeviceExtensionRequirement requirement)
    //         : VulkanDeviceExtension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, type, requirement)
    //     {
    //
    //     }
    //
    //     void* OnProcessVkDeviceCreateInfo() override
    //     {
    //         return &synchronization2Features;
    //     }
    //
    // private:
    //     VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Features = {};
    // };

    struct VulkanRenderBackendHandleManager
    {
        std::vector<uint32> freeIndices;
        uint32 nextIndex;
        template <typename HandleType>
        HandleType Allocate(uint32 deviceMask = ~0U)
        {
            uint32 index = 0;
            if (!freeIndices.empty())
            {
                index = freeIndices.back();
                freeIndices.pop_back();
            }
            else
            {
                nextIndex++;
                index = nextIndex;
            }
            HandleType handle = HandleType(index, deviceMask);
            return handle;
        }
        template <typename HandleType>
        void Free(HandleType handle)
        {
            uint32 index = ((RenderBackendHandle)handle).GetIndex();
            freeIndices.push_back(index);
        }
    };

    // It must match the values on the shader side
    enum
    {
        BINDLESS_RESOURCE_BINDING_SAMPLER                   = 0,
        BINDLESS_RESOURCE_BINDING_TEXTURE_SRV               = 1,
        BINDLESS_RESOURCE_BINDING_TEXTURE_UAV               = 2,
        BINDLESS_RESOURCE_BINDING_BUFFER_CBV                = 3,
        BINDLESS_RESOURCE_BINDING_BUFFER_SRV_AND_UAV        = 4,
        BINDLESS_RESOURCE_BINDING_ACCELERATION_STRUCTURE_SRV    = 5,
    };

    struct VulkanBindlessConfig
    {
        uint32 numSamplers;
        uint32 numSampledImages;
        uint32 numStorageImages;
        uint32 numUniformBuffers;
        uint32 numStorageBuffers;
        uint32 numAccelerationStructures;
    };

    struct VulkanBindlessDescriptorManager
    {
        VulkanBindlessConfig config;

        VkDevice device;
        VkDescriptorPool pool;
        VkDescriptorSetLayout layout;
        VkDescriptorSet set;

        uint32 pushConstantsSize;

        VkPipelineLayout compatibleComputePipelineLayout;
        VkPipelineLayout compatibleGraphicsPipelineLayout;
        VkPipelineLayout compatibleRayTracingPipelineLayout;

        std::vector<uint32> freeSamplers;
        std::vector<uint32> freeSampledImages;
        std::vector<uint32> freeStorageImages;
        std::vector<uint32> freeUniformBuffers;
        std::vector<uint32> freeStorageBuffers;
        std::vector<uint32> freeAccelerationStructures;

        uint32 AllocateSampledImageIndex()
        {
            uint32 index = freeSampledImages.back();
            freeSampledImages.pop_back();
            return index;
        }

        uint32 AllocateSamplerIndex()
        {
            uint32 index = freeSamplers.back();
            freeSamplers.pop_back();
            return index;
        }

        uint32 AllocateStorageImageIndex()
        {
            uint32 index = freeStorageImages.back();
            freeStorageImages.pop_back();
            return index;
        }

        uint32 AllocateStorageBufferIndex()
        {
            uint32 index = freeStorageBuffers.back();
            freeStorageBuffers.pop_back();
            return index;
        }

        uint32 AllocateUniformBufferIndex()
        {
            uint32 index = freeUniformBuffers.back();
            freeUniformBuffers.pop_back();
            return index;
        }

        uint32 AllocateAccelerationStructureIndex()
        {
            uint32 index = freeAccelerationStructures.back();
            freeAccelerationStructures.pop_back();
            return index;
        }

        void UpdateDescriptor(VulkanTextureView* textureView, uint32 descriptorIndex, bool shaderResourceView, const VulkanDeviceSpecificFunctionTable& deviceFunctions);
    };

    struct VulkanPhysicalDevice
    {
        VkPhysicalDevice handle;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        VkPhysicalDeviceDescriptorIndexingProperties descriptorIndexingProperties;
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties;
        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties;
        VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR fragmentShaderBarycentricProperties;

        VkPhysicalDeviceVulkan12Features vulkan12Features;
        VkPhysicalDeviceMaintenance4FeaturesKHR maintenance4Features;
        VkPhysicalDeviceMaintenance5FeaturesKHR maintenance5Features;
        VkPhysicalDeviceMaintenance6FeaturesKHR maintenance6Features;
        VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures;
        VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufferDeviceAddressFeatures;
        VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Features;
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures;
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures;
        VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;
        VkPhysicalDeviceShaderFloat16Int8FeaturesKHR shaderFloat16Int8Features;
        VkPhysicalDevice16BitStorageFeaturesKHR float16StorageFeatures;
        VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures;
        VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timelineSemaphoreFeatures;
        VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR fragmentShaderBarycentricFeatures;
        VkPhysicalDeviceMultiviewFeaturesKHR multiviewFeatures;
        VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR separateDepthStencilLayoutsFeatures;
        VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT shaderDemoteToHelperInvocationFeaturesEXT;
        VkPhysicalDeviceScalarBlockLayoutFeaturesEXT scalarBlockLayoutFeaturesEXT;
        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT;
        VkPhysicalDeviceHostQueryResetFeaturesEXT hostQueryResetFeaturesEXT;

        VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR shaderRelaxedExtendedInstructionFeatures;

        void* featuresEntry;
        VkPhysicalDeviceFeatures2 enabledFeatures;
        std::vector<VkLayerProperties> layerProperties;
        std::vector<VkExtensionProperties> extensionProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        uint32 queueFamilyIndices[RenderBackendQueueFamilyCount];
    };

    struct VulkanQueue
    {
        VkQueue handle;
        uint32 familyIndex;
        uint32 queueIndex;
    };

    struct VulkanSwapchain
    {
        enum class Status
        {
            Success,
            OutOfDate,
            Error,
        };

        VkSwapchainKHR             handle;
        VkSurfaceKHR               surface;
        VkSwapchainCreateInfoKHR   info;
        uint32                     numBuffers;
        uint32                     activeBackBufferIndex;
        RenderBackendTextureHandle buffers[RenderBackendMaxSwapChainBufferCount];
        uint32                     numSemaphores;
        uint32                     semaphoreIndex;
        VkFence                    imageAcquiredFences[RenderBackendMaxSwapChainBufferCount];
        VkSemaphore                imageAcquiredSemaphores[RenderBackendMaxSwapChainBufferCount];
    };

    struct VulkanCpuReadbackBuffer
    {
        VkBuffer handle;
        uint64 mipOffsets[RenderBackendMaxMipLevelCount];
        uint64 mipSize[RenderBackendMaxMipLevelCount];
        void* data;
    };

    struct VulkanTexture
    {
        std::string name;
        VkImage handle;
        bool swapchainBuffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        uint32 width;
        uint32 height;
        uint32 depth;
        uint32 arrayLayers;
        uint32 mipLevels;
        VkFormat format;

        VkImageType type;
        VkImageCreateInfo info;
        uint64 sparsePageSize;
        RenderBackendTextureType t;
        RenderBackendTextureCreateFlags flags;
        VkImageAspectFlags aspectMask;
        VkClearValue clearValue;

        VkImageView defaultView = VK_NULL_HANDLE;
        int32 srvIndex = -1;

        std::vector<VkImageView> renderTargetViews;

        VkImageView depthStencilViews[1];

        bool IsArray() const
        {
            if ((t == RenderBackendTextureType::Texture2D) && arrayLayers > 1)
            {
                return true;
            }
            if ((t == RenderBackendTextureType::TextureCube) && arrayLayers > 6)
            {
                return true;
            }
            return false;
        }
        struct SRV
        {
            VkImageView srv = VK_NULL_HANDLE;
            int32 srvIndex = -1;
        };
        std::vector<SRV> srvs;
        struct UAV
        {
            VkImageView uav = VK_NULL_HANDLE;
            int32 uavIndex = -1;
        };
        std::vector<UAV> uavs;
        VulkanCpuReadbackBuffer* cpuReadbackBuffer;
    };

    struct VulkanTextureView
    {
        VkImage image;
        VkImageView imageView;
        RenderBackendTextureViewType viewType;
    };

    struct VulkanSampler
    {
        VkSampler handle;
        uint32 bindlessIndex;
    };

    struct VulkanRayTracingPipelineState
    {
        VkPipelineLayout pipelineLayout;
        VkPipeline handle;
        uint32 numRayGenerationShaders;
        uint32 numMissShaders;
        uint32 numHitGroups;
    };

    struct VulkanRayTracingShaderBindingTable
    {
        VkStridedDeviceAddressRegionKHR rayGenShaderBindingTable;
        VkStridedDeviceAddressRegionKHR missShaderBindingTable;
        VkStridedDeviceAddressRegionKHR hitShaderBindingTable;
        VkStridedDeviceAddressRegionKHR callableShaderBindingTable;
    };

    struct VulkanBuffer
    {
        VkBuffer handle;
        VmaAllocation allocation;
        uint64 size;
        VkIndexType indexType;
        VkBufferUsageFlags usageFlags;
        VmaAllocationCreateFlags allocationFlags;
        VmaMemoryUsage memeryUsage;
        bool createMapped;
        bool mapped;
        void* mappedData;
        int32 bindlessResourceDescriptorIndexCBV;
        int32 bindlessResourceDescriptorIndexSRV;
        int32 bindlessResourceDescriptorIndexUAV;
        std::string name;
        VulkanRayTracingShaderBindingTable* shaderBindingTable;
        VkDeviceAddress deviceAddress;
    };

    struct VulkanGraphicsPipelineStateDesc
    {
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[RenderBackendMaxRenderTargetCount];
    };

    struct VulkanShader
    {
        VkPipelineShaderStageCreateInfo stageInfo;
        VkShaderModule module;
    };

    struct VulkanRayTracingAccelerationStructure
    {
        uint32 deviceMask;
        VkAccelerationStructureKHR handle;
        VkBuildAccelerationStructureFlagsKHR buildFlags;
        VkDeviceAddress deviceAddress;
        struct Buffer
        {
            VkBuffer buffer;
            uint64 size;
            VmaAllocation allocation;
            VmaAllocationInfo allocationInfo;
            VkDeviceAddress deviceAddress;
        };
        Buffer accelerationStructureBuffer;
        Buffer scratchBuffer;
        uint32 instanceCount = 0;
        std::vector<Buffer> resourceBuffers;
        uint32 descriptorIndex;
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
    };

    struct VulkanRenderingInfo
    {
        bool hasDepthStencil;
        VkExtent3D extent;
        VkRenderingInfo renderingInfo;
        uint32 colorAttachmentCount;
        VkFormat colorAttachmentFormats[RenderBackendMaxRenderTargetCount];
        VkRenderingAttachmentInfo colorAttachments[RenderBackendMaxRenderTargetCount];
        VkFormat depthStencilAttachmentFormat;
        VkRenderingAttachmentInfo depthStencilAttachment;
        VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo;
    };

    struct VulkanPipeline
    {
        uint64 hash;
        VkPipeline handle;
        VkPipelineLayout layout;
    };

    struct VulkanPipelineManager
    {
        VkPipelineCache pipelineCache;
        std::unordered_map<uint64, VulkanPipeline> pipelineMap;
        std::vector<VulkanPipeline> pipelines;
        std::unordered_map<uint64, VkPipelineLayout> pipelineLayoutMap;
        std::vector<VkPipelineLayout> pipelineLayouts;
    };

    struct VulkanCommandBuffer
    {
        VkCommandBuffer handle;
        VkSemaphore semaphore;
        VkFence fence;
    };

    struct VulkanSubmitContext
    {
        VkSemaphore completeSemaphore;
        VkFence completeFence;
        uint32 numPrimaryCommandBuffers;
        uint32 numSecondaryCommandBuffers;
    };

    struct VulkanFramebuffer
    {
        VkFramebuffer handle;
        uint32 width;
        uint32 height;
        uint32 layers;
        uint32 attachmentCount;
        uint32 colorAttachmentCount;
        VkImage images[RenderBackendMaxRenderTargetCount + 1];
        VkImageView attachments[RenderBackendMaxRenderTargetCount + 1];
    };

    struct VulkanRenderPassDesc
    {
        VkExtent3D extent;
        bool hasDepthStencil;
        uint32 renderPassCompatibleHash;
        uint32 renderPassFullHash;
        uint32 attachmentDescriptionCount;
        uint32 colorAttachmentCount;
        VkAttachmentDescription attachmentDescriptions[RenderBackendMaxRenderTargetCount + 1];
        VkAttachmentReference colorReferences[RenderBackendMaxRenderTargetCount];
        VkAttachmentReference depthStencilReference;
        VkImageLayout depthStencilLayout;
    };

    struct VulkanTimingQueryHeap
    {
        VkQueryPool handle;
        uint32 maxQueryCount;
    };

    struct RenderPassCompatibleHashInfo
    {
        uint8 attachmentCount;
        VkFormat formats[RenderBackendMaxRenderTargetCount + 1];
    };

    struct RenderPassFullHashInfo
    {
        /** +2 : 1 for depth, 1 for stencil. */
        uint8 loadOps[RenderBackendMaxRenderTargetCount + 2];
        uint8 storeOps[RenderBackendMaxRenderTargetCount + 2];
    };

    struct ResourceToDestroy
    {
        enum class Type
        {
            Buffer,
            Texture,
            Sampler,
        };
        Type type;
        uint64 vkHandle;
        VmaAllocation allocation;
        int32 bindlessSRV;
        int32 bindlessUAV;
    };

    class VulkanDevice
    {
    public:
        VulkanDevice();
        ~VulkanDevice();
        bool Init(VulkanRenderBackend* backend, VulkanPhysicalDevice* physicalDevive, const VulkanBindlessConfig& bindlessConfig);
        void Shutdown();
        void Tick();
        void WaitIdle();
        bool IsDeviceExtensionEnabled(const char* extension);
        void ResizeSwapChain(uint32 index, uint32* width, uint32* height);
        VulkanSwapchain::Status AcquireImageIndex(uint32 index);
        VulkanSwapchain::Status PresentSwapChain(uint32 index, VkSemaphore* waitSemaphores, uint32 waitSemaphoreCount);
        void RecreateSwapChain(uint32 index);
        RenderBackendTextureHandle GetActiveSwapChainBackBuffer(uint32 index);
        uint32 CreateSwapChain(const RenderBackendSwapChainDesc* desc);
        void DestroySwapChain(uint32 index);
        uint32 CreateBuffer(const RenderBackendBufferDescription* desc, const void* data, const char* name);
        void DestroyBuffer(uint32 index);
        void ResizeBuffer(uint32 index, uint64 size);
        void* MapBuffer(uint32 index);
        void UnmapBuffer(uint32 index);
        uint64 GetBufferDeviceAddress(RenderBackendBufferHandle bufferHandle);
        uint32 CreateTexture(const RenderBackendTextureDesc* desc, const void* data, const char* name);
        void DestroyTexture(uint32 index);
        uint32 CreateTextureSRV(uint32 textureIndex, const RenderBackendTextureSRVDesc* desc, const char* name);
        uint32 CreateTextureUAV(uint32 textureIndex, const RenderBackendTextureUAVDesc* desc, const char* name);
        int32 GetTextureSRVBindlessResourceDescriptorIndex(uint32 textureIndex);
        int32 GetTextureSRVBindlessResourceDescriptorIndex(uint32 textureIndex, uint32 mipLevel);
        int32 GetTextureUAVBindlessResourceDescriptorIndex(uint32 textureIndex, uint32 mipLevel);
        int32 GetBufferCBVBindlessResourceDescriptorIndex(uint32 bufferIndex);
        int32 GetBufferSRVBindlessResourceDescriptorIndex(uint32 bufferIndex);
        int32 GetBufferUAVBindlessResourceDescriptorIndex(uint32 bufferIndex);
        uint32 CreateSampler(const RenderBackendSamplerDesc* desc, const char* name);
        void DestroySampler(uint32 index);
        uint32 CreateShader(const RenderBackendShaderDesc* desc, const char* name);
        void DestroyShader(uint32 index);
        uint32 CreateRayTracingBottomLevelAccelerationStructure(const RenderBackendRayTracingBottomLevelAccelerationStructureDesc* desc, const char* name);
        uint32 CreateRayTracingTopLevelAccelerationStructure(const RenderBackendRayTracingTopLevelAccelerationStructureDesc* desc, const char* name);
        VkRenderPass FindOrCreateRenderPass(const VulkanRenderPassDesc& renderPassDesc);
        VulkanFramebuffer* FindOrCreateFramebuffer(const RenderBackendRenderPassInfo& renderPassInfo, const VulkanRenderPassDesc& renderPassDesc, VkRenderPass renderPass);
        VkPipelineLayout FindOrCreatePipelineLayout(uint32 pushConstantsSize, RenderBackendPipelineType pipelineType);
        VulkanPipeline* FindOrCreateComputePipeline(VulkanShader* computeShader, uint32 pushConstantsSize);
        VulkanPipeline* FindOrCreateGraphicsPipeline(
            VulkanShader* vertexShader,
            VulkanShader* pixelShader,
            VulkanShader* taskShader,
            VulkanShader* meshShader,
            const RenderBackendGraphicsPipelineStateDescription& pipelineState,
            uint32 pushConstantsSize,
            RenderBackendPrimitiveTopology topology,
            bool useDynamicRendering,
            VulkanRenderingInfo* renderingInfo,
            VkRenderPass renderPass,
            uint32 activeColorAttachmentCount);
        //VulkanPipeline* FindOrCreateRayTracingPipeline(VulkanShader* shader, uint32 pushConstantsSize);
        void SetDebugUtilsObjectName(VkObjectType objectType, uint64 objectHandle, const char* objectName);

        VkPhysicalDevice GetPhysicalDeviceHandle() const
        {
            return physicalDevice->handle;
        }

        inline VkDevice GetHandle() const
        {
            return handle;
        }
        inline VulkanRenderBackend* GetBackend()
        {
            return backend;
        }
        inline uint32 GetDeviceMask() const
        {
            return deviceMask;
        }
        inline VkDescriptorSet GetBindlessGlobalSet() const
        {
            return bindlessDescriptorManager.set;
        }
        inline uint32 GetQueueFamilyIndex(RenderBackendQueueFamily family) const
        {
            return physicalDevice->queueFamilyIndices[(uint32)family];
        }
        inline VulkanQueue* GetCommandQueue(uint32 family, uint32 index)
        {
            return &commandQueues[family].at(index);
        }
        inline VulkanQueue* GetCommandQueue(RenderBackendQueueFamily family, uint32 index)
        {
            return &commandQueues[(uint32)family].at(index);
        }
        inline const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingPipelineProperties() const
        {
            return physicalDevice->rayTracingPipelineProperties;
        }
        inline VulkanTexture* GetTexture(RenderBackendTextureHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return &textures[index];
        }
        inline VulkanBuffer* GetBuffer(RenderBackendBufferHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return &buffers[index];
        }
        inline VulkanSampler* GetSampler(RenderBackendSamplerHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return &samplers[index];
        }
        inline VulkanShader* GetShader(RenderBackendShaderHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return &shaders[index];
        }
        inline const VulkanTimingQueryHeap& GetTimingQueryHeap(RenderBackendTimingQueryHeapHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return timingQueryHeaps.Get(index);
        }
        inline VulkanRayTracingAccelerationStructure* GetRayTracingAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return &accelerationStructures[index];
        }
        inline VulkanRayTracingPipelineState* GetRayTracingPipelineState(RenderBackendRayTracingPipelineStateHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return &rayTracingPipelineStates[index];
        }
        inline uint32 GetRenderBackendHandleRepresentation(uint32 handle)
        {
            if (handleRepresentations.find(handle) == handleRepresentations.end())
            {
                assert(false);
            }
            //assert(handleRepresentations.find(handle) != handleRepresentations.end());
            return handleRepresentations[handle];
        }
        inline bool TryGetRenderBackendHandleRepresentation(uint32 handle, uint32* outValue)
        {
            if (handleRepresentations.find(handle) == handleRepresentations.end())
            {
                return false;
            }
            *outValue = handleRepresentations[handle];
            return true;
        }
        inline void SetRenderBackendHandleRepresentation(uint32 handle, uint32 value)
        {
            handleRepresentations[handle] = value;
        }
        inline bool RemoveRenderBackendHandleRepresentation(uint32 handle)
        {
            return handleRepresentations.erase(handle);
        }

        VulkanCommandBufferManager* commandBufferManager;

        std::vector<VkSemaphore> presentSemaphores;
        std::vector<VulkanSwapchain> swapchains;

        void CreateVmaAllocator();
        void DestroyVmaAllocator();
        bool CreateBindlessDescriptorManager(const VulkanBindlessConfig& bindlessConfig);
        void DestroyBindlessDescriptorManager();
        uint32 CreateAccelerationStructure(VulkanRayTracingAccelerationStructure* accelerationStructure, VkAccelerationStructureTypeKHR type, uint32* primitiveCounts, const char* name);

        void BindBindlessDescriptorSets(VkCommandBuffer commandBuffer);

        MemoryArena*          allocator;
        VulkanRenderBackend*  backend;
        VulkanPhysicalDevice* physicalDevice;
        VkInstance            instance;
        VkDevice              handle;
        VmaAllocator          vmaAllocator;
        uint32                deviceMask;

        VulkanDeviceSpecificFunctionTable deviceFunctions;

        std::vector<const char*> enabledDeviceExtensions;
        std::vector<const char*> enabledValidationLayers;

        uint32 numCommandQueues[RenderBackendQueueFamilyCount] = { 1, 1, 1, 1 };
        std::vector<VulkanQueue> commandQueues[RenderBackendQueueFamilyCount];

        VulkanBindlessDescriptorManager bindlessDescriptorManager;
        VulkanPipelineManager pipelineManager;

        struct FramebufferList
        {
            std::vector<VulkanFramebuffer> framebuffers;
        };
        std::map<uint32, FramebufferList> cachedFramebuffers;
        std::map<uint32, VkRenderPass> cachedRenderPasses;

        template <typename ResourceType>
        struct ResourceContainer
        {
            uint32 Add(const ResourceType& resource)
            {
                uint32 index = 0;
                if (!freeResourceIndices.empty())
                {
                    index = freeResourceIndices.back();
                    freeResourceIndices.pop_back();
                    resources[index] = resource;
                }
                else
                {
                    index = (uint32)resources.size();
                    resources.emplace_back(resource);
                }
                return index;
            }
            void Free(uint32 index)
            {
                freeResourceIndices.push_back(index);
            }
            const ResourceType& Get(uint32 index)
            {
                return resources[index];
            }
            std::vector<ResourceType> resources;
            std::vector<uint32> freeResourceIndices;
        };

        std::vector<VulkanBuffer> buffers;
        std::vector<uint32> freeBuffers;
        std::vector<VulkanTexture> textures;
        std::vector<uint32> freeTextures;
        std::vector<VulkanSampler> samplers;
        std::vector<uint32> freeSamplers;
        std::vector<VulkanShader> shaders;
        std::vector<uint32> freeShaders;
        ResourceContainer<VulkanTimingQueryHeap> timingQueryHeaps;

        std::vector<VulkanRayTracingAccelerationStructure> accelerationStructures;
        std::vector<uint32> freeAccelerationStructures;
        std::vector<VulkanRayTracingPipelineState> rayTracingPipelineStates;

        std::queue<ResourceToDestroy> resourcesToDestroy;

        std::map<uint32, uint32> handleRepresentations;
    };

    class VulkanCommandBufferManager
    {
    public:

        VulkanCommandBufferManager(VulkanDevice* device, RenderBackendQueueFamily family)
            : device(device)
            , queueFamily(family)
        {
            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = device->GetQueueFamilyIndex(family);
            VK_CHECK(device->deviceFunctions.vkCreateCommandPool(device->GetHandle(), &poolInfo, VULKAN_ALLOCATION_CALLBACKS, &pool));
        }

        ~VulkanCommandBufferManager()
        {
            device->deviceFunctions.vkDestroyCommandPool(device->GetHandle(), pool, VULKAN_ALLOCATION_CALLBACKS);
            pool = VK_NULL_HANDLE;
        }

        inline VkCommandPool GetCommandPoolHandle() const
        {
            return pool;
        }

        VulkanCommandBuffer* AllocateCommandBuffer()
        {
            VulkanCommandBuffer commandBuffer;

            VkCommandBufferAllocateInfo allocateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_CHECK(device->deviceFunctions.vkAllocateCommandBuffers(device->GetHandle(), &allocateInfo, &commandBuffer.handle));

            VkFenceCreateInfo fenceInfo =
            {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };
            VK_CHECK(device->deviceFunctions.vkCreateFence(device->GetHandle(), &fenceInfo, VULKAN_ALLOCATION_CALLBACKS, &commandBuffer.fence));

            VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                .pNext = nullptr,
                .semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
                .initialValue = 0,
            };
            VkSemaphoreCreateInfo semaphoreCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = &semaphoreTypeCreateInfo,
                .flags = 0,
            };
            VK_CHECK(device->deviceFunctions.vkCreateSemaphore(device->GetHandle(), &semaphoreCreateInfo, VULKAN_ALLOCATION_CALLBACKS, &commandBuffer.semaphore));

            commandBuffers.emplace_back(commandBuffer);

            return &commandBuffers.back();
        }

        VulkanCommandBuffer* PrepareForNextCommandBuffer()
        {
            for (VulkanCommandBuffer& commandBuffer : commandBuffers)
            {
                VkResult result = device->deviceFunctions.vkGetFenceStatus(device->GetHandle(), commandBuffer.fence);
                switch (result)
                {
                case VK_SUCCESS:
                    return &commandBuffer;
                    break;
                case VK_NOT_READY:
                    break;
                default:
                    std::unreachable();
                    break;
                }
            }
            return AllocateCommandBuffer();
        }

    private:

        VulkanDevice* device;
        RenderBackendQueueFamily queueFamily;
        VkCommandPool pool;
        std::vector<VulkanCommandBuffer> commandBuffers;
    };

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData)
    {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            LogVerbose(GLogger, std::format("{}: {}", callbackData->messageIdNumber, callbackData->pMessage));
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            LogInfo(GLogger, std::format("{}: {}", callbackData->messageIdNumber, callbackData->pMessage));
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            LogWarning(GLogger, std::format("{} - {}: {}", callbackData->messageIdNumber, callbackData->pMessageIdName, callbackData->pMessage));
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            LogError(GLogger, std::format("{} - {}: {}", callbackData->messageIdNumber, callbackData->pMessageIdName, callbackData->pMessage));
        }
        return VK_FALSE;
    }

    static bool CheckInstanceExtensionSupport(const char* name, const std::vector<VkExtensionProperties>& supportedExtensions)
    {
        for (const VkExtensionProperties& supportedExtension : supportedExtensions)
        {
            if (strcmp(name, supportedExtension.extensionName) == 0)
            {
                return true;
            }
        }
        return false;
    }

    static bool CheckInstanceLayerSupport(const char* name, const std::vector<VkLayerProperties>& supportedLayers)
    {
        for (const VkLayerProperties& supportedLayer : supportedLayers)
        {
            if (strcmp(name, supportedLayer.layerName) == 0)
            {
                return true;
            }
        }
        return false;
    }

    class VulkanRenderBackendCommandListContext
    {
    public:
        VulkanRenderBackendCommandListContext(VulkanDevice* device, RenderBackendQueueFamily family, VkCommandBuffer commandBuffer)
            : device(device)
            , queueFamily(family)
            , commandBuffer(commandBuffer)
            , activeRenderPass(VK_NULL_HANDLE)
            , activeColorAttachmentCount(0)
            , activeComputePipeline(VK_NULL_HANDLE)
            , activeGraphicsPipeline(VK_NULL_HANDLE)
            , activeRayTracingPipeline(VK_NULL_HANDLE)
            , imageBarriers()
            , bufferBarriers() {}
        virtual ~VulkanRenderBackendCommandListContext() = default;
        inline RenderBackendQueueFamily GetQueueFamily() const { return queueFamily; }
        inline VkCommandBuffer GetCommandBuffer() const { return commandBuffer; }
        bool CompileRenderBackendCommands(const RenderBackendCommandContainer& container);
        bool CompileRenderBackendCommand(const RenderBackendCommandCopyBuffer& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandCopyTexture& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandUpdateBuffer& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandUpdateTexture& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandClearBufferUAV& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandClearTextureUAV& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBarriers& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBeginTimingQuery& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandEndTimingQuery& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandResolveTimingQueryResults& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatch& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchIndirect& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandSetViewport& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandSetScissor& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandSetStencilReference& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBeginRenderPass& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandEndRenderPass& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDraw& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDrawIndirect& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchMesh& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchMeshIndirect& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBeginDebugLabel& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandEndDebugLabel& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBuildRayTracingBottomLevelAccelerationStructure& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBuildRayTracingTopLevelAccelerationStructure& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchRays& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchSuperSampling& command);
    private:
        void ApplyTransitions();
        bool PrepareForDispatch(RenderBackendShaderHandle computeShader, const RenderBackendPushConstantValues& shaderConstants);
        bool PrepareForDraw(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineStateDescription& pipelineState, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendPushConstantValues& shaderConstants);
        bool PrepareForMeshShading(RenderBackendShaderHandle amplificationShader, RenderBackendShaderHandle meshShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineStateDescription& pipelineState, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendPushConstantValues& shaderConstants);
        VulkanDevice* device;
        RenderBackendQueueFamily queueFamily;
        VkCommandBuffer commandBuffer;
        VkRenderPass activeRenderPass;
        uint32 activeColorAttachmentCount;
        VulkanRenderingInfo renderingInfo;
        bool insideRenderPass;
        VkPipeline activeComputePipeline;
        VkPipeline activeGraphicsPipeline;
        VkPipeline activeRayTracingPipeline;
        std::vector<VkImageMemoryBarrier2> imageBarriers;
        std::vector<VkBufferMemoryBarrier2> bufferBarriers;
    };

    class VulkanRenderBackend final : public RenderBackend
    {
    public:

        RenderBackendType GetType() const override
        {
            return RenderBackendType::Vulkan;
        }

        bool Init(const RenderBackendDesc* desc);
        void Exit();

        void EnumeratePhysicalDevices();

        bool IsInstanceExtensionEnabled(const char* extension);

        void Tick() override;
        void CreateRenderDevices(PhysicalDeviceID* physicalDeviceIDs, uint32 numDevices, uint32* outDeviceMasks) override;
        void DestroyRenderDevices() override;
        void FlushRenderDevices() override;
        RenderBackendDeviceContext GetNativeDevice() override;
        void GetRenderBackendVulkanInfo(RenderBackendVulkanInfo* vulkanInfo) override;
        RenderBackendSwapChainHandle CreateSwapChain(const RenderBackendSwapChainDesc* desc) override;
        void DestroySwapChain(RenderBackendSwapChainHandle swapChain) override;
        void ResizeSwapChain(RenderBackendSwapChainHandle swapChain, uint32* width, uint32* height) override;
        bool PresentSwapChain(RenderBackendSwapChainHandle swapChain) override;
        RenderBackendTextureHandle GetActiveSwapChainBuffer(RenderBackendSwapChainHandle swapChain) override;
        RenderBackendBufferHandle CreateBuffer(const RenderBackendBufferDescription* desc, const void* data, const char* name) override;
        void DestroyBuffer(RenderBackendBufferHandle buffer) override;
        void ResizeBuffer(RenderBackendBufferHandle buffer, uint64 size) override;
        void MapBuffer(RenderBackendBufferHandle buffer, void** data) override;
        void UnmapBuffer(RenderBackendBufferHandle buffer) override;
        RenderBackendTextureHandle CreateTexture(const RenderBackendTextureDesc* desc, const void* data, const char* name) override;
        void DestroyTexture(RenderBackendTextureHandle texture) override;
        void UploadTexture(RenderBackendTextureHandle handle, const RenderBackendTextureUploadDataDesc& data) override;
        RenderBackendTextureViewHandle CreateTextureView(RenderBackendTextureHandle textureHandle, const RenderBackendTextureViewDesc* desc, int32* descriptor) override;
        void GetTextureReadbackData(RenderBackendTextureHandle texture, void** data) override;
        //RenderBackendTextureSRVHandle CreateTextureSRV(const RenderBackendTextureSRVDesc* desc, const char* name) override;
        //RenderBackendTextureUAVHandle CreateTextureUAV(const RenderBackendTextureUAVDesc* desc, const char* name) override;
        int32 GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle) override;
        int32 GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle, uint32 mipLevel) override;
        int32 GetTextureUAVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle, uint32 mipLevel) override;
        int32 GetBufferCBVBindlessResourceDescriptorIndex(RenderBackendBufferHandle handle) override;
        int32 GetBufferSRVBindlessResourceDescriptorIndex(RenderBackendBufferHandle handle) override;
        int32 GetBufferUAVBindlessResourceDescriptorIndex(RenderBackendBufferHandle handle) override;
        int32 GetAccelerationStructureSRVBindlessResourceDescriptorIndex(RenderBackendRayTracingAccelerationStructureHandle handle) override;
        RenderBackendSamplerHandle CreateSampler(const RenderBackendSamplerDesc* desc, const char* name) override;
        void DestroySampler(RenderBackendSamplerHandle sampler) override;
        RenderBackendShaderHandle CreateShader(const RenderBackendShaderDesc* desc, const char* name) override;
        void DestroyShader(RenderBackendShaderHandle shader) override;
        RenderBackendTimingQueryHeapHandle CreateTimingQueryHeap(const RenderBackendTimingQueryHeapDesc* desc, const char* name) override;
        void DestroyTimingQueryHeap(RenderBackendTimingQueryHeapHandle timingQueryHeap) override;
        void SubmitCommandLists(RenderBackendCommandList** commandLists, uint32 numCommandLists, RenderBackendSwapChainHandle swapChain) override;
        RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingBottomLevelAccelerationStructure(const RenderBackendRayTracingBottomLevelAccelerationStructureDesc* desc, const char* name) override;
        RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingTopLevelAccelerationStructure(const RenderBackendRayTracingTopLevelAccelerationStructureDesc* desc, const char* name) override;
        RenderBackendRayTracingPipelineStateHandle CreateRayTracingPipelineState(const RenderBackendRayTracingPipelineStateDesc* desc, const char* name) override;
        RenderBackendBufferHandle CreateRayTracingShaderBindingTable(const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name) override;
        void SetObjectName(RenderBackendTextureHandle handle, const char* name) override
        {
            VulkanTexture* texture = device.GetTexture(handle);
            device.SetDebugUtilsObjectName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64>(texture->handle), name);
        }
        void SetObjectName(RenderBackendBufferHandle handle, const char* name) override
        {
            VulkanBuffer* buffer = device.GetBuffer(handle);
            device.SetDebugUtilsObjectName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64>(buffer->handle), name);
        }
        const char*    applicationName;
        uint32         applicationVersion;
        const char*    engineName;
        uint32         engineVersion;

        VkInstance instance;

        bool enableValidationLayers = false;

        std::vector<const char*> enabledInstanceLayers;
        std::vector<const char*> enabledInstanceExtensions;

        VkDebugUtilsMessengerEXT debugUtilsMessenger;

        VulkanDevice device;

        std::vector<VulkanPhysicalDevice> availablePhysicalDevices;

        VulkanRenderBackendHandleManager handleManager;

        bool enableMeshShaderSupport = false;
        bool enableHardwareRayTracing = false;

        VulkanInstanceSpecificFunctionTable instanceFunctions;
    };
}

namespace Horizon::Refactor
{
    class VulkanInstance;
    class VulkanPhysicalDeviceManager;
    class VulkanDevice;

    class VulkanPhysicalDeviceManager
    {
    public:
    private:

        VulkanInstance* instance;
    };

    class VulkanInstance
    {
    public:
    private:

        VulkanInstance();

        VkAllocationCallbacks allocationCallbacks;

        const uint32 apiVersion;

        const uint32 appVersion;

        std::vector<const char*> enabledInstanceLayers;

        std::vector<const char*> enabledInstanceExtensions;

        VkDebugUtilsMessengerEXT debugUtilsMessenger;

        VulkanInstanceSpecificFunctionTable instanceFunctions;
    };

    class VulkanDevice
    {
    public:

        void WaitIdle();

        VulkanBuffer* CreateBuffer(
            const RenderBackendBufferDescription* desc,
            const void* data,
            const char* name);

        VulkanBufferView* CreateBufferView(
            const RenderBackendBufferViewDesc* desc,
            const char* name);

        VulkanTexture* CreateTexture(
            const RenderBackendTextureDesc* desc,
            const void* data,
            const char* name);

        VulkanTexture* CreateTextureView(
            const RenderBackendTextureViewDesc* desc,
            const char* name);

        VulkanSampler* CreateSampler(
            const RenderBackendSamplerDesc* desc,
            const char* name);

    protected:

        VulkanDeviceSpecificFunctionTable deviceFunctions;
    };
}