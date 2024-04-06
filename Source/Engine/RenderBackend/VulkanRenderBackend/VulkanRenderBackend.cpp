#include "RenderBackend/VulkanRenderBackend/VulkanRenderBackendCommon.h"
#include "RenderBackend/VulkanRenderBackend/VulkanRenderBackend.h"
#include "RenderBackend/VulkanRenderBackend/VulkanRenderBackendDefinitions.h"
#include "RenderBackend/VulkanRenderBackend/VulkanRenderBackendUtils.h"

#include <optick.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if HE_ENBALE_STREAMLINE_SUPPORT
#include <sl.h>
#include <sl_consts.h>
#include <sl_hooks.h>
#include <sl_version.h>

// #include <sl_nrd.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_reflex.h>
#include <sl_helpers.h>
#include <sl_helpers_vk.h>

static void StreamlineLogMessageCallback(sl::LogType type, const char* msg)
{
    switch (type)
    {
    case sl::LogType::eError:
        HE::LogError(HE::GLogger, std::format("Streamline: {}", msg));
        break;
    case sl::LogType::eWarn:
        HE::LogWarning(HE::GLogger, std::format("Streamline: {}", msg));
        break;
    case sl::LogType::eInfo:
        HE::LogInfo(HE::GLogger, std::format("Streamline: {}", msg));
        break;
        break;
    }
}

static void myAPIErrorCallback(const sl::APIError& e)
{
    // Handle error, use e.hres with DirectX and e.vkRes on Vulkan
    printf("VkResult %d\n", e.vkRes);
};

#endif

#include <ffx_fsr2.h>
#include <vk/ffx_fsr2_vk.h>

#define VK_CHECK(VkFunction) { const VkResult result = VkFunction; if (result != VK_SUCCESS) { VerifyVkResult(result, #VkFunction, __FILE__, __LINE__); } }
#define VK_CHECK_RESULT(result) { if (result != VK_SUCCESS) { VerifyVkResult(result, __FUNCTION__, __FILE__, __LINE__); } }
#define NGX_CHECK(result) assert(result == NVSDK_NGX_Result_Success)


#ifndef VULKAN_RENDER_BACKEND_DYNAMIC_RENDERING
#define VULKAN_RENDER_BACKEND_DYNAMIC_RENDERING 1
#endif

namespace HE
{
    static void FSR2MessageCallBack(FfxFsr2MsgType type, const wchar_t* message)
    {
        if (type == FFX_FSR2_MESSAGE_TYPE_ERROR)
        {
            LogError(GLogger, std::format(L"FSR2_API_DEBUG_ERROR: {}", message));
        }
        else if (type == FFX_FSR2_MESSAGE_TYPE_WARNING)
        {
            LogWarning(GLogger, std::format(L"FSR2_API_DEBUG_WARNING: {}", message));
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

    class VulkanRenderBackend;
    class VulkanCommandBufferManager;

    struct VulkanPushConstants
    {
        int32 indices[16];
        float data[16];

        VulkanPushConstants()
        {
            for (uint32 i = 0; i < 16; i++)
            {
                indices[i] = -1;
            }
            memset(data, 0, sizeof(data));
        }
    };

    struct VulkanPushConstantsTest
    {
        int32 indices[16];
        uint8 data[64];

        VulkanPushConstantsTest()
        {
            for (uint32 i = 0; i < 16; i++)
            {
                indices[i] = -1;
            }
            memset(data, 0, 64);
        }
    };

    struct VulkanRenderBackendHandleManager
    {
        std::vector<uint32> freeIndices;
        uint32 nextIndex;
        template <typename HandleType>
        HandleType Allocate(uint32 deviceMask)
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

    enum
    {
        BindlessBindingSamplers               = 0,
        BindlessBindingSampledImages          = 1,
        BindlessBindingStroageImages          = 2,
        BindlessBindingStroageBuffers         = 3,
        BindlessBindingAccelerationStructures = 4,
    };

    struct VulkanBindlessConfig
    {
        uint32 numSamplers;
        uint32 numSampledImages;
        uint32 numStorageImages;
        uint32 numStorageBuffers;
        uint32 numAccelerationStructures;
    };

    struct VulkanBindlessDescriptorManager
    {
        VulkanBindlessConfig config;

        VkDescriptorPool pool;
        VkDescriptorSetLayout layout;
        VkDescriptorSet set;

        uint32 pushConstantSize;

        VkPipelineLayout compatibleGraphicsPipelineLayout;
        VkPipelineLayout compatibleComputePipelineLayout;
        VkPipelineLayout compatibleRayTracingPipelineLayout;

        std::vector<uint32> freeSampledImages;
        std::vector<uint32> freeSamplers;
        std::vector<uint32> freeStorageImages;
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

        uint32 AllocateAccelerationStructureIndex()
        {
            uint32 index = freeAccelerationStructures.back();
            freeAccelerationStructures.pop_back();
            return index;
        }
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

        VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures;
        VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures;
        VkPhysicalDeviceSynchronization2Features synchronization2Features;
        VkPhysicalDeviceMaintenance4Features maintenance4Features;
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures;
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures;
        VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;
        VkPhysicalDeviceHostQueryResetFeatures hostQueryResetFeatures;
        VkPhysicalDeviceShaderFloat16Int8FeaturesKHR shaderFloat16Int8Features;
        VkPhysicalDevice16BitStorageFeaturesKHR float16StorageFeatures;
        VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures;
        VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timelineSemaphoreFeatures;
        VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR fragmentShaderBarycentricFeatures;
        VkPhysicalDeviceMultiviewFeatures multiviewFeatures;
        VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures shaderDemoteToHelperInvocationFeatures;
        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT;

        void* featuresEntry;
        VkPhysicalDeviceFeatures enabledFeatures;
        std::vector<VkLayerProperties> layerProperties;
        std::vector<VkExtensionProperties> extensionProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        uint32 queueFamilyIndices[RENDER_BACKEND_NUM_QUEUE_FAMILIES];
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
        uint32                     semaphoreIndex;
        RenderBackendTextureHandle buffers[RenderBackendMaxNumSwapChainBuffers];
        uint32                     numSemaphores;
        VkFence                    imageAcquiredFences[RenderBackendMaxNumSwapChainBuffers + 1];
        VkSemaphore                imageAcquiredSemaphores[RenderBackendMaxNumSwapChainBuffers + 1];
    };

    struct VulkanCpuReadbackBuffer
    {
        VkBuffer handle;
        uint64 mipOffsets[RenderBackendMaxNumTextureMipLevels];
        uint64 mipSize[RenderBackendMaxNumTextureMipLevels];
        void* data;
    };

    struct VulkanTexture
    {
        VkImage handle;
        bool swapchainBuffer;
        VmaAllocation allocation;
        VkDeviceMemory deivceMemory;
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
        VkImageView srv = VK_NULL_HANDLE;
        std::vector<VkImageView> rtv;
        std::vector<VkImageView> dsv;
        int32 srvIndex = -1;

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
        VkBufferUsageFlags usageFlags;
        VmaAllocationCreateFlags allocationFlags;
        VmaMemoryUsage memeryUsage;
        bool createMapped;
        bool mapped;
        void* mappedData;
        int32 uavIndex;
        std::string name;
        VulkanRayTracingShaderBindingTable* shaderBindingTable;
        VkDeviceAddress deviceAddress;
    };

    struct VulkanGraphicsPipelineStateDesc
    {
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[RenderBackendMaxNumSimultaneousColorRenderTargets];
    };

    struct VulkanShader
    {
        uint32 numStages;
        VkPipelineShaderStageCreateInfo stages[RenderBackendMaxNumShaderStages];
        std::string entryPoints[RenderBackendMaxNumShaderStages];
    };

    struct VulkanShaderCompiler
    {
        MemoryArena* allocator;
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
        std::vector<Buffer> resourceBuffers;
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        union
        {
            RenderBackendRayTracingBottomLevelAccelerationDesc blasDesc;
            RenderBackendRayTracingTopLevelAccelerationDesc tlasDesc;
        };
        uint32 descriptorIndex;
    };

    struct VulkanRenderingInfo
    {
        bool hasDepthStencil;
        VkExtent3D extent;
        VkRenderingInfo renderingInfo;
        uint32 numColorAttachments;
        VkFormat colorAttachmentFormats[RenderBackendMaxNumSimultaneousColorRenderTargets];
        VkRenderingAttachmentInfo colorAttachments[RenderBackendMaxNumSimultaneousColorRenderTargets];
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
        uint32 numAttachments;
        uint32 numColorAttachments;
        VkImage images[RenderBackendMaxNumSimultaneousColorRenderTargets + 1];
        VkImageView attachments[RenderBackendMaxNumSimultaneousColorRenderTargets + 1];
    };

    struct VulkanRenderPassDesc
    {
        VkExtent3D extent;
        bool hasDepthStencil;
        uint32 renderPassCompatibleHash;
        uint32 renderPassFullHash;
        uint32 numAttachmentDescriptions;
        uint32 numColorAttachments;
        VkAttachmentDescription attachmentDescriptions[RenderBackendMaxNumSimultaneousColorRenderTargets + 1];
        VkAttachmentReference colorReferences[RenderBackendMaxNumSimultaneousColorRenderTargets];
        VkAttachmentReference depthStencilReference;
        VkImageLayout depthStencilLayout;
    };

    struct VulkanTimingQueryHeap
    {
        VkQueryPool handle;
        uint32 maxQueryCount;
    };

    struct VulkanOcclusionQueryHeap
    {
        VkQueryPool handle;
        uint32 maxQueryCount;
    };

    struct RenderPassCompatibleHashInfo
    {
        uint8 numAttachments;
        VkFormat formats[RenderBackendMaxNumSimultaneousColorRenderTargets + 1];
    };

    struct RenderPassFullHashInfo
    {
        /** +2 : 1 for depth, 1 for stencil. */
        uint8 loadOps[RenderBackendMaxNumSimultaneousColorRenderTargets + 2];
        uint8 storeOps[RenderBackendMaxNumSimultaneousColorRenderTargets + 2];
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
        uint32 CreateBuffer(const RenderBackendBufferDesc* desc, const void* data, const char* name);
        void DestroyBuffer(uint32 index);
        void ResizeBuffer(uint32 index, uint64 size);
        uint64 GetBufferDeviceAddress(RenderBackendBufferHandle bufferHandle);
        void* MapBuffer(uint32 index);
        void UnmapBuffer(uint32 index);
        uint32 CreateTexture(const RenderBackendTextureDesc* desc, const void* data, const char* name);
        void DestroyTexture(uint32 index);
        uint32 CreateTextureSRV(uint32 textureIndex, const RenderBackendTextureSRVDesc* desc, const char* name);
        int32 GetTextureSRVDescriptorIndex(uint32 textureIndex);
        uint32 CreateTextureUAV(uint32 textureIndex, const RenderBackendTextureUAVDesc* desc, const char* name);
        int32 GetTextureUAVDescriptorIndex(uint32 textureIndex, uint32 mipLevel);
        uint32 CreateSampler(const RenderBackendSamplerDesc* desc, const char* name);
        void DestroySampler(uint32 index);
        uint32 CreateShader(const RenderBackendShaderDesc* desc, const char* name);
        void DestroyShader(uint32);
        uint32 CreateBottomLevelAS(const RenderBackendRayTracingBottomLevelAccelerationDesc* desc, const char* name);
        uint32 CreateTopLevelAS(const RenderBackendRayTracingTopLevelAccelerationDesc* desc, const char* name);
        VkRenderPass FindOrCreateRenderPass(const VulkanRenderPassDesc& renderPassDesc);
        VulkanFramebuffer* FindOrCreateFramebuffer(const RenderBackendRenderPassInfo& renderPassInfo, const VulkanRenderPassDesc& renderPassDesc, VkRenderPass renderPass);
        VkPipelineLayout FindOrCreatePipelineLayout(uint32 pushConstantSize, RenderBackendPipelineType pipelineType);
        VulkanPipeline* FindOrCreateComputePipeline(VulkanShader* shader, uint32 pushConstantSize);
        VulkanPipeline* FindOrCreateRayTracingPipeline(VulkanShader* shader, uint32 pushConstantSize);
        VulkanPipeline* FindOrCreateGraphicsPipeline(
            VulkanShader* shader,
            const RenderBackendGraphicsPipelineState& pipelineState,
            uint32 pushConstantSize,
            RenderBackendPrimitiveTopology topology,
            bool useDynamicRendering,
            VulkanRenderingInfo* renderingInfo,
            VkRenderPass renderPass,
            uint32 activeColorAttachmentCount);
        void SetDebugUtilsObjectName(VkObjectType type, uint64 handle, const char* name);

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
        inline VulkanRayTracingAccelerationStructure* GetAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle handle)
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
        std::vector<VkSemaphore> presentSemaphores;
        VulkanCommandBufferManager* commandBufferManager;
        std::vector<VulkanSwapchain> swapchains;
        void CreateVmaAllocator();
        void DestroyVmaAllocator();
        bool CreateBindlessDescriptorManager(const VulkanBindlessConfig& bindlessConfig);
        void DestroyBindlessDescriptorManager();
        void CreateDefaultResources();
        uint32 CreateAccelerationStructure(VulkanRayTracingAccelerationStructure* accelerationStructure, VkAccelerationStructureTypeKHR type, uint32* primitiveCounts, const char* name);

        void BindBindlessDescriptorSets(VkCommandBuffer commandBuffer);

        MemoryArena*          allocator;
        VulkanRenderBackend*  backend;
        VulkanPhysicalDevice* physicalDevice;
        VkInstance            instance;
        VkDevice              handle;
        VmaAllocator          vmaAllocator;
        uint32                deviceMask;

        bool fsr2EnableDebugCheck = true;
        FfxFsr2ContextDescription fsr2InitializationParameters = {};
        FfxFsr2Context fsr2Context;

        std::vector<const char*> enabledDeviceExtensions;
        std::vector<const char*> enabledValidationLayers;

        uint32 numCommandQueues[RENDER_BACKEND_NUM_QUEUE_FAMILIES] = { 1, 1, 1, 1 };
        std::vector<VulkanQueue> commandQueues[RENDER_BACKEND_NUM_QUEUE_FAMILIES];

        RenderBackendSamplerHandle defaultSampler;
        RenderBackendBufferHandle defaultStorageBuffer;

        VulkanBindlessDescriptorManager bindlessDescriptorManager;
        VulkanPipelineManager pipelineManager;

        struct FramebufferList
        {
            std::vector<VulkanFramebuffer> framebuffers;
        };
        std::map<uint32, FramebufferList> cachedFramebuffers;
        std::map<uint32, VkRenderPass> cachedRenderPasses;

        template<typename ResourceType>
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
        ResourceContainer<VulkanOcclusionQueryHeap> occlusionQueryHeaps;

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
            VK_CHECK(vkCreateCommandPool(device->GetHandle(), &poolInfo, VULKAN_ALLOCATION_CALLBACKS, &pool));
        }
        ~VulkanCommandBufferManager()
        {
            vkDestroyCommandPool(device->GetHandle(), pool, VULKAN_ALLOCATION_CALLBACKS);
            pool = VK_NULL_HANDLE;
        }
        inline VkCommandPool GetCommandPoolHandle() const
        {
            return pool;
        }
        VulkanCommandBuffer* AllocateCommandBuffer()
        {
            VulkanCommandBuffer commandBuffer;
            VkCommandBufferAllocateInfo allocateInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_CHECK(vkAllocateCommandBuffers(device->GetHandle(), &allocateInfo, &commandBuffer.handle));
            VkFenceCreateInfo fenceInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };
            VK_CHECK(vkCreateFence(device->GetHandle(), &fenceInfo, VULKAN_ALLOCATION_CALLBACKS, &commandBuffer.fence));
            VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                .pNext = nullptr,
                .semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
                .initialValue = 0,
            };
            VkSemaphoreCreateInfo semaphoreCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = &semaphoreTypeCreateInfo,
                .flags = 0,
            };
            VK_CHECK(vkCreateSemaphore(device->GetHandle(), &semaphoreCreateInfo, VULKAN_ALLOCATION_CALLBACKS, &commandBuffer.semaphore));
            commandBuffers.emplace_back(commandBuffer);
            return &commandBuffers.back();
        }
        VulkanCommandBuffer* PrepareForNextCommandBuffer()
        {
            for (VulkanCommandBuffer& commandBuffer : commandBuffers)
            {
                VkResult result = vkGetFenceStatus(device->GetHandle(), commandBuffer.fence);
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

    class VulkanRenderBackend : public RenderBackend
    {
    public:

        RenderBackendType GetType() const override
        {
            return RenderBackendType::Vulkan;
        }

        bool Init(int flags);
        void Exit();
        void EnumeratePhysicalDevices();
        bool IsInstanceExtensionEnabled(const char* extension);

        struct VulkanFunctions
        {
            // KHR
            PFN_vkGetBufferDeviceAddressKHR                vkGetBufferDeviceAddressKHR;
            PFN_vkCreateAccelerationStructureKHR           vkCreateAccelerationStructureKHR;
            PFN_vkDestroyAccelerationStructureKHR          vkDestroyAccelerationStructureKHR;
            PFN_vkGetAccelerationStructureBuildSizesKHR    vkGetAccelerationStructureBuildSizesKHR;
            PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
            PFN_vkGetRayTracingShaderGroupHandlesKHR       vkGetRayTracingShaderGroupHandlesKHR;
            PFN_vkBuildAccelerationStructuresKHR           vkBuildAccelerationStructuresKHR;
            PFN_vkCreateRayTracingPipelinesKHR             vkCreateRayTracingPipelinesKHR;
            PFN_vkCmdPipelineBarrier2KHR                   vkCmdPipelineBarrier2KHR;
            PFN_vkCmdBuildAccelerationStructuresKHR        vkCmdBuildAccelerationStructuresKHR;
            PFN_vkCmdTraceRaysKHR                          vkCmdTraceRaysKHR;
            // EXT
            PFN_vkCmdDrawMeshTasksEXT                      vkCmdDrawMeshTasksEXT;
            PFN_vkCmdDrawMeshTasksIndirectEXT              vkCmdDrawMeshTasksIndirectEXT;
            PFN_vkSetDebugUtilsObjectNameEXT               vkSetDebugUtilsObjectNameEXT = VK_NULL_HANDLE;
            PFN_vkCmdBeginDebugUtilsLabelEXT               vkCmdBeginDebugUtilsLabelEXT = VK_NULL_HANDLE;
            PFN_vkCmdEndDebugUtilsLabelEXT                 vkCmdEndDebugUtilsLabelEXT   = VK_NULL_HANDLE;
        };
        VulkanFunctions functions;

        void Tick() override;
        void CreateRenderDevices(PhysicalDeviceID* physicalDeviceIDs, uint32 numDevices, uint32* outDeviceMasks) override;
        void DestroyRenderDevices() override;
        void FlushRenderDevices() override;
        RenderBackendSwapChainHandle CreateSwapChain(uint32 deviceMask, const RenderBackendSwapChainDesc* desc) override;
        void DestroySwapChain(RenderBackendSwapChainHandle swapChain) override;
        void ResizeSwapChain(RenderBackendSwapChainHandle swapChain, uint32* width, uint32* height) override;
        bool PresentSwapChain(RenderBackendSwapChainHandle swapChain) override;
        RenderBackendTextureHandle GetActiveSwapChainBuffer(RenderBackendSwapChainHandle swapChain) override;
        RenderBackendBufferHandle CreateBuffer(uint32 deviceMask, const RenderBackendBufferDesc* desc, const void* data, const char* name) override;
        void ResizeBuffer(RenderBackendBufferHandle buffer, uint64 size) override;
        void MapBuffer(RenderBackendBufferHandle buffer, void** data) override;
        void UnmapBuffer(RenderBackendBufferHandle buffer) override;
        void UpdateBuffer(RenderBackendBufferHandle buffer, uint64 offset, const void* data, uint64 size) override;
        void DestroyBuffer(RenderBackendBufferHandle buffer) override;
        RenderBackendTextureHandle CreateTexture(uint32 deviceMask, const RenderBackendTextureDesc* desc, const void* data, const char* name) override;
        void DestroyTexture(RenderBackendTextureHandle texture) override;
        void UploadTexture(RenderBackendTextureHandle handle, const RenderBackendTextureUploadDataDesc& data) override;
        void GetTextureReadbackData(RenderBackendTextureHandle texture, void** data) override;
        RenderBackendTextureSRVHandle CreateTextureSRV(uint32 deviceMask, const RenderBackendTextureSRVDesc* desc, const char* name) override;
        RenderBackendTextureUAVHandle CreateTextureUAV(uint32 deviceMask, const RenderBackendTextureUAVDesc* desc, const char* name) override;
        int32 GetTextureSRVDescriptorIndex(uint32 deviceMask, RenderBackendTextureHandle srv) override;
        int32 GetTextureUAVDescriptorIndex(uint32 deviceMask, RenderBackendTextureHandle uav) override;
        int32 GetBufferUAVDescriptorIndex(uint32 deviceMask, RenderBackendBufferHandle uav) override;
        RenderBackendSamplerHandle CreateSampler(uint32 deviceMask, const RenderBackendSamplerDesc* desc, const char* name) override;
        void DestroySampler(RenderBackendSamplerHandle sampler) override;
        RenderBackendShaderHandle CreateShader(uint32 deviceMask, const RenderBackendShaderDesc* desc, const char* name) override;
        void DestroyShader(RenderBackendShaderHandle shader) override;
        RenderBackendTimingQueryHeapHandle CreateTimingQueryHeap(uint32 deviceMask, const RenderBackendTimingQueryHeapDesc* desc, const char* name) override;
        void DestroyTimingQueryHeap(RenderBackendTimingQueryHeapHandle timingQueryHeap) override;
        RenderBackendOcclusionQueryHeapHandle CreateOcclusionQueryHeap(uint32 deviceMask, const RenderBackendOcclusionQueryHeapDesc* desc, const char* name) override;
        void DestroyOcclusionQueryHeap(RenderBackendOcclusionQueryHeapHandle occlusionQueryHeap) override;
        void SubmitCommandLists(RenderBackendCommandList** commandLists, uint32 numCommandLists, RenderBackendSwapChainHandle swapChain) override;
        RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingBottomLevelAccelerationStructure(uint32 deviceMask, const RenderBackendRayTracingBottomLevelAccelerationDesc* desc, const char* name) override;
        RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingTopLevelAccelerationStructure(uint32 deviceMask, const RenderBackendRayTracingTopLevelAccelerationDesc* desc, const char* name) override;
        RenderBackendRayTracingPipelineStateHandle CreateRayTracingPipelineState(uint32 deviceMask, const RenderBackendRayTracingPipelineStateDesc* desc, const char* name) override;
        RenderBackendBufferHandle CreateRayTracingShaderBindingTable(uint32 deviceMask, const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name) override;

        VkInstance instance;
        std::vector<const char*> enabledInstanceLayers;
        std::vector<const char*> enabledInstanceExtensions;
        VkDebugUtilsMessengerEXT debugUtilsMessenger;
        uint32 numDevices;
        VulkanDevice devices[RenderBackendMaxNumDevices];
        VulkanPhysicalDevice availablePhysicalDevices[RenderBackendMaxNumDevices];
        VulkanRenderBackendHandleManager handleManager;

        bool enableRayTracingSupport = false;
    };

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData)
    {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
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
        for (const auto& supportedExtension : supportedExtensions)
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
        for (const auto& supportedLayer : supportedLayers)
        {
            if (strcmp(name, supportedLayer.layerName) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool VulkanRenderBackend::IsInstanceExtensionEnabled(const char* extension)
    {
        for (const auto& enabledExtension : enabledInstanceExtensions)
        {
            if (strcmp(extension, enabledExtension) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void VulkanRenderBackend::EnumeratePhysicalDevices()
    {
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &numDevices, 0));
        if (numDevices == 0)
        {
            LogInfo(GLogger, std::format("No available physical device.\n"));
            return;
        }
        VkPhysicalDevice physicalDeviceHandles[RenderBackendMaxNumDevices];
        assert(numDevices < RenderBackendMaxNumDevices);
        vkEnumeratePhysicalDevices(instance, &numDevices, physicalDeviceHandles);

        for (uint32 index = 0; index < numDevices; index++)
        {
            VulkanPhysicalDevice& physicalDevice = availablePhysicalDevices[index];

            physicalDevice.handle = physicalDeviceHandles[index];

            VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = &physicalDevice.descriptorIndexingProperties,
            };
            physicalDevice.descriptorIndexingProperties = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT,
                .pNext = &physicalDevice.rayTracingPipelineProperties
            };
            physicalDevice.rayTracingPipelineProperties = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
                .pNext = &physicalDevice.accelerationStructureProperties,
            };
            physicalDevice.accelerationStructureProperties = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR,
                .pNext = &physicalDevice.fragmentShaderBarycentricProperties,
            };
            physicalDevice.fragmentShaderBarycentricProperties = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR,
                .pNext = nullptr,
            };
            vkGetPhysicalDeviceProperties2(physicalDevice.handle, &physicalDeviceProperties2);
            physicalDevice.properties = physicalDeviceProperties2.properties;

            vkGetPhysicalDeviceMemoryProperties(physicalDevice.handle, &physicalDevice.memoryProperties);

            // Ray tracing features
            if (enableRayTracingSupport)
            {
                physicalDevice.accelerationStructureFeatures = {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
                    .pNext = &physicalDevice.rayTracingPipelineFeatures
                };
                physicalDevice.rayTracingPipelineFeatures = {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
                    .pNext = &physicalDevice.rayQueryFeatures
                };
                physicalDevice.rayQueryFeatures = {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
                    .pNext = &physicalDevice.bufferDeviceAddressFeatures
                };
            }

            physicalDevice.bufferDeviceAddressFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
                .pNext = &physicalDevice.descriptorIndexingFeatures
            };
            physicalDevice.descriptorIndexingFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
                .pNext = &physicalDevice.dynamicRenderingFeatures
            };
            physicalDevice.dynamicRenderingFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
                .pNext = &physicalDevice.timelineSemaphoreFeatures
            };
            physicalDevice.timelineSemaphoreFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
                .pNext = &physicalDevice.synchronization2Features
            };
            physicalDevice.synchronization2Features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
                .pNext = &physicalDevice.maintenance4Features
            };
            physicalDevice.maintenance4Features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES,
                .pNext = &physicalDevice.float16StorageFeatures
            };
            physicalDevice.float16StorageFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR,
                .pNext = &physicalDevice.shaderFloat16Int8Features
            };
            physicalDevice.shaderFloat16Int8Features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES,
                .pNext = &physicalDevice.hostQueryResetFeatures,
            };
            physicalDevice.hostQueryResetFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES,
                .pNext = &physicalDevice.fragmentShaderBarycentricFeatures,
            };
            physicalDevice.fragmentShaderBarycentricFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR,
                .pNext = &physicalDevice.multiviewFeatures,
            };
            physicalDevice.multiviewFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES,
                .pNext = &physicalDevice.shaderDemoteToHelperInvocationFeatures,
            };
            physicalDevice.shaderDemoteToHelperInvocationFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES,
                .pNext = &physicalDevice.meshShaderFeaturesEXT,
            };
            physicalDevice.meshShaderFeaturesEXT = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
                .pNext = nullptr,
            };

            if (enableRayTracingSupport)
            {
                physicalDevice.featuresEntry = (void*)&physicalDevice.accelerationStructureFeatures;
            }
            else
            {
                physicalDevice.featuresEntry = (void*)&physicalDevice.bufferDeviceAddressFeatures;
            }

            VkPhysicalDeviceFeatures2 deviceFeatures2 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = physicalDevice.featuresEntry
            };
            vkGetPhysicalDeviceFeatures2(physicalDevice.handle, &deviceFeatures2);
            physicalDevice.enabledFeatures = deviceFeatures2.features;

            // TODO
            physicalDevice.meshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader = false;

            LogInfo(GLogger, std::format("Found physical device (name: {}, type: {}, vendor id: {}, device id: {}, support vulkan version: {}.{}.{})",
                physicalDevice.properties.deviceName,
                (int32)physicalDevice.properties.deviceType,
                physicalDevice.properties.vendorID,
                physicalDevice.properties.deviceID,
                VK_API_VERSION_MAJOR(physicalDevice.properties.apiVersion),
                VK_API_VERSION_MINOR(physicalDevice.properties.apiVersion),
                VK_API_VERSION_PATCH(physicalDevice.properties.apiVersion)
            ));

            uint32 numQueueFamilyProperties;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.handle, &numQueueFamilyProperties, 0);
            physicalDevice.queueFamilyProperties.resize(numQueueFamilyProperties);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.handle, &numQueueFamilyProperties, physicalDevice.queueFamilyProperties.data());

            uint32& graphicsQueueFamilyIndex = physicalDevice.queueFamilyIndices[(uint32)RenderBackendQueueFamily::Graphics] = ~uint32(0);
            uint32& computeQueueFamilyIndex = physicalDevice.queueFamilyIndices[(uint32)RenderBackendQueueFamily::Compute] = ~uint32(0);
            uint32& transferQueueFamilyIndex = physicalDevice.queueFamilyIndices[(uint32)RenderBackendQueueFamily::Copy] = ~uint32(0);
            uint32& opticalFlowQueueFamilyIndex = physicalDevice.queueFamilyIndices[(uint32)RenderBackendQueueFamily::OpticalFlow] = ~uint32(0);

            for (uint32 i = 0; i < numQueueFamilyProperties; i++)
            {
                VkQueueFlags queueFlags = physicalDevice.queueFamilyProperties[i].queueFlags;
                if ((queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && graphicsQueueFamilyIndex == ~uint32(0))
                {
                    graphicsQueueFamilyIndex = i;
                }
                else if ((queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 && computeQueueFamilyIndex == ~uint32(0))
                {
                    computeQueueFamilyIndex = i;
                }
                else if ((queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 && transferQueueFamilyIndex == ~uint32(0))
                {
                    transferQueueFamilyIndex = i;
                }
                else if ((queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) != 0 && opticalFlowQueueFamilyIndex == ~uint32(0))
                {
                    opticalFlowQueueFamilyIndex = i;
                }
            }

            uint32 numLayerProperties = 0;
            VK_CHECK(vkEnumerateDeviceLayerProperties(physicalDevice.handle, &numLayerProperties, nullptr));
            physicalDevice.layerProperties.resize(numLayerProperties);
            VK_CHECK(vkEnumerateDeviceLayerProperties(physicalDevice.handle, &numLayerProperties, physicalDevice.layerProperties.data()));
            for (const auto& layerProperties : physicalDevice.layerProperties)
            {
                LogInfo(GLogger, std::format("Available device layer: {} - vulkan apid version: {}.{}.{} - implemetation version: {} - description: {}.",
                    layerProperties.layerName,
                    VK_API_VERSION_MAJOR(layerProperties.specVersion),
                    VK_API_VERSION_MINOR(layerProperties.specVersion),
                    VK_API_VERSION_PATCH(layerProperties.specVersion),
                    layerProperties.implementationVersion,
                    layerProperties.description
                ));
            }

            uint32 numExtensionProperties = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice.handle, nullptr, &numExtensionProperties, nullptr));
            physicalDevice.extensionProperties.resize(numExtensionProperties);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice.handle, nullptr, &numExtensionProperties, physicalDevice.extensionProperties.data()));
            for (const auto& extensionProperty : physicalDevice.extensionProperties)
            {
                LogInfo(GLogger, std::format("Available device extension: {} - extension version: {}.",
                    extensionProperty.extensionName,
                    extensionProperty.specVersion
                ));
            }
        }
    }

    bool VulkanRenderBackend::Init(int flags)
    {
#if HE_ENBALE_STREAMLINE_SUPPORT
        // Init Streamline
        {
            sl::Feature features[] = { sl::kFeatureDLSS, sl::kFeatureDLSS_G, sl::kFeatureReflex };

            sl::Preferences pref = {};
            pref.showConsole = true;
            pref.logLevel = sl::LogLevel::eDefault;
            pref.pathsToPlugins = nullptr;
            pref.numPathsToPlugins = 0;
            pref.pathToLogsAndData = nullptr;
            pref.allocateCallback = nullptr;
            pref.releaseCallback = nullptr;
            pref.logMessageCallback = StreamlineLogMessageCallback;
            pref.flags = sl::PreferenceFlags::eDisableCLStateTracking;// | sl::PreferenceFlags::eAllowOTA;
            pref.featuresToLoad = features;
            pref.numFeaturesToLoad = _countof(features);
            pref.applicationId = sl::INVALID_UINT;
            pref.engine = sl::EngineType::eCustom;
            pref.engineVersion = "Horizon Engine";
            pref.projectId = "a0f57b54-1daf-4934-90ae-c4035c19df04";
            pref.renderAPI = sl::RenderAPI::eVulkan;

            sl::Result result;
            if (SL_FAILED(result, slInit(pref, sl::kSDKVersion)))
            {
                LogInfo(GLogger, std::format("slInit, error code: {}.", (int32)result));
            }

            //{
            //    sl::FeatureRequirements requirements = {};
            //    if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureNRD, requirements)))
            //    {
            //        LogInfo(GLogger, std::format("slGetFeatureRequirements, error code: {}.", (int32)result));
            //    }
            //    else
            //    {
            //        // Feature is loaded, we can check the requirements
            //        assert(requirements.flags & sl::FeatureRequirementFlags::eVulkanSupported);

            //        for (uint32 i = 0; i < requirements.vkNumInstanceExtensions; i++)
            //        {
            //            printf("%s\n", requirements.vkInstanceExtensions[i]);
            //        }
            //        for (uint32 i = 0; i < requirements.vkNumDeviceExtensions; i++)
            //        {
            //            printf("%s\n", requirements.vkDeviceExtensions[i]);
            //        }
            //    }
            //}

            {
                sl::FeatureRequirements requirements = {};
                if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureDLSS, requirements)))
                {
                    LogInfo(GLogger, std::format("slGetFeatureRequirements, error code: {}.", (int32)result));
                }
                else
                {
                    // Feature is loaded, we can check the requirements
                    assert(requirements.flags & sl::FeatureRequirementFlags::eVulkanSupported);

                    for (uint32 i = 0; i < requirements.vkNumInstanceExtensions; i++)
                    {
                        printf("%s\n", requirements.vkInstanceExtensions[i]);
                    }
                    for (uint32 i = 0; i < requirements.vkNumDeviceExtensions; i++)
                    {
                        printf("%s\n", requirements.vkDeviceExtensions[i]);
                    }
                }
            }

            {
                sl::FeatureRequirements requirements = {};
                if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureDLSS_G, requirements)))
                {
                    LogInfo(GLogger, std::format("slGetFeatureRequirements, error code: {}.", (int32)result));
                }
                else
                {
                    // Feature is loaded, we can check the requirements
                    assert(requirements.flags & sl::FeatureRequirementFlags::eVulkanSupported);

                    for (uint32 i = 0; i < requirements.vkNumInstanceExtensions; i++)
                    {
                        printf("%s\n", requirements.vkInstanceExtensions[i]);
                    }
                    for (uint32 i = 0; i < requirements.vkNumDeviceExtensions; i++)
                    {
                        printf("%s\n", requirements.vkDeviceExtensions[i]);
                    }
                }
            }

            {
                sl::FeatureRequirements requirements = {};
                if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureReflex, requirements)))
                {
                    LogInfo(GLogger, std::format("slGetFeatureRequirements, error code: {}.", (int32)result));
                }
                else
                {
                    // Feature is loaded, we can check the requirements
                    assert(requirements.flags & sl::FeatureRequirementFlags::eVulkanSupported);

                    for (uint32 i = 0; i < requirements.vkNumInstanceExtensions; i++)
                    {
                        printf("%s\n", requirements.vkInstanceExtensions[i]);
                    }
                    for (uint32 i = 0; i < requirements.vkNumDeviceExtensions; i++)
                    {
                        printf("%s\n", requirements.vkDeviceExtensions[i]);
                    }
                }
            }

            {
                sl::ReflexState state = {};
                if (SL_FAILED(result, slReflexGetState(state)))
                {
                    LogInfo(GLogger, std::format("slReflexGetState, error code: {}", (int32)result));
                }
                if (state.lowLatencyAvailable)
                {
                    //
                    // Reflex Low Latency is available, on NVDA hardware this would be done through Reflex.
                    //
                    // The application can show the Reflex Low Latency UI. (Otherwise hide/disable the UI.)
                    // This is for UI only. Do everything else the same, even when this is false.
                    //
                }
                if (state.flashIndicatorDriverControlled)
                {
                    //
                    // Reflex Flash Indicator (RFI) is controlled by the driver. This means
                    // the application should always check for left mouse button clicks and
                    // send the trigger flash markers accordingly. The driver will decide
                    // whether to show the RFI on screen based on user preference.
                    //
                }
            }
            //// We are using NULL adapter on purpose
            //sl::AdapterInfo adapterInfo = {};
            //if (SL_FAILED(result, slIsFeatureSupported(sl::Feature::eDLSS, adapterInfo)))
            //{
            //    // Requested feature is not supported, let's see why
            //    switch (result)
            //    {
            //    case sl::Result::eErrorOSOutOfDate:              // inform user to update OS
            //    case sl::Result::eErrorDriverOutOfDate:          // inform user to update driver
            //    case sl::Result::eErrorNoSupportedAdapterFound:  // cannot use any available adapter
            //        // and so on ...
            //    };
            //}
            //else
            //{
            //    // Feature is supported on at least one adapter so now we need to figure out which one before we create our device.
            //}
        }
#endif

        bool enableValidationLayers = flags & VULKAN_RENDER_BACKEND_CREATE_FLAGS_VALIDATION_LAYERS;

        std::vector<const char*> requiredInstanceLayers;
        if (enableValidationLayers)
        {
            requiredInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");
            requiredInstanceLayers.push_back("VK_LAYER_KHRONOS_synchronization2");

            //requiredInstanceLayers.push_back("VK_LAYER_LUNARG_api_dump");
            //requiredInstanceLayers.push_back("VK_LAYER_LUNARG_device_simulation");
            //requiredInstanceLayers.push_back("VK_LAYER_LUNARG_gfxreconstruct");
            //requiredInstanceLayers.push_back("VK_LAYER_LUNARG_monitor");
            // requiredInstanceLayers.push_back("VK_LAYER_KHRONOS_profiles");

            //requiredInstanceLayers.push_back("VK_LAYER_NV_nomad_release_public_2022_1_1");
            //requiredInstanceLayers.push_back("VK_LAYER_NV_GPU_Trace_release_public_2022_1_1");
            requiredInstanceLayers.push_back("VK_LAYER_NV_optimus");
        }

        uint32 numInstanceLayerProperties = 0;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&numInstanceLayerProperties, nullptr));
        std::vector<VkLayerProperties> instanceLayerProperties(numInstanceLayerProperties);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&numInstanceLayerProperties, instanceLayerProperties.data()));
        for (const auto& instanceLayerProperty : instanceLayerProperties)
        {
            LogInfo(GLogger, std::format("Available instance layer: {} - vulkan api version: {}.{}.{} - implementation version: {} - description: {}.",
                instanceLayerProperty.layerName,
                VK_API_VERSION_MAJOR(instanceLayerProperty.specVersion),
                VK_API_VERSION_MINOR(instanceLayerProperty.specVersion),
                VK_API_VERSION_PATCH(instanceLayerProperty.specVersion),
                instanceLayerProperty.implementationVersion,
                instanceLayerProperty.description
            ));
        }

        for (const auto& requiredInstanceLayer : requiredInstanceLayers)
        {
            if (CheckInstanceLayerSupport(requiredInstanceLayer, instanceLayerProperties))
            {
                enabledInstanceLayers.push_back(requiredInstanceLayer);
                LogInfo(GLogger, std::format("Enabled instance layer: {}.", requiredInstanceLayer));
            }
            else
            {
                LogError(GLogger, std::format("Missing required instance layer: {}.", requiredInstanceLayer));
                return false;
            }
        }

        uint32 numInstanceExtensionProperties = 0;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensionProperties, nullptr));
        std::vector<VkExtensionProperties> instanceExtensionProperties(numInstanceExtensionProperties);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensionProperties, instanceExtensionProperties.data()));
        for (const auto& instanceExtensionProperty : instanceExtensionProperties)
        {
            LogInfo(GLogger, std::format("Available instance extension: {} - extension version: {}.",
                instanceExtensionProperty.extensionName,
                instanceExtensionProperty.specVersion
            ));
        }

        std::vector<const char*> requiredInstanceExtensions;
        if (enableValidationLayers)
        {
            // @see https://www.lunarg.com/wp-content/uploads/2018/05/Vulkan-Debug-Utils_05_18_v1.pdf
            requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        if (flags & VULKAN_RENDER_BACKEND_CREATE_FLAGS_SURFACE)
        {
            requiredInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            requiredInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            requiredInstanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
            requiredInstanceExtensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
        }
        requiredInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        requiredInstanceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
        requiredInstanceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
        requiredInstanceExtensions.push_back(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME);

        for (const auto& requiredInstanceExtension : requiredInstanceExtensions)
        {
            if (CheckInstanceExtensionSupport(requiredInstanceExtension, instanceExtensionProperties))
            {
                enabledInstanceExtensions.push_back(requiredInstanceExtension);
                LogInfo(GLogger, std::format("Enabled instance extension: {}.", requiredInstanceExtension));
            }
            else
            {
                LogError(GLogger, std::format("Missing required instance extension: {}.", requiredInstanceExtension));
                return false;
            }
        }

        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
            .pfnUserCallback = DebugUtilsMessengerCallback
        };

        VkApplicationInfo applicationInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            //.pApplicationName = ,
            //.applicationVersion = ,
            .pEngineName = HE_ENGINE_NAME,
            .engineVersion = HE_ENGINE_VERSION,
            .apiVersion = VK_API_VERSION_1_3,
        };
        VkInstanceCreateInfo instanceInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = enableValidationLayers ? &debugUtilsMessengerInfo : nullptr,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = (uint32)(enabledInstanceLayers.size()),
            .ppEnabledLayerNames = enabledInstanceLayers.data(),
            .enabledExtensionCount = (uint32)(enabledInstanceExtensions.size()),
            .ppEnabledExtensionNames = enabledInstanceExtensions.data()
        };

        VkResult result = vkCreateInstance(&instanceInfo, VULKAN_ALLOCATION_CALLBACKS, &instance);
        if (result != VK_SUCCESS)
        {
            return false;
        }

        if (enableValidationLayers)
        {
            PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
            VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerInfo, VULKAN_ALLOCATION_CALLBACKS, &debugUtilsMessenger));
        }
        else
        {
            debugUtilsMessenger = VK_NULL_HANDLE;
        }

        functions.vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetInstanceProcAddr(instance, "vkGetBufferDeviceAddressKHR"));
        functions.vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetInstanceProcAddr(instance, "vkCreateAccelerationStructureKHR"));
        functions.vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetInstanceProcAddr(instance, "vkDestroyAccelerationStructureKHR"));
        functions.vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureBuildSizesKHR"));
        functions.vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureDeviceAddressKHR"));
        functions.vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupHandlesKHR"));
        functions.vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetInstanceProcAddr(instance, "vkBuildAccelerationStructuresKHR"));
        functions.vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetInstanceProcAddr(instance, "vkCreateRayTracingPipelinesKHR"));
        functions.vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(vkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier2KHR"));
        functions.vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetInstanceProcAddr(instance, "vkCmdBuildAccelerationStructuresKHR"));
        functions.vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetInstanceProcAddr(instance, "vkCmdTraceRaysKHR"));

        functions.vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksEXT"));
        functions.vkCmdDrawMeshTasksIndirectEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksIndirectEXT>(vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectEXT"));
        functions.vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
        functions.vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
        functions.vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));

        enableRayTracingSupport = flags & VULKAN_RENDER_BACKEND_CREATE_FLAGS_RAY_TRACING;

        EnumeratePhysicalDevices();

        return true;
    }

    void VulkanRenderBackend::Exit()
    {
        vkDeviceWaitIdle(devices[0].handle);

#if HE_ENBALE_STREAMLINE_SUPPORT
        if (slShutdown() != sl::Result::eOk)
        {
            // Handle error, check the logs
        }
#endif

        if (debugUtilsMessenger != VK_NULL_HANDLE)
        {
            PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
            vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, VULKAN_ALLOCATION_CALLBACKS);
            debugUtilsMessenger = VK_NULL_HANDLE;
        }
        if (instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(instance, VULKAN_ALLOCATION_CALLBACKS);
            instance = VK_NULL_HANDLE;
        }
    }

    static void GetRenderPassDescAndClearValues(VulkanDevice* device, const RenderBackendRenderPassInfo& renderPassInfo, VkImageLayout depthStencilLayout, VulkanRenderPassDesc* outRenderPassDesc, VkClearValue* outClearValues)
    {
        RenderPassCompatibleHashInfo compatibleHashInfo = {};
        RenderPassFullHashInfo fullHashInfo = {};
        bool bSetExtent = true;

        for (uint32 index = 0; index < RenderBackendMaxNumSimultaneousColorRenderTargets; index++)
        {
            const RenderBackendRenderPassInfo::ColorRenderTarget& colorRenderTarget = renderPassInfo.colorRenderTargets[index];

            if (!colorRenderTarget.texture)
            {
                continue;
            }

            VulkanTexture* texture = device->GetTexture(colorRenderTarget.texture);
            uint32 mipLevel = renderPassInfo.colorRenderTargets[index].mipLevel;

            if (bSetExtent)
            {
                outRenderPassDesc->extent.width = std::max(1u, texture->width >> mipLevel);
                outRenderPassDesc->extent.height = std::max(1u, texture->height >> mipLevel);
                outRenderPassDesc->extent.depth = texture->depth;
                bSetExtent = false;
            }
            else
            {
                assert(outRenderPassDesc->extent.width == std::max(1u, texture->width >> mipLevel));
                assert(outRenderPassDesc->extent.height == std::max(1u, texture->height >> mipLevel));
                assert(outRenderPassDesc->extent.depth == texture->depth);
            }

            VkAttachmentDescription& attachmentDesc = outRenderPassDesc->attachmentDescriptions[outRenderPassDesc->numAttachmentDescriptions];
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.format = texture->format;
            attachmentDesc.loadOp = ConvertToVkAttachmentLoadOp(colorRenderTarget.loadOp);
            attachmentDesc.storeOp = ConvertToVkAttachmentStoreOp(colorRenderTarget.storeOp);
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference& colorReference = outRenderPassDesc->colorReferences[outRenderPassDesc->numColorAttachments];
            colorReference.attachment = outRenderPassDesc->numAttachmentDescriptions;
            colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            compatibleHashInfo.numAttachments++;
            compatibleHashInfo.formats[outRenderPassDesc->numColorAttachments] = attachmentDesc.format;
            fullHashInfo.loadOps[outRenderPassDesc->numColorAttachments] = attachmentDesc.loadOp;
            fullHashInfo.storeOps[outRenderPassDesc->numColorAttachments] = attachmentDesc.storeOp;

            outClearValues[outRenderPassDesc->numAttachmentDescriptions].color = texture->clearValue.color;
            outRenderPassDesc->numAttachmentDescriptions++;
            outRenderPassDesc->numColorAttachments++;
        }

        if (renderPassInfo.depthStencilRenderTarget.texture)
        {
            const RenderBackendRenderPassInfo::DepthStencilRenderTarget& depthStencilRenderTarget = renderPassInfo.depthStencilRenderTarget;
            VulkanTexture* texture = device->GetTexture(depthStencilRenderTarget.texture);

            if (bSetExtent)
            {
                outRenderPassDesc->extent.width = texture->width;
                outRenderPassDesc->extent.height = texture->height;
                outRenderPassDesc->extent.depth = 1;
                bSetExtent = false;
            }
            else
            {
                // Depth can be greater or equal to color. Clamp to the smaller size.
                outRenderPassDesc->extent.width = std::min(outRenderPassDesc->extent.width, texture->width);
                outRenderPassDesc->extent.height = std::min(outRenderPassDesc->extent.height, texture->height);
            }

            VkAttachmentDescription& attachmentDesc = outRenderPassDesc->attachmentDescriptions[outRenderPassDesc->numAttachmentDescriptions];
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.format = texture->format;
            attachmentDesc.loadOp = ConvertToVkAttachmentLoadOp(depthStencilRenderTarget.depthLoadOp);
            attachmentDesc.storeOp = ConvertToVkAttachmentStoreOp(depthStencilRenderTarget.depthStoreOp);
            attachmentDesc.stencilLoadOp = ConvertToVkAttachmentLoadOp(depthStencilRenderTarget.stencilLoadOp);
            attachmentDesc.stencilStoreOp = ConvertToVkAttachmentStoreOp(depthStencilRenderTarget.stencilStoreOp);
            attachmentDesc.initialLayout = depthStencilLayout;
            attachmentDesc.finalLayout = depthStencilLayout;

            outRenderPassDesc->depthStencilReference.attachment = outRenderPassDesc->numAttachmentDescriptions;
            outRenderPassDesc->depthStencilReference.layout = depthStencilLayout;

            compatibleHashInfo.formats[RenderBackendMaxNumSimultaneousColorRenderTargets] = attachmentDesc.format;
            fullHashInfo.loadOps[RenderBackendMaxNumSimultaneousColorRenderTargets] = attachmentDesc.loadOp;
            fullHashInfo.storeOps[RenderBackendMaxNumSimultaneousColorRenderTargets] = attachmentDesc.storeOp;
            fullHashInfo.loadOps[RenderBackendMaxNumSimultaneousColorRenderTargets + 1] = attachmentDesc.stencilLoadOp;
            fullHashInfo.storeOps[RenderBackendMaxNumSimultaneousColorRenderTargets + 1] = attachmentDesc.stencilStoreOp;

            outClearValues[outRenderPassDesc->numAttachmentDescriptions].depthStencil = texture->clearValue.depthStencil;
            outRenderPassDesc->hasDepthStencil = true;
            outRenderPassDesc->numAttachmentDescriptions++;
        }

        outRenderPassDesc->depthStencilLayout = depthStencilLayout;
        outRenderPassDesc->renderPassCompatibleHash = Crc32(&compatibleHashInfo, sizeof(compatibleHashInfo));
        outRenderPassDesc->renderPassFullHash = Crc32(&fullHashInfo, sizeof(fullHashInfo), outRenderPassDesc->renderPassCompatibleHash);
    }

    static void GetRenderingInfo(VulkanDevice* device, const RenderBackendRenderPassInfo& renderPassInfo, VulkanRenderingInfo* outRenderingInfo)
    {
        bool bSetExtent = true;

        memset(outRenderingInfo, 0, sizeof(VulkanRenderingInfo));

        for (uint32 index = 0; index < RenderBackendMaxNumSimultaneousColorRenderTargets; index++)
        {
            const RenderBackendRenderPassInfo::ColorRenderTarget& colorRenderTarget = renderPassInfo.colorRenderTargets[index];

            if (!colorRenderTarget.texture)
            {
                continue;
            }

            VulkanTexture* texture = device->GetTexture(colorRenderTarget.texture);

            uint32 mipLevel = renderPassInfo.colorRenderTargets[index].mipLevel;
            uint32 arrayLayer = renderPassInfo.colorRenderTargets[index].arrayLayer;

            if (bSetExtent)
            {
                outRenderingInfo->extent.width = std::max(1u, texture->width >> mipLevel);
                outRenderingInfo->extent.height = std::max(1u, texture->height >> mipLevel);
                outRenderingInfo->extent.depth = texture->depth;
                bSetExtent = false;
            }
            else
            {
                assert(outRenderingInfo->extent.width == std::max(1u, texture->width >> mipLevel));
                assert(outRenderingInfo->extent.height == std::max(1u, texture->height >> mipLevel));
                assert(outRenderingInfo->extent.depth == texture->depth);
            }

            VkRenderingAttachmentInfo& attachmentInfo = outRenderingInfo->colorAttachments[outRenderingInfo->numColorAttachments];
            attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachmentInfo.pNext = nullptr;
            attachmentInfo.imageView = texture->rtv[mipLevel]; // TODO: specify mip level
            attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            //attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            //attachmentInfo.resolveImageView = VK_NULL_HANDLE;
            //attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
            attachmentInfo.loadOp = ConvertToVkAttachmentLoadOp(colorRenderTarget.loadOp);
            attachmentInfo.storeOp = ConvertToVkAttachmentStoreOp(colorRenderTarget.storeOp);
            attachmentInfo.clearValue.color = texture->clearValue.color;

            outRenderingInfo->colorAttachmentFormats[outRenderingInfo->numColorAttachments] = texture->format;

            outRenderingInfo->numColorAttachments++;
        }

        if (renderPassInfo.depthStencilRenderTarget.texture)
        {
            const RenderBackendRenderPassInfo::DepthStencilRenderTarget& depthStencilRenderTarget = renderPassInfo.depthStencilRenderTarget;
            VulkanTexture* texture = device->GetTexture(depthStencilRenderTarget.texture);

            bool hasStencil = IsStencilFormat(texture->format);
            uint32 mipLevel = renderPassInfo.depthStencilRenderTarget.mipLevel;
            uint32 arrayLayer = renderPassInfo.depthStencilRenderTarget.arrayLayer;

            if (bSetExtent)
            {
                outRenderingInfo->extent.width = texture->width;
                outRenderingInfo->extent.height = texture->height;
                outRenderingInfo->extent.depth = 1;
                bSetExtent = false;
            }
            else
            {
                assert(outRenderingInfo->extent.width == std::max(1u, texture->width));
                assert(outRenderingInfo->extent.height == std::max(1u, texture->height));
            }

            VkRenderingAttachmentInfo& attachmentInfo = outRenderingInfo->depthStencilAttachment;
            attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachmentInfo.pNext = nullptr;
            attachmentInfo.imageView = texture->dsv[arrayLayer]; // TODO: specify mip level
            attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            //attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            //attachmentInfo.resolveImageView = VK_NULL_HANDLE;
            //attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
            attachmentInfo.loadOp = ConvertToVkAttachmentLoadOp(depthStencilRenderTarget.depthLoadOp);
            attachmentInfo.storeOp = ConvertToVkAttachmentStoreOp(depthStencilRenderTarget.depthStoreOp);
            attachmentInfo.clearValue.depthStencil = texture->clearValue.depthStencil;

            outRenderingInfo->depthStencilAttachmentFormat = texture->format;

            outRenderingInfo->hasDepthStencil = true;
        }

        VkRenderingInfo renderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = { 0, 0, outRenderingInfo->extent.width, outRenderingInfo->extent.height },
            .layerCount = 1,
            .colorAttachmentCount = outRenderingInfo->numColorAttachments,
            .pColorAttachments = outRenderingInfo->colorAttachments,
            .pDepthAttachment = outRenderingInfo->hasDepthStencil ? &outRenderingInfo->depthStencilAttachment : nullptr,
            .pStencilAttachment = nullptr // TODO: add stencil attachment
            //.pStencilAttachment = &outRenderingInfo->depthStencilAttachment
        };

        VkPipelineRenderingCreateInfo pipelineRenderingInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .colorAttachmentCount = outRenderingInfo->numColorAttachments,
            .pColorAttachmentFormats = outRenderingInfo->colorAttachmentFormats,
            .depthAttachmentFormat = outRenderingInfo->depthStencilAttachmentFormat,
            //.stencilAttachmentFormat = outRenderingInfo->depthStencilAttachmentFormat
        };

        outRenderingInfo->pipelineRenderingInfo = pipelineRenderingInfo;
        outRenderingInfo->renderingInfo = renderingInfo;
    }

    void VulkanDevice::Tick()
    {
        /*while (!resourcesToDestroy.empty())
        {
            const auto& resource = resourcesToDestroy.front();
            switch (resource.type)
            {
            case ResourceToDestroy::Type::Buffer:
                vmaDestroyBuffer(vmaAllocator, (VkBuffer)resource.vkHandle, resource.allocation);
                break;
            default:
                break;
            }
            resourcesToDestroy.pop();
        }*/
    }

    uint32 VulkanDevice::CreateBuffer(const RenderBackendBufferDesc* desc, const void* data, const char* name)
    {
        VulkanBuffer buffer = {
            .size = desc->size,
            .usageFlags = GetVkBufferUsageFlags(desc->flags),
            .name = name,
        };

        VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = buffer.size,
            .usage = buffer.usageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        buffer.allocationFlags = GetVmaAllocationCreateFlags(desc->flags);
        buffer.memeryUsage = GetVmaMemoryUsage(desc->flags);
        buffer.createMapped = (buffer.allocationFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT) ? true : false;

        VmaAllocationCreateInfo memoryInfo = {
            .flags = buffer.allocationFlags,
            .usage = buffer.memeryUsage,
        };

        VmaAllocationInfo allocationInfo = {};
        VK_CHECK(vmaCreateBuffer(vmaAllocator, &bufferInfo, &memoryInfo, &buffer.handle, &buffer.allocation, &allocationInfo));

        if (!buffer.name.empty())
        {
            SetDebugUtilsObjectName(VK_OBJECT_TYPE_BUFFER, (uint64)buffer.handle, buffer.name.c_str());
        }

        if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {};
            bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            bufferDeviceAddressInfo.buffer = buffer.handle;
            buffer.deviceAddress = vkGetBufferDeviceAddress(handle, &bufferDeviceAddressInfo);
        }

        if (buffer.createMapped)
        {
            VK_CHECK(vmaMapMemory(vmaAllocator, buffer.allocation, &buffer.mappedData));
            buffer.mapped = true;
        }

        if (data != nullptr)
        {
            uint64 bufferSize = (uint32)buffer.size;
            RenderBackendBufferDesc uploadBufferDesc = RenderBackendBufferDesc::CreateUpload(bufferSize);
            uint32 bufferIndex = CreateBuffer(&uploadBufferDesc, nullptr, "UploadBuffer");
            VulkanBuffer& uploadBuffer = buffers[bufferIndex];

            memcpy(uploadBuffer.mappedData, data, bufferSize);

            VkCommandBuffer commandBuffer; VkCommandPool pool;
            VulkanHelper::CreateTemporaryCommandBuffer(handle, GetQueueFamilyIndex(RenderBackendQueueFamily::Graphics), pool, commandBuffer);

            VkBufferCopy region = {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = bufferSize,
            };
            vkCmdCopyBuffer(commandBuffer, uploadBuffer.handle, buffer.handle, 1, &region);

            VulkanHelper::FlushTemporaryCommandBuffer(handle, GetCommandQueue(RenderBackendQueueFamily::Graphics, 0)->handle, pool, commandBuffer);
            DestroyBuffer(bufferIndex);
        }

        if (HAS_ANY_FLAGS(desc->flags, RenderBackendBufferCreateFlags::UnorderedAccess))
        {
            uint32 index = bindlessDescriptorManager.AllocateStorageBufferIndex();
            VkDescriptorBufferInfo descriptorBufferInfo = {
                .buffer = buffer.handle,
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = bindlessDescriptorManager.set,
                .dstBinding = BindlessBindingStroageBuffers,
                .dstArrayElement = index,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &descriptorBufferInfo,
            };
            vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
            buffer.uavIndex = index;
        }

        uint32 bufferIndex = 0;
        if (!freeBuffers.empty())
        {
            bufferIndex = freeBuffers.back();
            freeBuffers.pop_back();
            buffers[bufferIndex] = buffer;
        }
        else
        {
            bufferIndex = (uint32)buffers.size();
            buffers.emplace_back(buffer);
        }
        return bufferIndex;
    }

    void VulkanDevice::ResizeBuffer(uint32 index, uint64 size)
    {
        VulkanBuffer& buffer = buffers[index];
        if (buffer.handle != VK_NULL_HANDLE)
        {
            ResourceToDestroy resource = {
                .type = ResourceToDestroy::Type::Buffer,
                .vkHandle = (uint64)buffer.handle,
                .allocation = buffer.allocation,
            };
            resourcesToDestroy.emplace(resource);
        }
        if (size > 0)
        {
            buffer.size = size;

            VkBufferCreateInfo bufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = buffer.size,
                .usage = buffer.usageFlags,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };
            VmaAllocationCreateInfo memoryInfo = {
                .flags = buffer.allocationFlags,
                .usage = buffer.memeryUsage,
            };
            VmaAllocationInfo allocationInfo = {};
            VK_CHECK(vmaCreateBuffer(vmaAllocator, &bufferInfo, &memoryInfo, &buffer.handle, &buffer.allocation, &allocationInfo));

            if (!buffer.name.empty())
            {
                SetDebugUtilsObjectName(VK_OBJECT_TYPE_BUFFER, (uint64)buffer.handle, buffer.name.c_str());
            }

            if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
            {
                VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {};
                bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
                bufferDeviceAddressInfo.buffer = buffer.handle;
                buffer.deviceAddress = vkGetBufferDeviceAddress(handle, &bufferDeviceAddressInfo);
            }

            if (buffer.createMapped)
            {
                VK_CHECK(vmaMapMemory(vmaAllocator, buffer.allocation, &buffer.mappedData));
                buffer.mapped = true;
            }

            if ((buffer.usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) || (buffer.usageFlags & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT))
            {
                VkDescriptorBufferInfo descriptorBufferInfo = {
                   .buffer = buffer.handle,
                   .offset = 0,
                   .range = VK_WHOLE_SIZE
                };
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = bindlessDescriptorManager.set,
                    .dstBinding = BindlessBindingStroageBuffers,
                    .dstArrayElement = (uint32)buffer.uavIndex,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &descriptorBufferInfo,
                };
                vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
            }
        }
        else
        {
            buffer.handle = VK_NULL_HANDLE;
            buffer.allocation = VK_NULL_HANDLE;
        }
    }

    uint64 VulkanDevice::GetBufferDeviceAddress(RenderBackendBufferHandle bufferHandle)
    {
        uint32 index = GetRenderBackendHandleRepresentation(bufferHandle.GetIndex());
        VulkanBuffer& buffer = buffers[index];
        VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buffer.handle,
        };
        return backend->functions.vkGetBufferDeviceAddressKHR(handle, &bufferDeviceAddressInfo);
    }

    void VulkanDevice::DestroyBuffer(uint32 index)
    {
        VulkanBuffer& buffer = buffers[index];

        if (buffer.mapped)
        {
            vmaUnmapMemory(vmaAllocator, buffer.allocation);
        }

        vmaDestroyBuffer(vmaAllocator, buffer.handle, buffer.allocation);
        buffer.handle = VK_NULL_HANDLE;

        freeBuffers.push_back(index);
    }

    void* VulkanDevice::MapBuffer(uint32 index)
    {
        VulkanBuffer& buffer = buffers[index];
        if (!buffer.createMapped && !buffer.mapped && !buffer.mappedData)
        {
            VK_CHECK(vmaMapMemory(vmaAllocator, buffer.allocation, &buffer.mappedData));
            buffer.mapped = true;
        }
        return buffer.mappedData;
    }

    void VulkanDevice::UnmapBuffer(uint32 index)
    {
        VulkanBuffer& buffer = buffers[index];
        VK_CHECK(vmaFlushAllocation(vmaAllocator, buffer.allocation, 0, buffer.size));
        if (!buffer.createMapped && buffer.mapped && buffer.mappedData)
        {
            vmaUnmapMemory(vmaAllocator, buffer.allocation);
            buffer.mapped = false;
            buffer.mappedData = nullptr;
        }
    }

    uint32 VulkanDevice::CreateTexture(const RenderBackendTextureDesc* desc, const void* data, const char* name)
    {
        uint32 textureIndex = 0;
        if (!freeTextures.empty())
        {
            textureIndex = freeTextures.back();
            freeTextures.pop_back();
        }
        else
        {
            textureIndex = (uint32)textures.size();
            textures.emplace_back();
        }

        VkFormat format = ConvertToVkFormat(desc->format);

        VulkanTexture& texture = textures[textureIndex];
        texture = {
            .swapchainBuffer = false,
            .width = desc->width,
            .height = desc->height,
            .depth = desc->depth,
            .arrayLayers = desc->arrayLayers,
            .mipLevels = desc->mipLevels,
            .format = format,
            .type = ConvertToVkImageType(desc->type),
            .t = desc->type,
            .flags = desc->flags,
            .aspectMask = GetVkImageAspectFlags(format),
            .clearValue = ConvertToVkClearValue(desc->clearValue),
        };

        if (HAS_ANY_FLAGS(desc->flags, RenderBackendTextureCreateFlags::Readback))
        {
            texture.cpuReadbackBuffer = new VulkanCpuReadbackBuffer();

            uint32 stride = RenderBackendGetTextureFormatDesc(desc->format).bytes;
            uint32 width = texture.width;
            uint32 height = texture.height;
            uint32 depth = texture.depth;
            uint64 size = 0;
            for (uint32 mipLevel = 0; mipLevel < desc->mipLevels; mipLevel++)
            {
                uint32 mipSize = width * height * depth * stride;
                texture.cpuReadbackBuffer->mipOffsets[mipLevel] = size;
                texture.cpuReadbackBuffer->mipSize[mipLevel] = mipSize;
                width = std::max(1u, width / 2);
                height = std::max(1u, height / 2);
                depth = std::max(1u, depth / 2);
                size += mipSize;
            }

            VkBufferCreateInfo bufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = size,
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            };

            VmaAllocationCreateInfo memoryInfo = {
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_GPU_TO_CPU,
            };

            VmaAllocationInfo allocationInfo = {};
            VK_CHECK(vmaCreateBuffer(vmaAllocator, &bufferInfo, &memoryInfo, &texture.cpuReadbackBuffer->handle, &texture.allocation, &allocationInfo));

            VK_CHECK(vmaMapMemory(vmaAllocator, texture.allocation, &texture.cpuReadbackBuffer->data));

            return textureIndex;
        }

        VkImageCreateFlags flags = 0;
        if (desc->type == RenderBackendTextureType::TextureCube)
        {
            flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        VkImageUsageFlags usage = GetVkImageUsageFlags(desc->flags);
        VkImageCreateInfo imageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = flags,
            .imageType = texture.type,
            .format = texture.format,
            .extent = { texture.width, texture.height, texture.depth },
            .mipLevels = texture.mipLevels,
            .arrayLayers = texture.arrayLayers,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        if (HAS_ANY_FLAGS(desc->flags, RenderBackendTextureCreateFlags::Sparse))
        {
            //flags |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
            //flags |= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
            //// flags |= VK_IMAGE_CREATE_SPARSE_ALIASED_BIT;

            //VK_CHECK(vkCreateImage(handle, &imageInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.handle));

            //VkMemoryRequirements memoryRequirements = {};
            //vkGetImageMemoryRequirements(handle, texture.handle, &memoryRequirements);
            //texture.sparsePageSize = memoryRequirements.alignment;

            //uint32 sparseMemoryRequirementCount = 0;
            //vkGetImageSparseMemoryRequirements(handle, texture.handle, &sparseMemoryRequirementCount, nullptr);
            //std::vector<VkSparseImageMemoryRequirements> sparseImageMemoryRequirements(sparseMemoryRequirementCount);
            //vkGetImageSparseMemoryRequirements(handle, texture.handle, &sparseMemoryRequirementCount, sparseImageMemoryRequirements.data());

            //texture->sparse_properties = &texture->sparse_texture_properties;

            //SparseTextureProperties& out_sparse = internal_state->sparse_texture_properties;
            //out_sparse.total_tile_count = uint32_t(memory_requirements.size / memory_requirements.alignment);

            //for (size_t i = 0; i < sparse_requirements.size(); ++i)
            //{
            //    const VkSparseImageMemoryRequirements& in_sparse = sparse_requirements[i];
            //    if (i == 0)
            //    {
            //        // These should be common for all subresources right? Like in DX12?
            //        out_sparse.tile_width = in_sparse.formatProperties.imageGranularity.width;
            //        out_sparse.tile_height = in_sparse.formatProperties.imageGranularity.height;
            //        out_sparse.tile_depth = in_sparse.formatProperties.imageGranularity.depth;
            //        out_sparse.packed_mip_start = in_sparse.imageMipTailFirstLod;
            //        out_sparse.packed_mip_count = texture->desc.mip_levels - in_sparse.imageMipTailFirstLod;
            //        out_sparse.packed_mip_tile_offset = uint32_t(in_sparse.imageMipTailOffset / memory_requirements.alignment);
            //        out_sparse.packed_mip_tile_count = uint32_t(in_sparse.imageMipTailSize / memory_requirements.alignment);
            //    }
            //}
        }
        else
        {
            VmaAllocationCreateInfo memoryInfo = {};
            memoryInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            if (imageInfo.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
            {
                memoryInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
            }
            VK_CHECK(vmaCreateImage(vmaAllocator, &imageInfo, &memoryInfo, &texture.handle, &texture.allocation, VULKAN_ALLOCATION_CALLBACKS));
            texture.deivceMemory = texture.allocation->GetMemory();
            texture.info = imageInfo;
        }

        if (name)
        {
            SetDebugUtilsObjectName(VK_OBJECT_TYPE_IMAGE, (uint64)texture.handle, name);
        }

        if (HAS_ANY_FLAGS(desc->flags, RenderBackendTextureCreateFlags::ShaderResource))
        {
            VkImageViewCreateInfo imageViewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = texture.handle,
                .viewType = ConvertToVkImageViewType(desc->type, texture.IsArray()),
                .format = texture.format,
                .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                .subresourceRange = { texture.aspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS }
            };
            VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.srv));

            uint32 index = bindlessDescriptorManager.AllocateSampledImageIndex();
            VkDescriptorImageInfo descriptorImageInfo = {
                .imageView = texture.srv,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = bindlessDescriptorManager.set,
                .dstBinding = BindlessBindingSampledImages,
                .dstArrayElement = index,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &descriptorImageInfo,
            };
            vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
            texture.srvIndex = index;

            texture.srvs.resize(texture.mipLevels);
            for (uint32 mipLevel = 0; mipLevel < texture.mipLevels; mipLevel++)
            {
                VkImageViewCreateInfo imageViewInfo = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = texture.handle,
                    .viewType = ConvertToVkImageViewType(desc->type, texture.IsArray()),
                    .format = texture.format,
                    .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                    .subresourceRange = { texture.aspectMask, mipLevel, 1, 0, VK_REMAINING_ARRAY_LAYERS }
                };
                VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.srvs[mipLevel].srv));

                uint32 index = bindlessDescriptorManager.AllocateSampledImageIndex();
                VkDescriptorImageInfo descriptorImageInfo = {
                    .imageView = texture.srvs[mipLevel].srv,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = bindlessDescriptorManager.set,
                    .dstBinding = BindlessBindingSampledImages,
                    .dstArrayElement = index,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .pImageInfo = &descriptorImageInfo,
                };
                vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
                texture.srvs[mipLevel].srvIndex = index;
            }
        }
        if (HAS_ANY_FLAGS(desc->flags, RenderBackendTextureCreateFlags::UnorderedAccess))
        {
            texture.uavs.resize(texture.mipLevels);
            for (uint32 mipLevel = 0; mipLevel < texture.mipLevels; mipLevel++)
            {
                VkImageViewCreateInfo imageViewInfo = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = texture.handle,
                    .viewType = ConvertToVkImageViewType(desc->type, false),
                    .format = texture.format,
                    .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                    .subresourceRange = { texture.aspectMask, mipLevel, 1, 0, VK_REMAINING_ARRAY_LAYERS }
                };
                VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.uavs[mipLevel].uav));

                uint32 index = bindlessDescriptorManager.AllocateStorageImageIndex();
                VkDescriptorImageInfo descriptorImageInfo = {
                    .imageView = texture.uavs[mipLevel].uav,
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                };
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = bindlessDescriptorManager.set,
                    .dstBinding = BindlessBindingStroageImages,
                    .dstArrayElement = index,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &descriptorImageInfo,
                };
                vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
                texture.uavs[mipLevel].uavIndex = index;
            }
        }
        if (HAS_ANY_FLAGS(desc->flags, RenderBackendTextureCreateFlags::RenderTarget))
        {
            texture.rtv.resize(texture.mipLevels);
            for (uint32 i = 0; i < texture.mipLevels; i++)
            {
                VkImageViewCreateInfo imageViewInfo = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = texture.handle,
                    .viewType = ConvertToVkImageViewType(desc->type, false),
                    .format = texture.format,
                    .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                    .subresourceRange = { texture.aspectMask, i, 1, 0, 1 }
                };
                VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.rtv[i]));
            }
        }
        if (HAS_ANY_FLAGS(desc->flags, RenderBackendTextureCreateFlags::DepthStencil))
        {
            for (uint32 i = 0; i < texture.arrayLayers; i++)
            {
                VkImageView dsv = VK_NULL_HANDLE;
                VkImageViewCreateInfo imageViewInfo = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = texture.handle,
                    .viewType = ConvertToVkImageViewType(desc->type, false),
                    .format = texture.format,
                    .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                    .subresourceRange = { texture.aspectMask, 0, 1, i, 1 }
                };
                VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &dsv));
                texture.dsv.push_back(dsv);
            }
        }

        if (data != nullptr)
        {
            RenderBackendBufferDesc uploadBufferDesc = RenderBackendBufferDesc::CreateUpload((uint32)texture.allocation->GetSize());
            uint32 bufferIndex = CreateBuffer(&uploadBufferDesc, nullptr, "UploadBuffer");
            VulkanBuffer& uploadBuffer = buffers[bufferIndex];
            MapBuffer(bufferIndex);
            std::vector<VkBufferImageCopy> copyRegions;
            VkDeviceSize copyOffset = 0;
            uint64 dataOffset = 0;
            for (uint32 layer = 0; layer < desc->arrayLayers; layer++)
            {
                uint32 width = imageInfo.extent.width;
                uint32 height = imageInfo.extent.height;
                uint32 depth = imageInfo.extent.depth;
                for (uint32 level = 0; level < desc->mipLevels; level++)
                {
                    uint64 copySize = width * height * depth * RenderBackendGetTextureFormatDesc(desc->format).bytes;
                    uint8* copyDst = (uint8*)uploadBuffer.mappedData + copyOffset;
                    memcpy(copyDst, (uint8*)data + dataOffset, copySize);

                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset = copyOffset;
                    copyRegion.bufferRowLength = 0;
                    copyRegion.bufferImageHeight = 0;
                    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel = level;
                    copyRegion.imageSubresource.baseArrayLayer = layer;
                    copyRegion.imageSubresource.layerCount = 1;
                    copyRegion.imageOffset = { 0, 0, 0 };
                    copyRegion.imageExtent = { width, height, depth };

                    copyRegions.emplace_back(copyRegion);
                    copyOffset += copySize;

                    width = std::max(1u, width / 2);
                    height = std::max(1u, height / 2);
                    depth = std::max(1u, depth / 2);
                }
            }
            UnmapBuffer(bufferIndex);

            VkCommandBuffer commandBuffer; VkCommandPool pool;
            VulkanHelper::CreateTemporaryCommandBuffer(handle, GetQueueFamilyIndex(RenderBackendQueueFamily::Graphics), pool, commandBuffer);

            {
                VkImageMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = texture.handle,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = imageInfo.mipLevels,
                        .baseArrayLayer = 0,
                        .layerCount = imageInfo.arrayLayers,
                    },
                };

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                vkCmdCopyBufferToImage(
                    commandBuffer,
                    uploadBuffer.handle,
                    texture.handle,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    (uint32)copyRegions.size(),
                    copyRegions.data());

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
            }

            for (uint32 mipLevel = 1; mipLevel < texture.mipLevels; mipLevel++)
            {
                VkImageBlit imageBlit = {
                    .srcSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = mipLevel - 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                    .srcOffsets = {
                        { .x = 0, .y = 0, .z = 0 },
                        { .x = std::max((int32)(texture.width >> (mipLevel - 1)), 1), .y = std::max((int32)(texture.height >> (mipLevel - 1)), 1), .z = 1,},
                    },
                    .dstSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = mipLevel,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                    .dstOffsets = {
                        { .x = 0, .y = 0, .z = 0 },
                        { .x = std::max((int32)(texture.width >> mipLevel), 1), .y = std::max((int32)(texture.height >> mipLevel), 1), .z = 1,},
                    },
                };

                VkImageMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = texture.handle,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = mipLevel,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };

                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                vkCmdBlitImage(commandBuffer,
                    texture.handle,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    texture.handle,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &imageBlit,
                    VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
            }

            {
                VkImageMemoryBarrier barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = texture.handle,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = texture.mipLevels,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
            }

            VulkanHelper::FlushTemporaryCommandBuffer(handle, GetCommandQueue(RenderBackendQueueFamily::Graphics, 0)->handle, pool, commandBuffer);
            DestroyBuffer(bufferIndex);
        }

        if (desc->initialState != RenderBackendResourceState::Undefined)
        {
            VkCommandBuffer commandBuffer; VkCommandPool pool;
            VulkanHelper::CreateTemporaryCommandBuffer(handle, GetQueueFamilyIndex(RenderBackendQueueFamily::Graphics), pool, commandBuffer);

            VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            VkAccessFlags2 srcAccessMask, dstAccessMask;
            VkImageLayout oldLayout, newLayout;
            // TODO
            GetBarrierInfo2(
                RenderBackendResourceState::Undefined,
                desc->initialState,
                &oldLayout,
                &newLayout,
                &srcStageMask,
                &dstStageMask,
                &srcAccessMask,
                &dstAccessMask);
            VkImageMemoryBarrier2 imageBarrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = srcStageMask,
                .srcAccessMask = srcAccessMask,
                .dstStageMask = dstStageMask,
                .dstAccessMask = dstAccessMask,
                .oldLayout = oldLayout,
                .newLayout = newLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = texture.handle,
                .subresourceRange = {
                    .aspectMask = texture.aspectMask,
                    .baseMipLevel = 0,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS,
                },
            };

            VkDependencyInfo dependency = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &imageBarrier,
            };
            vkCmdPipelineBarrier2(commandBuffer, &dependency);

            VulkanHelper::FlushTemporaryCommandBuffer(handle, GetCommandQueue(RenderBackendQueueFamily::Graphics, 0)->handle, pool, commandBuffer);
        }

        return textureIndex;
    }

    void VulkanDevice::DestroyTexture(uint32 index)
    {

    }

    uint32 VulkanDevice::CreateTextureSRV(uint32 textureIndex, const RenderBackendTextureSRVDesc* desc, const char* name)
    {
        VulkanTexture& texture = textures[textureIndex];
        VkImageViewCreateInfo imageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = texture.handle,
            .viewType = ConvertToVkImageViewType(texture.t, desc->numArrayLayers > 1),
            .format = texture.format,
            .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
            .subresourceRange = { texture.aspectMask, desc->baseMipLevel, desc->numMipLevels, desc->baseArrayLayer, desc->numArrayLayers }
        };
        VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.srv));

        uint32 index = bindlessDescriptorManager.AllocateSampledImageIndex();
        VkDescriptorImageInfo descriptorImageInfo = {
            .imageView = texture.srv,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = bindlessDescriptorManager.set,
            .dstBinding = BindlessBindingSampledImages,
            .dstArrayElement = index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &descriptorImageInfo,
        };
        vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
        texture.srvIndex = index;
        return index;
    }

    int32 VulkanDevice::GetTextureSRVDescriptorIndex(uint32 textureIndex)
    {
        return textures[textureIndex].srvIndex;
    }

    uint32 VulkanDevice::CreateTextureUAV(uint32 textureIndex, const RenderBackendTextureUAVDesc* desc, const char* name)
    {
        VulkanTexture& texture = textures[textureIndex];
        VkImageViewCreateInfo imageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = texture.handle,
            .viewType = ConvertToVkImageViewType(texture.t, false),
            .format = texture.format,
            .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
            .subresourceRange = { texture.aspectMask, desc->mipLevel, 1, 0, 1 }
        };
        VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.uavs[desc->mipLevel].uav));

        uint32 index = bindlessDescriptorManager.AllocateSampledImageIndex();
        VkDescriptorImageInfo descriptorImageInfo = {
            .imageView = texture.uavs[desc->mipLevel].uav,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = bindlessDescriptorManager.set,
            .dstBinding = BindlessBindingSampledImages,
            .dstArrayElement = index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &descriptorImageInfo,
        };
        vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
        texture.uavs[desc->mipLevel].uavIndex = index;
        return index;
    }

    int32 VulkanDevice::GetTextureUAVDescriptorIndex(uint32 textureIndex, uint32 mipLevel)
    {
        VulkanTexture& texture = textures[textureIndex];
        return texture.uavs[mipLevel].uavIndex;
    }

    uint32 VulkanDevice::CreateSampler(const RenderBackendSamplerDesc* desc, const char* name)
    {
        VulkanSampler sampler = {};

        VkFilter minFilter, magFilter;
        VkSamplerMipmapMode mipmapMode;
        bool anisotropyEnable;
        bool compareEnable;
        GetVkFilterAndVkSamplerMipmapMode(desc->filter, &minFilter, &magFilter, &mipmapMode, &anisotropyEnable, &compareEnable);

        void* next = nullptr;
        VkSamplerReductionModeCreateInfo reductionModeInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO
        };

        VkCompareOp compareOp = ConvertToVkCompareOp(desc->compareOp);
        if (false)
        {
            switch (desc->filter)
            {
            case RenderBackendTextureFilter::MinimumMinMagMipPoint:
            case RenderBackendTextureFilter::MinimumMinMagPointMipLinear:
            case RenderBackendTextureFilter::MinimumMinPointMagLinearMipPoint:
            case RenderBackendTextureFilter::MinimumMinPointMagMipLinear:
            case RenderBackendTextureFilter::MinimumMinLinearMagMipPoint:
            case RenderBackendTextureFilter::MinimumMinLinearMagPointMipLinear:
            case RenderBackendTextureFilter::MinimumMinMagLinearMipPoint:
            case RenderBackendTextureFilter::MinimumMinMagMipLinear:
            case RenderBackendTextureFilter::MinimumAnisotropic:
                reductionModeInfo.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
                next = &reductionModeInfo;
                break;
            case RenderBackendTextureFilter::MaximumMinMagMipPoint:
            case RenderBackendTextureFilter::MaximumMinMagPointMipLinear:
            case RenderBackendTextureFilter::MaximumMinPointMagLinearMipPoint:
            case RenderBackendTextureFilter::MaximumMinPointMagMipLinear:
            case RenderBackendTextureFilter::MaximumMinLinearMagMipPoint:
            case RenderBackendTextureFilter::MaximumMinLinearMagPointMipLinear:
            case RenderBackendTextureFilter::MaximumMinMagLinearMipPoint:
            case RenderBackendTextureFilter::MaximumMinMagMipLinear:
            case RenderBackendTextureFilter::MaximumAnisotropic:
                reductionModeInfo.reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX;
                next = &reductionModeInfo;
                break;
            default:
                break;
            }
        }

        VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = next,
            .magFilter = minFilter,
            .minFilter = magFilter,
            .mipmapMode = mipmapMode,
            .addressModeU = ConvertToVkSamplerAddressMode(desc->addressModeU),
            .addressModeV = ConvertToVkSamplerAddressMode(desc->addressModeV),
            .addressModeW = ConvertToVkSamplerAddressMode(desc->addressModeW),
            .mipLodBias = desc->mipLodBias,
            .anisotropyEnable = ConvertToVkBool(anisotropyEnable),
            .maxAnisotropy = (float)desc->maxAnisotropy,
            .compareEnable = ConvertToVkBool(compareEnable),
            .compareOp = compareEnable ? compareOp : VK_COMPARE_OP_NEVER,
            .minLod = desc->minLod,
            .maxLod = desc->maxLod,
            .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        VK_CHECK(vkCreateSampler(handle, &samplerInfo, VULKAN_ALLOCATION_CALLBACKS, &sampler.handle));

        sampler.bindlessIndex = bindlessDescriptorManager.AllocateSamplerIndex();

        VkDescriptorImageInfo imageInfo = {
            .sampler = sampler.handle
        };
        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = bindlessDescriptorManager.set,
            .dstBinding = BindlessBindingSamplers,
            .dstArrayElement = sampler.bindlessIndex,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo = &imageInfo,
        };
        vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);

        uint32 samplerIndex = 0;
        if (!freeSamplers.empty())
        {
            samplerIndex = freeSamplers.back();
            freeSamplers.pop_back();
            samplers[samplerIndex] = sampler;
        }
        else
        {
            samplerIndex = (uint32)samplers.size();
            samplers.emplace_back(sampler);
        }
        return samplerIndex;
    }

    void VulkanDevice::DestroySampler(uint32 index)
    {

    }

    static void InitVkPipelineRasterizationStateCreateInfo(const RenderBackendRasterizationState& state, VkPipelineRasterizationStateCreateInfo& outInfo)
    {
        outInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        outInfo.depthClampEnable = ConvertToVkBool(state.depthClampEnable);
        outInfo.depthBiasEnable = (state.depthBiasConstantFactor != 0.0f || state.depthBiasSlopeFactor != 0.0f) ? VK_TRUE :VK_FALSE;
        outInfo.depthBiasConstantFactor = state.depthBiasConstantFactor;
        outInfo.depthBiasClamp = state.depthBiasClamp;
        outInfo.depthBiasSlopeFactor = state.depthBiasSlopeFactor;
        outInfo.rasterizerDiscardEnable = VK_FALSE;
        outInfo.polygonMode = ConvertToVkPolygonMode(state.fillMode);
        outInfo.cullMode = ConvertToVkCullModeFlags(state.cullMode);
        outInfo.frontFace = state.frontFaceCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        outInfo.lineWidth = state.lineWidth;
    }

    static void InitVkPipelineDepthStencilStateCreateInfo(const RenderBackendDepthStencilState& state, VkPipelineDepthStencilStateCreateInfo& outInfo)
    {
        outInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        outInfo.depthTestEnable = ConvertToVkBool(state.depthTestEnable);
        outInfo.depthWriteEnable = ConvertToVkBool(state.depthWriteEnable);
        outInfo.depthBoundsTestEnable = VK_FALSE;
        outInfo.depthCompareOp = ConvertToVkCompareOp(state.depthCompareFunction);
        outInfo.stencilTestEnable = ConvertToVkBool(state.stencilTestEnable);
        outInfo.front.passOp = ConvertToVkStencilOp(state.frontFaceStencilPassOp);
        outInfo.front.failOp = ConvertToVkStencilOp(state.frontFaceStencilFailOp);
        outInfo.front.depthFailOp = ConvertToVkStencilOp(state.frontFaceStencilDepthFailOp);
        outInfo.front.compareOp = ConvertToVkCompareOp(state.frontFaceStencilCompareFunction);
        outInfo.front.compareMask = state.stencilReadMask;
        outInfo.front.writeMask = state.stencilWriteMask;
        outInfo.front.reference = 0;
        outInfo.back.passOp = ConvertToVkStencilOp(state.backFaceStencilPassOp);
        outInfo.back.failOp = ConvertToVkStencilOp(state.backFaceStencilFailOp);
        outInfo.back.depthFailOp = ConvertToVkStencilOp(state.backFaceStencilDepthFailOp);
        outInfo.back.compareOp = ConvertToVkCompareOp(state.backFaceStencilCompareFunction);
        outInfo.back.compareMask = state.stencilReadMask;
        outInfo.back.writeMask = state.stencilWriteMask;
        outInfo.back.reference = 0;
    }

    static void InitVkPipelineColorBlendStateCreateInfo(const RenderBackendColorBlendState& state, VkPipelineColorBlendAttachmentState* attachmentStates, uint32 attachmentCount, VkPipelineColorBlendStateCreateInfo& outInfo)
    {
        for (uint32 i = 0; i < attachmentCount; i++)
        {
            attachmentStates[i].blendEnable = ConvertToVkBool(state.targetBlends[i].blendEnable);
            attachmentStates[i].srcColorBlendFactor = ConvertToVkBlendFactor(state.targetBlends[i].srcColorBlendFactor);
            attachmentStates[i].dstColorBlendFactor = ConvertToVkBlendFactor(state.targetBlends[i].dstColorBlendFactor);
            attachmentStates[i].colorBlendOp = ConvertToVkBlendOp(state.targetBlends[i].colorBlendOp);
            attachmentStates[i].srcAlphaBlendFactor = ConvertToVkBlendFactor(state.targetBlends[i].srcAlphaBlendFactor);
            attachmentStates[i].dstAlphaBlendFactor = ConvertToVkBlendFactor(state.targetBlends[i].dstAlphaBlendFactor);
            attachmentStates[i].alphaBlendOp = ConvertToVkBlendOp(state.targetBlends[i].alphaBlendOp);
            attachmentStates[i].colorWriteMask = 0;
            attachmentStates[i].colorWriteMask |= HAS_ANY_FLAGS(state.targetBlends[i].colorWriteMask, RenderBackendColorComponentFlags::R) ? VK_COLOR_COMPONENT_R_BIT : 0;
            attachmentStates[i].colorWriteMask |= HAS_ANY_FLAGS(state.targetBlends[i].colorWriteMask, RenderBackendColorComponentFlags::G) ? VK_COLOR_COMPONENT_G_BIT : 0;
            attachmentStates[i].colorWriteMask |= HAS_ANY_FLAGS(state.targetBlends[i].colorWriteMask, RenderBackendColorComponentFlags::B) ? VK_COLOR_COMPONENT_B_BIT : 0;
            attachmentStates[i].colorWriteMask |= HAS_ANY_FLAGS(state.targetBlends[i].colorWriteMask, RenderBackendColorComponentFlags::A) ? VK_COLOR_COMPONENT_A_BIT : 0;
        }
        outInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        outInfo.logicOpEnable = VK_FALSE;
        outInfo.logicOp = VK_LOGIC_OP_COPY;
        outInfo.attachmentCount = attachmentCount;
        outInfo.pAttachments = attachmentStates;
        outInfo.blendConstants[0] = 1.0f;
        outInfo.blendConstants[1] = 1.0f;
        outInfo.blendConstants[2] = 1.0f;
        outInfo.blendConstants[3] = 1.0f;
    }

    uint32 VulkanDevice::CreateShader(const RenderBackendShaderDesc* desc, const char* name)
    {
        uint32 shaderIndex = 0;
        if (!freeShaders.empty())
        {
            shaderIndex = freeShaders.back();
            freeShaders.pop_back();
        }
        else
        {
            shaderIndex = (uint32)shaders.size();
            shaders.emplace_back();
        }
        VulkanShader& shader = shaders[shaderIndex];

        for (uint32 stageIndex = 0; stageIndex < (uint32)RenderBackendShaderStage::Count; stageIndex++)
        {
            RenderBackendShaderStage stage = (RenderBackendShaderStage)stageIndex;
            if (desc->stages[stageIndex].size == 0 || !desc->stages[stageIndex].data)
            {
                continue;
            }

            shader.entryPoints[stageIndex] = desc->entryPoints[stageIndex].c_str();

            if (shader.entryPoints[stageIndex].empty())
            {
                shader.entryPoints[stageIndex] = "?";
            }
            printf("%s\n", shader.entryPoints[stageIndex].c_str());

            VkPipelineShaderStageCreateInfo& shaderStageInfo = shader.stages[shader.numStages];
            shaderStageInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = ConvertToVkShaderStageFlagBits(stage),
                .pName = shader.entryPoints[stageIndex].c_str(),
            };
            VkShaderModuleCreateInfo shaderModuleInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = desc->stages[stageIndex].size,
                .pCode = (uint32*)desc->stages[stageIndex].data,
            };
            VK_CHECK(vkCreateShaderModule(handle, &shaderModuleInfo, VULKAN_ALLOCATION_CALLBACKS, &shaderStageInfo.module));
            SetDebugUtilsObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64)shaderStageInfo.module, name);
            shader.numStages++;
        }

        return shaderIndex;
    }

    void VulkanDevice::DestroyShader(uint32 index)
    {

    }

    uint32 VulkanDevice::CreateAccelerationStructure(VulkanRayTracingAccelerationStructure* accelerationStructure, VkAccelerationStructureTypeKHR type, uint32* primitiveCounts, const char* name)
    {
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = type,
            .flags = accelerationStructure->buildFlags,
            .geometryCount = (uint32)accelerationStructure->geometries.size(),
            .pGeometries = accelerationStructure->geometries.data(),
        };

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
        };
        backend->functions.vkGetAccelerationStructureBuildSizesKHR(
            handle,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            primitiveCounts,
            &accelerationStructureBuildSizesInfo);

        // Create Acceleration Structure Buffer
        {
            accelerationStructure->accelerationStructureBuffer.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
            VkBufferCreateInfo bufferCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = accelerationStructure->accelerationStructureBuffer.size,
                .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            };
            VmaAllocationCreateInfo memoryInfo = {
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            };
            VmaAllocationInfo allocationInfo = {};
            VK_CHECK(vmaCreateBuffer(
                vmaAllocator,
                &bufferCreateInfo,
                &memoryInfo,
                &accelerationStructure->accelerationStructureBuffer.buffer,
                &accelerationStructure->accelerationStructureBuffer.allocation,
                &accelerationStructure->accelerationStructureBuffer.allocationInfo));
            SetDebugUtilsObjectName(VK_OBJECT_TYPE_BUFFER, (uint64)accelerationStructure->accelerationStructureBuffer.buffer, "Acceleration Structure Buffer");
            VkBufferDeviceAddressInfoKHR bufferDeviceInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = accelerationStructure->accelerationStructureBuffer.buffer,
            };
            accelerationStructure->accelerationStructureBuffer.deviceAddress = backend->functions.vkGetBufferDeviceAddressKHR(handle, &bufferDeviceInfo);
        }

        // Create Scratch Buffer
        {
            accelerationStructure->scratchBuffer.size = accelerationStructureBuildSizesInfo.buildScratchSize;
            VkBufferCreateInfo bufferCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = accelerationStructure->scratchBuffer.size,
                .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            };
            VmaAllocationCreateInfo memoryInfo = {
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            };
            VmaAllocationInfo allocationInfo = {};
            VK_CHECK(vmaCreateBuffer(
                vmaAllocator,
                &bufferCreateInfo,
                &memoryInfo,
                &accelerationStructure->scratchBuffer.buffer,
                &accelerationStructure->scratchBuffer.allocation,
                &accelerationStructure->scratchBuffer.allocationInfo));
            SetDebugUtilsObjectName(VK_OBJECT_TYPE_BUFFER, (uint64)accelerationStructure->scratchBuffer.buffer, "Acceleration Structure Scratch Buffer");
            VkBufferDeviceAddressInfoKHR bufferDeviceInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = accelerationStructure->scratchBuffer.buffer,
            };
            accelerationStructure->scratchBuffer.deviceAddress = backend->functions.vkGetBufferDeviceAddressKHR(handle, &bufferDeviceInfo);
        }

        VkAccelerationStructureCreateInfoKHR accelerationStructureInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = accelerationStructure->accelerationStructureBuffer.buffer,
            .size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
            .type = type
        };
        VK_CHECK(backend->functions.vkCreateAccelerationStructureKHR(
            handle,
            &accelerationStructureInfo,
            VULKAN_ALLOCATION_CALLBACKS,
            &accelerationStructure->handle));
        SetDebugUtilsObjectName(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64)accelerationStructure->handle, name);

        VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .accelerationStructure = accelerationStructure->handle,
        };
        accelerationStructure->deviceAddress = backend->functions.vkGetAccelerationStructureDeviceAddressKHR(handle, &deviceAddressInfo);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = type,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .dstAccelerationStructure = accelerationStructure->handle,
            .geometryCount = (uint32)accelerationStructure->geometries.size(),
            .pGeometries = accelerationStructure->geometries.data(),
            .scratchData = {.deviceAddress = accelerationStructure->scratchBuffer.deviceAddress }
        };

        std::vector<VkAccelerationStructureBuildRangeInfoKHR> accelerationBuildStructureRangeInfos((uint32)accelerationStructure->geometries.size());
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos((uint32)accelerationStructure->geometries.size());
        for (uint32 geometryIndex = 0; geometryIndex < (uint32)accelerationStructure->geometries.size(); geometryIndex++)
        {
            accelerationBuildStructureRangeInfos[geometryIndex] = {
                .primitiveCount = primitiveCounts[geometryIndex],
                .primitiveOffset = 0,
                .firstVertex = 0,
                .transformOffset = 0,
            };
            pBuildRangeInfos[geometryIndex] = &accelerationBuildStructureRangeInfos[geometryIndex];
        }

        VkCommandBuffer commandBuffer; VkCommandPool pool;
        VulkanHelper::CreateTemporaryCommandBuffer(handle, GetQueueFamilyIndex(RenderBackendQueueFamily::Graphics), pool, commandBuffer);
        backend->functions.vkCmdBuildAccelerationStructuresKHR(
            commandBuffer,
            1,
            &accelerationBuildGeometryInfo,
            pBuildRangeInfos.data());
        VulkanHelper::FlushTemporaryCommandBuffer(handle, GetCommandQueue(RenderBackendQueueFamily::Graphics, 0)->handle, pool, commandBuffer);

        if (type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR || type == VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR)
        {
            uint32 descriptorIndex = bindlessDescriptorManager.AllocateAccelerationStructureIndex();
            const VkWriteDescriptorSetAccelerationStructureKHR writeAccelerationStructureInfo = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
                .accelerationStructureCount = 1,
                .pAccelerationStructures = &accelerationStructure->handle,
            };
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = &writeAccelerationStructureInfo,
                .dstSet = bindlessDescriptorManager.set,
                .dstBinding = BindlessBindingAccelerationStructures,
                .dstArrayElement = descriptorIndex,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            };
            vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
            accelerationStructure->descriptorIndex = descriptorIndex;
        }

        uint32 index = 0;
        if (!freeAccelerationStructures.empty())
        {
            index = freeAccelerationStructures.back();
            freeAccelerationStructures.pop_back();
            accelerationStructures[index] = *accelerationStructure;
        }
        else
        {
            index = (uint32)accelerationStructures.size();
            accelerationStructures.emplace_back(*accelerationStructure);
        }
        return index;
    }

    void VulkanDevice::BindBindlessDescriptorSets(VkCommandBuffer commandBuffer)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bindlessDescriptorManager.compatibleGraphicsPipelineLayout, 0, 1, &bindlessDescriptorManager.set, 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindlessDescriptorManager.compatibleComputePipelineLayout, 0, 1, &bindlessDescriptorManager.set, 0, nullptr);
        if (backend->enableRayTracingSupport)
        {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, bindlessDescriptorManager.compatibleRayTracingPipelineLayout, 0, 1, &bindlessDescriptorManager.set, 0, nullptr);
        }
    }

    uint32 VulkanDevice::CreateBottomLevelAS(const RenderBackendRayTracingBottomLevelAccelerationDesc* desc, const char* name)
    {
        VulkanRayTracingAccelerationStructure accelerationStructure;
        accelerationStructure.buildFlags = ConvertToVkBuildAccelerationStructureFlagsKHR(desc->buildFlags);
        accelerationStructure.blasDesc = *desc;

        std::vector<uint32> primitiveCounts(desc->numGeometries);
        for (uint32 i = 0; i < desc->numGeometries; i++)
        {
            const RenderBackendRayTracingGeometryDesc& geometryDesc = desc->geometryDescs[i];
            VkAccelerationStructureGeometryKHR geometry = {
               .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
               .geometryType = (VkGeometryTypeKHR)geometryDesc.type,
               .flags = ConvertToVkGeometryFlagsKHR(geometryDesc.flags),
            };
            if (geometry.geometryType == VK_GEOMETRY_TYPE_TRIANGLES_KHR)
            {
                uint32 maxVertex = geometryDesc.triangleDesc.numVertices;
                geometry.geometry.triangles = {
                    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                    .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
                    .vertexData = GetBufferDeviceAddress(geometryDesc.triangleDesc.vertexBuffer) + geometryDesc.triangleDesc.vertexOffset,
                    .vertexStride = geometryDesc.triangleDesc.vertexStride,
                    .maxVertex = maxVertex,
                    .indexType = VK_INDEX_TYPE_UINT32,
                    .indexData = GetBufferDeviceAddress(geometryDesc.triangleDesc.indexBuffer) + geometryDesc.triangleDesc.indexOffset,
                    .transformData = GetBufferDeviceAddress(geometryDesc.triangleDesc.transformBuffer) + geometryDesc.triangleDesc.transformOffset
                };
                primitiveCounts[i] = geometryDesc.triangleDesc.numIndices / 3;
            }
            else if (geometry.geometryType == VK_GEOMETRY_TYPE_AABBS_KHR)
            {
                geometry.geometry.aabbs = {
                   .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
                   .data = GetBufferDeviceAddress(geometryDesc.aabbDesc.buffer) + geometryDesc.aabbDesc.offset,
                   .stride = sizeof(VkAabbPositionsKHR)
                };
                VulkanBuffer* buffer = GetBuffer(geometryDesc.aabbDesc.buffer);
                primitiveCounts[i] = (uint32)(buffer->size / sizeof(VkAabbPositionsKHR));
            }
            accelerationStructure.geometries.emplace_back(geometry);
        }
        uint32 index = CreateAccelerationStructure(&accelerationStructure, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, primitiveCounts.data(), name);

        return index;
    }

    uint32 VulkanDevice::CreateTopLevelAS(const RenderBackendRayTracingTopLevelAccelerationDesc* desc, const char* name)
    {
        VulkanRayTracingAccelerationStructure accelerationStructure = {};
        accelerationStructure.buildFlags = ConvertToVkBuildAccelerationStructureFlagsKHR(desc->buildFlags);
        accelerationStructure.tlasDesc = *desc;

        std::vector<VkAccelerationStructureInstanceKHR> instances;
        for (uint32 i = 0; i < desc->numInstances; i++)
        {
            VkTransformMatrixKHR transformMatrix;
            memcpy(&transformMatrix, &desc->instances[i].transformMatrix, sizeof(VkTransformMatrixKHR));
            VkAccelerationStructureInstanceKHR instance = {
                .transform = transformMatrix,
                .instanceCustomIndex = desc->instances[i].instanceID,
                .mask = desc->instances[i].instanceMask,
                .instanceShaderBindingTableRecordOffset = desc->instances[i].instanceContributionToHitGroupIndex,
                .flags = ConvertToVkGeometryInstanceFlagsKHR(desc->instances[i].flags),
                .accelerationStructureReference = GetAccelerationStructure(desc->instances[i].blas)->deviceAddress,
            };
            instances.emplace_back(instance);
        }

        VulkanRayTracingAccelerationStructure::Buffer instanceBuffer;
        {
            instanceBuffer.size = desc->numInstances * sizeof(VkAccelerationStructureInstanceKHR);
            VkBufferCreateInfo bufferCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = instanceBuffer.size,
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            };
            VmaAllocationCreateInfo memoryInfo = {
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_CPU_ONLY,
            };
            VmaAllocationInfo allocationInfo = {};
            VK_CHECK(vmaCreateBuffer(
                vmaAllocator,
                &bufferCreateInfo,
                &memoryInfo,
                &instanceBuffer.buffer,
                &instanceBuffer.allocation,
                &instanceBuffer.allocationInfo));
            SetDebugUtilsObjectName(VK_OBJECT_TYPE_BUFFER, (uint64)instanceBuffer.buffer, "Acceleration Structure Instance Buffer");
            VkBufferDeviceAddressInfoKHR bufferDeviceInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = instanceBuffer.buffer,
            };
            instanceBuffer.deviceAddress = backend->functions.vkGetBufferDeviceAddressKHR(handle, &bufferDeviceInfo);
            memcpy(instanceBuffer.allocationInfo.pMappedData, instances.data(), instanceBuffer.size);
            VK_CHECK(vmaFlushAllocation(vmaAllocator, instanceBuffer.allocation, 0, instanceBuffer.size));
            accelerationStructure.resourceBuffers.emplace_back(instanceBuffer);
        }

        VkAccelerationStructureGeometryKHR geometry = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .geometry = {
                .instances = {
                    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                    .arrayOfPointers = VK_FALSE,
                    .data = {
                        .deviceAddress = instanceBuffer.deviceAddress
                    },
                },
            },
            .flags = ConvertToVkGeometryFlagsKHR(desc->geometryFlags),
        };
        accelerationStructure.geometries.emplace_back(geometry);

        uint32 numPrimitives = desc->numInstances;
        uint32 index = CreateAccelerationStructure(&accelerationStructure, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, &numPrimitives, name);

        return index;
    }

    void VulkanDevice::ResizeSwapChain(uint32 index, uint32* width, uint32* height)
    {
        VulkanSwapchain* swapchain = &swapchains[index];
        if (swapchain->info.imageExtent.width == *width && swapchain->info.imageExtent.height == *height)
        {
            return;
        }
        RecreateSwapChain(index);
        *width = swapchain->info.imageExtent.width;
        *height = swapchain->info.imageExtent.height;
    }

    VulkanSwapchain::Status VulkanDevice::AcquireImageIndex(uint32 index)
    {
        OPTICK_EVENT();

        VulkanSwapchain* swapchain = &swapchains[index];

        uint32 semaphoreIndex = (swapchain->semaphoreIndex + 1) % swapchain->numSemaphores;
        VkSemaphore& imageAcquiredSemaphore = swapchain->imageAcquiredSemaphores[semaphoreIndex];
        VkFence& imageAcquiredFence = swapchain->imageAcquiredFences[semaphoreIndex];

        // TODO: investigate this
        if (vkGetFenceStatus(handle, imageAcquiredFence) == VK_NOT_READY)
        {
            VK_CHECK(vkWaitForFences(handle, 1, &imageAcquiredFence, VK_TRUE, UINT64_MAX));
        }
        VK_CHECK(vkResetFences(handle, 1, &imageAcquiredFence));

        uint32 imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(handle, swapchain->handle, UINT64_MAX, imageAcquiredSemaphore, imageAcquiredFence, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return VulkanSwapchain::Status::OutOfDate;
        }
        assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

        swapchain->semaphoreIndex = semaphoreIndex;
        swapchain->activeBackBufferIndex = imageIndex;

        return VulkanSwapchain::Status::Success;
    }

    VulkanSwapchain::Status VulkanDevice::PresentSwapChain(uint32 index, VkSemaphore* waitSemaphores, uint32 waitSemaphoreCount)
    {
        OPTICK_EVENT();

        VulkanSwapchain* swapchain = &swapchains[index];
        VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = waitSemaphoreCount,
            .pWaitSemaphores = waitSemaphores,
            .swapchainCount = 1,
            .pSwapchains = &swapchain->handle,
            .pImageIndices = &swapchain->activeBackBufferIndex,
        };
        VkQueue presentQueue = commandQueues[(uint32)RenderBackendQueueFamily::Graphics][0].handle;
        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return VulkanSwapchain::Status::OutOfDate;
        }
        assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);
        return VulkanSwapchain::Status::Success;
    }

    VkRenderPass VulkanDevice::FindOrCreateRenderPass(const VulkanRenderPassDesc& renderPassDesc)
    {
        VkRenderPass renderPass = VK_NULL_HANDLE;

        uint32 renderPassHash = renderPassDesc.renderPassFullHash;
        if (cachedRenderPasses.find(renderPassHash) != cachedRenderPasses.end())
        {
            renderPass = cachedRenderPasses[renderPassHash];
        }
        else
        {
            std::array<VkSubpassDependency, 2> subpassDependencies;
            subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            subpassDependencies[0].dstSubpass = 0;
            subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            subpassDependencies[1].srcSubpass = 0;
            subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            VkSubpassDescription subpassDesc = {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = renderPassDesc.numColorAttachments,
                .pColorAttachments = renderPassDesc.colorReferences,
                .pDepthStencilAttachment = renderPassDesc.hasDepthStencil ? &renderPassDesc.depthStencilReference : nullptr,
            };
            VkRenderPassCreateInfo renderPassInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = renderPassDesc.numAttachmentDescriptions,
                .pAttachments = renderPassDesc.attachmentDescriptions,
                .subpassCount = 1,
                .pSubpasses = &subpassDesc,
                .dependencyCount = (uint32)subpassDependencies.size(),
                .pDependencies = subpassDependencies.data(),
            };
            VK_CHECK(vkCreateRenderPass(handle, &renderPassInfo, VULKAN_ALLOCATION_CALLBACKS, &renderPass));
            cachedRenderPasses[renderPassHash] = renderPass;
        }

        return renderPass;
    }

    bool Matches(VulkanDevice* device, const VulkanFramebuffer& framebuffer, const RenderBackendRenderPassInfo& renderPassInfo, uint32 numColorAttachments)
    {
        if (framebuffer.numColorAttachments != numColorAttachments)
        {
            return false;
        }

        for (uint32 index = 0; index < framebuffer.numColorAttachments; index++)
        {
            VkImage image1 = framebuffer.images[index];
            VkImage image2 = device->GetTexture(renderPassInfo.colorRenderTargets[index].texture)->handle;
            if (image1 != image2)
            {
                return false;
            }
        }

        if ((framebuffer.numAttachments != framebuffer.numColorAttachments) && renderPassInfo.depthStencilRenderTarget.texture)
        {
            VkImage image1 = framebuffer.images[framebuffer.numColorAttachments];
            VkImage image2 = device->GetTexture(renderPassInfo.depthStencilRenderTarget.texture)->handle;
            if (image1 != image2)
            {
                return false;
            }
        }

        return true;
    }

    VulkanFramebuffer* VulkanDevice::FindOrCreateFramebuffer(const RenderBackendRenderPassInfo& renderPassInfo, const VulkanRenderPassDesc& renderPassDesc, VkRenderPass renderPass)
    {
        uint32 renderPassCompatibleHash = renderPassDesc.renderPassCompatibleHash;

        uint64 mipLevelsAndArrayLayers[RenderBackendMaxNumSimultaneousColorRenderTargets + 1];
        for (int32 index = 0; index < RenderBackendMaxNumSimultaneousColorRenderTargets; index++)
        {
            mipLevelsAndArrayLayers[index] = ((uint64)renderPassInfo.colorRenderTargets[index].arrayLayer << (uint64)32) | (uint64)renderPassInfo.colorRenderTargets[index].mipLevel;
        }
        mipLevelsAndArrayLayers[RenderBackendMaxNumSimultaneousColorRenderTargets] = ((uint64)renderPassInfo.depthStencilRenderTarget.arrayLayer << (uint64)32) | (uint64)renderPassInfo.depthStencilRenderTarget.mipLevel;
        uint32 framebufferHash = Crc32(mipLevelsAndArrayLayers, (RenderBackendMaxNumSimultaneousColorRenderTargets + 1) * sizeof(uint64), renderPassCompatibleHash);

        FramebufferList* framebufferList = nullptr;
        if (cachedFramebuffers.find(framebufferHash) != cachedFramebuffers.end())
        {
            framebufferList = &cachedFramebuffers[framebufferHash];
            for (uint64 index = 0; index < framebufferList->framebuffers.size(); index++)
            {
                if (Matches(this, framebufferList->framebuffers[index], renderPassInfo, renderPassDesc.numColorAttachments))
                {
                    return &framebufferList->framebuffers[index];
                }
            }
        }
        else
        {
            cachedFramebuffers.emplace(framebufferHash, FramebufferList{});
            framebufferList = &cachedFramebuffers[framebufferHash];
        }

        const VkExtent3D& extent = renderPassDesc.extent;
        uint32 width = extent.width;
        uint32 height = extent.height;
        uint32 layers = extent.depth;

        VulkanFramebuffer framebuffer = {
            .width = width,
            .height = height,
            .layers = layers,
        };

        uint32 numAttachments = renderPassDesc.numColorAttachments;

        for (uint32 index = 0; index < numAttachments; index++)
        {
            VulkanTexture* texture = GetTexture(renderPassInfo.colorRenderTargets[index].texture);
            assert(texture->width == width && texture->height == height);

            uint32 mipLevel = renderPassInfo.colorRenderTargets[index].mipLevel;
            uint32 arrayLayer = renderPassInfo.colorRenderTargets[index].arrayLayer;

            framebuffer.images[index] = texture->handle;
            framebuffer.attachments[index] = texture->rtv[mipLevel];
        }
        if (renderPassDesc.hasDepthStencil)
        {
            VulkanTexture* texture = GetTexture(renderPassInfo.depthStencilRenderTarget.texture);
            assert(texture->width == width && texture->height == height);

            uint32 mipLevel = renderPassInfo.depthStencilRenderTarget.mipLevel;
            uint32 arrayLayer = renderPassInfo.depthStencilRenderTarget.arrayLayer;

            bool hasStencil = IsStencilFormat(texture->format);

            framebuffer.images[numAttachments] = texture->handle;
            framebuffer.attachments[numAttachments] = texture->dsv[arrayLayer];
            numAttachments++;
        }
        framebuffer.numColorAttachments = renderPassDesc.numColorAttachments;
        framebuffer.numAttachments = numAttachments;

        assert(layers != (uint32)-1 && layers != 0);

        VkFramebufferCreateInfo frameBufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = numAttachments,
            .pAttachments = framebuffer.attachments,
            .width = width,
            .height = height,
            .layers = layers,
        };
        VK_CHECK(vkCreateFramebuffer(handle, &frameBufferInfo, VULKAN_ALLOCATION_CALLBACKS, &framebuffer.handle));

        framebufferList->framebuffers.emplace_back(framebuffer);

        return &framebufferList->framebuffers.back();
    }

    VkPipelineLayout VulkanDevice::FindOrCreatePipelineLayout(uint32 pushConstantSize, RenderBackendPipelineType pipelineType)
    {
        uint64 layoutHash = Crc32(&pushConstantSize, sizeof(uint32), (uint32)pipelineType);
        if (pipelineManager.pipelineLayoutMap.find(layoutHash) != pipelineManager.pipelineLayoutMap.end())
        {
            return pipelineManager.pipelineLayoutMap[layoutHash];
        }

        VkShaderStageFlags shaderStageFlags = 0;
        switch (pipelineType)
        {
        case RenderBackendPipelineType::Graphics:
            shaderStageFlags = VK_SHADER_STAGE_ALL;
            break;
        case RenderBackendPipelineType::Compute:
            shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            break;
        case RenderBackendPipelineType::RayTracing:
            shaderStageFlags = VK_SHADER_STAGE_ALL;
            break;
        default:
            std::unreachable();
            break;
        }
        VkPushConstantRange pushConstantRange = {
            .stageFlags = shaderStageFlags,
            .offset = 0,
            .size = pushConstantSize
        };
        VkPipelineLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &bindlessDescriptorManager.layout,
            .pushConstantRangeCount = pushConstantSize ? 1u : 0u,
            .pPushConstantRanges = pushConstantSize ? &pushConstantRange : nullptr,
        };
        VkPipelineLayout pipelineLayout;
        VK_CHECK(vkCreatePipelineLayout(handle, &layoutInfo, VULKAN_ALLOCATION_CALLBACKS, &pipelineLayout));

        pipelineManager.pipelineLayoutMap.emplace(layoutHash, pipelineLayout);
        pipelineManager.pipelineLayouts.push_back(pipelineLayout);

        return pipelineLayout;
    }

    VulkanPipeline* VulkanDevice::FindOrCreateComputePipeline(VulkanShader* shader, uint32 pushConstantSize)
    {
        uint32 pipelineHash = Crc32(shader, sizeof(VulkanShader), pushConstantSize);

        if (pipelineManager.pipelineMap.find(pipelineHash) != pipelineManager.pipelineMap.end())
        {
            return &pipelineManager.pipelineMap[pipelineHash];
        }

        VkPipelineLayout pipelineLayout = FindOrCreatePipelineLayout(pushConstantSize, RenderBackendPipelineType::Compute);

        shader->stages[0].pName = shader->entryPoints[2].c_str();
        VkComputePipelineCreateInfo computePipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shader->stages[0],
            .layout = pipelineLayout,
        };

        VkPipeline pipeline;
        VK_CHECK(vkCreateComputePipelines(handle, pipelineManager.pipelineCache, 1, &computePipelineInfo, VULKAN_ALLOCATION_CALLBACKS, &pipeline));
        printf("%s\n", shader->entryPoints[2].c_str());

        pipelineManager.pipelineMap.emplace(pipelineHash, VulkanPipeline{ pipelineHash, pipeline, pipelineLayout });
        pipelineManager.pipelines.push_back(VulkanPipeline{ pipelineHash, pipeline, pipelineLayout });

        return &pipelineManager.pipelineMap[pipelineHash];
    }

    VulkanPipeline* VulkanDevice::FindOrCreateGraphicsPipeline(VulkanShader* shader, const RenderBackendGraphicsPipelineState& pipelineState, uint32 pushConstantSize, RenderBackendPrimitiveTopology topology, bool useDynamicRendering, VulkanRenderingInfo* renderingInfo, VkRenderPass renderPass, uint32 activeColorAttachmentCount)
    {
        assert(useDynamicRendering ^ (renderPass != VK_NULL_HANDLE));

        uint32 colorAttachmentCount = useDynamicRendering ? renderingInfo->numColorAttachments : activeColorAttachmentCount;

        VulkanGraphicsPipelineStateDesc pipelineStateDesc = {};
        InitVkPipelineRasterizationStateCreateInfo(pipelineState.rasterizationState, pipelineStateDesc.rasterizationStateCreateInfo);
        InitVkPipelineDepthStencilStateCreateInfo(pipelineState.depthStencilState, pipelineStateDesc.depthStencilStateCreateInfo);
        InitVkPipelineColorBlendStateCreateInfo(pipelineState.colorBlendState, pipelineStateDesc.colorBlendAttachmentStates, colorAttachmentCount, pipelineStateDesc.colorBlendStateCreateInfo);

        // TODO: Optimize this
        uint64 renderingInfoFullHash = renderingInfo ? Crc32(renderingInfo, sizeof(VulkanRenderingInfo)) : uint64(renderPass);
        uint64 pipelineStateDescHash = Crc32(&pipelineStateDesc, sizeof(VulkanGraphicsPipelineStateDesc));
        uint64 values[] = { renderingInfoFullHash, pipelineStateDescHash, uint64(shader), uint64(topology), uint64(pushConstantSize) };
        uint64 pipelineHash = uint64(Crc32(values, 5 * sizeof(uint64)));

        if (pipelineManager.pipelineMap.find(pipelineHash) != pipelineManager.pipelineMap.end())
        {
            return &pipelineManager.pipelineMap[pipelineHash];
        }

        VkPipelineLayout pipelineLayout = FindOrCreatePipelineLayout(pushConstantSize, RenderBackendPipelineType::Graphics);

        // TODO:
        bool useMeshShader = false;
        for (uint32 i = 0; i < shader->numStages; i++)
        {
            if (shader->stages[i].stage == VK_SHADER_STAGE_TASK_BIT_EXT)
            {
                shader->stages[i].pName = shader->entryPoints[8].c_str();
            }
            else if (shader->stages[i].stage == VK_SHADER_STAGE_MESH_BIT_EXT)
            {
                useMeshShader = true;
                shader->stages[i].pName = shader->entryPoints[9].c_str();
            }
            else if (shader->stages[i].stage == VK_SHADER_STAGE_VERTEX_BIT)
            {
                shader->stages[i].pName = shader->entryPoints[0].c_str();
            }
            else if (shader->stages[i].stage == VK_SHADER_STAGE_FRAGMENT_BIT)
            {
                shader->stages[i].pName = shader->entryPoints[1].c_str();
            }
        }

        static const VkPipelineViewportStateCreateInfo viewportStateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = nullptr,
            .scissorCount = 1,
            .pScissors = nullptr,
        };

        static const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        };

        static const VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        };

        VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = ARRAY_SIZE(dynamicStates),
            .pDynamicStates = dynamicStates,
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = ConvertToVkPrimitiveTopology(topology),
            .primitiveRestartEnable = (topology == RenderBackendPrimitiveTopology::TriangleStrip || topology == RenderBackendPrimitiveTopology::TriangleFan) ? VK_TRUE : VK_FALSE,
        };

        VkPipelineRasterizationStateCreateInfo& rasterizationStateInfo = pipelineStateDesc.rasterizationStateCreateInfo;

        VkPipelineDepthStencilStateCreateInfo& depthStencilStateInfo = pipelineStateDesc.depthStencilStateCreateInfo;

        VkPipelineColorBlendStateCreateInfo& colorBlendStateInfo = pipelineStateDesc.colorBlendStateCreateInfo;

        VkSampleMask sampleMask = 0xFFFFFFFF;

        VkPipelineMultisampleStateCreateInfo multisamplingStateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0.0f,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
        };

        VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = useDynamicRendering ? &renderingInfo->pipelineRenderingInfo : nullptr,
            .stageCount = shader->numStages,
            .pStages = shader->stages,
            .pVertexInputState = useMeshShader ? nullptr : &vertexInputStateInfo,
            .pInputAssemblyState = useMeshShader ? nullptr : &inputAssemblyStateInfo,
            .pViewportState = &viewportStateInfo,
            .pRasterizationState = &rasterizationStateInfo,
            .pMultisampleState = &multisamplingStateInfo,
            .pDepthStencilState = &depthStencilStateInfo,
            .pColorBlendState = &colorBlendStateInfo,
            .pDynamicState = &dynamicStateInfo,
            .layout = pipelineLayout,
            .renderPass = useDynamicRendering ? VK_NULL_HANDLE : renderPass,
            .subpass = 0
        };

        VkPipeline pipeline;
        VK_CHECK(vkCreateGraphicsPipelines(handle, pipelineManager.pipelineCache, 1, &graphicsPipelineInfo, VULKAN_ALLOCATION_CALLBACKS, &pipeline));

        pipelineManager.pipelineMap.emplace(pipelineHash, VulkanPipeline{ pipelineHash, pipeline, pipelineLayout });
        pipelineManager.pipelines.push_back(VulkanPipeline{ pipelineHash, pipeline, pipelineLayout });

        return &pipelineManager.pipelineMap[pipelineHash];
    }

    void VulkanDevice::RecreateSwapChain(uint32 index)
    {
        vkDeviceWaitIdle(handle);

        VulkanSwapchain& swapchain = swapchains[index];

        for (uint32 i = 0; i < swapchain.numBuffers; i++)
        {
            VulkanTexture* texture = GetTexture(swapchain.buffers[i]);
            texture = {};
            RemoveRenderBackendHandleRepresentation(swapchain.buffers[i].GetIndex());
        }
        for (uint32 i = 0; i < swapchain.numSemaphores; i++)
        {
            vkWaitForFences(handle, 1, &swapchain.imageAcquiredFences[i], VK_TRUE, UINT64_MAX);
            vkDestroyFence(handle, swapchain.imageAcquiredFences[i], VULKAN_ALLOCATION_CALLBACKS);
            vkDestroySemaphore(handle, swapchain.imageAcquiredSemaphores[i], VULKAN_ALLOCATION_CALLBACKS);
        }
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->handle, swapchain.surface, &surfaceCapabilities));

        swapchain.info.imageExtent = surfaceCapabilities.currentExtent;
        swapchain.info.oldSwapchain = swapchain.handle;
        VK_CHECK(vkCreateSwapchainKHR(handle, &swapchain.info, VULKAN_ALLOCATION_CALLBACKS, &swapchain.handle));
        vkDestroySwapchainKHR(handle, swapchain.info.oldSwapchain, VULKAN_ALLOCATION_CALLBACKS);

        VK_CHECK(vkGetSwapchainImagesKHR(handle, swapchain.handle, &swapchain.numBuffers, nullptr));
        VkImage swapchainImages[RenderBackendMaxNumSwapChainBuffers] = { 0 };
        VK_CHECK(vkGetSwapchainImagesKHR(handle, swapchain.handle, &swapchain.numBuffers, swapchainImages));

        VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
        };
        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreTypeCreateInfo,
        };
        VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        for (uint32 i = 0; i < swapchain.numSemaphores; i++)
        {
            VK_CHECK(vkCreateSemaphore(handle, &semaphoreCreateInfo, VULKAN_ALLOCATION_CALLBACKS, &swapchain.imageAcquiredSemaphores[i]));
            VK_CHECK(vkCreateFence(handle, &fenceInfo, VULKAN_ALLOCATION_CALLBACKS, &swapchain.imageAcquiredFences[i]));
        }

        for (uint32 i = 0; i < swapchain.numBuffers; i++)
        {
            VulkanTexture texture = {
                .handle = swapchainImages[i],
                .swapchainBuffer = true,
                .width = swapchain.info.imageExtent.width,
                .height = swapchain.info.imageExtent.height,
                .depth = 1,
                .arrayLayers = 1,
                .mipLevels = 1,
                .format = swapchain.info.imageFormat,
                .type = VK_IMAGE_TYPE_2D,
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .clearValue = { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
            };

            uint32 textureIndex = 0;
            if (!freeTextures.empty())
            {
                textureIndex = freeTextures.back();
                freeTextures.pop_back();
                textures[textureIndex] = texture;
            }
            else
            {
                textureIndex = (uint32)textures.size();
                textures.emplace_back(texture);
            }
            swapchain.buffers[i] = backend->handleManager.Allocate<RenderBackendTextureHandle>(deviceMask);
            SetRenderBackendHandleRepresentation(swapchain.buffers[i].GetIndex(), textureIndex);
        }
        AcquireImageIndex(index);
    }

    RenderBackendTextureHandle VulkanDevice::GetActiveSwapChainBackBuffer(uint32 index)
    {
        return swapchains[index].buffers[swapchains[index].activeBackBufferIndex];
    }

    static VkSurfaceKHR CreateSurface(VulkanRenderBackend* backend, VkInstance instance, uint64 window)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(NULL),
            .hwnd = (HWND)window,
        };
        VK_CHECK(vkCreateWin32SurfaceKHR(instance, &win32SurfaceInfo, VULKAN_ALLOCATION_CALLBACKS, &surface));
        return surface;
    }

    uint32 VulkanDevice::CreateSwapChain(const RenderBackendSwapChainDesc* desc)
    {
        VulkanSwapchain swapchain;

        VkSurfaceKHR surface = CreateSurface(backend, instance, desc->windowHandle);
        uint32 presentQueueFamilyIndex = physicalDevice->queueFamilyIndices[(uint32)RenderBackendQueueFamily::Graphics];

        VkBool32 supported = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice->handle, presentQueueFamilyIndex, surface, &supported));
        assert(supported == VK_TRUE);

        uint32 presentModeCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice->handle, surface, &presentModeCount, nullptr));
        std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice->handle, surface, &presentModeCount, availablePresentModes.data()));

        uint32 numSurfaceFormats;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->handle, surface, &numSurfaceFormats, nullptr));
        std::vector<VkSurfaceFormatKHR> availableSurfaceFormats(numSurfaceFormats);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->handle, surface, &numSurfaceFormats, availableSurfaceFormats.data()));

        VkSurfaceFormatKHR surfaceFormat;
        for (const auto& format : availableSurfaceFormats)
        {
            if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surfaceFormat = format;
                break;
            }
        }
        for (const auto& format : availableSurfaceFormats)
        {
            if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == ConvertToVkFormat(desc->format))
            {
                surfaceFormat = format;
                break;
            }
        }

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->handle, surface, &surfaceCapabilities));
        assert(surfaceCapabilities.supportedUsageFlags & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
        VkSurfaceTransformFlagBitsKHR preTransform;
        if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            preTransform = surfaceCapabilities.currentTransform;
        }
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        {
            compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }

        uint32 imageCount = std::max(std::min(desc->numBuffers, surfaceCapabilities.maxImageCount), surfaceCapabilities.minImageCount);

        swapchain.info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = surfaceCapabilities.currentExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &presentQueueFamilyIndex,
            .preTransform = preTransform,
            .compositeAlpha = compositeAlpha,
            .presentMode = (VkPresentModeKHR)desc->presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };
        swapchain.surface = surface;
        VK_CHECK(vkCreateSwapchainKHR(handle, &swapchain.info, VULKAN_ALLOCATION_CALLBACKS, &swapchain.handle));

        VK_CHECK(vkGetSwapchainImagesKHR(handle, swapchain.handle, &swapchain.numBuffers, nullptr));
        VkImage swapchainImages[RenderBackendMaxNumSwapChainBuffers] = { 0 };
        VK_CHECK(vkGetSwapchainImagesKHR(handle, swapchain.handle, &swapchain.numBuffers, swapchainImages));

        VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
        };
        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreTypeCreateInfo,
        };
        VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        swapchain.numSemaphores = swapchain.numBuffers + 1;
        for (uint32 i = 0; i < swapchain.numSemaphores; i++)
        {
            VK_CHECK(vkCreateSemaphore(handle, &semaphoreCreateInfo, VULKAN_ALLOCATION_CALLBACKS, &swapchain.imageAcquiredSemaphores[i]));
            VK_CHECK(vkCreateFence(handle, &fenceInfo, VULKAN_ALLOCATION_CALLBACKS, &swapchain.imageAcquiredFences[i]));
        }

        for (uint32 i = 0; i < swapchain.numBuffers; i++)
        {
            VulkanTexture texture = {
                .handle = swapchainImages[i],
                .swapchainBuffer = true,
                .width = swapchain.info.imageExtent.width,
                .height = swapchain.info.imageExtent.height,
                .depth = 1,
                .arrayLayers = 1,
                .mipLevels = 1,
                .format = swapchain.info.imageFormat,
                .type = VK_IMAGE_TYPE_2D,
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .clearValue = {.color = {.float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
            };

            uint32 textureIndex = 0;
            if (!freeTextures.empty())
            {
                textureIndex = freeTextures.back();
                freeTextures.pop_back();
                textures[textureIndex] = texture;
            }
            else
            {
                textureIndex = (uint32)textures.size();
                textures.emplace_back(texture);
            }
            swapchain.buffers[i] = backend->handleManager.Allocate<RenderBackendTextureHandle>(deviceMask);
            SetRenderBackendHandleRepresentation(swapchain.buffers[i].GetIndex(), textureIndex);
        }

        uint32 swapchainIndex = (uint32)swapchains.size();
        swapchains.emplace_back(swapchain);
        presentSemaphores.resize(swapchains.size());

        AcquireImageIndex(swapchainIndex);

        return swapchainIndex;
    }

    void VulkanDevice::DestroySwapChain(uint32 index)
    {
        VulkanSwapchain& swapchain = swapchains[index];
        vkDeviceWaitIdle(handle);
        vkDestroySwapchainKHR(handle, swapchain.handle, VULKAN_ALLOCATION_CALLBACKS);
        vkDestroySurfaceKHR(instance, swapchain.surface, VULKAN_ALLOCATION_CALLBACKS);
        for (uint32 i = 0; i < swapchain.numSemaphores; i++)
        {
            vkDestroyFence(handle, swapchain.imageAcquiredFences[i], VULKAN_ALLOCATION_CALLBACKS);
            vkDestroySemaphore(handle, swapchain.imageAcquiredSemaphores[i], VULKAN_ALLOCATION_CALLBACKS);
        }
        swapchains.erase(swapchains.begin() + index);
    }

    VulkanDevice::VulkanDevice()
        : backend(nullptr)
        , physicalDevice(nullptr)
        , instance(VK_NULL_HANDLE)
        , handle(VK_NULL_HANDLE)
        , vmaAllocator(VK_NULL_HANDLE)
        , bindlessDescriptorManager()
    {
        assert(deviceMask == 0);
        assert(handle == VK_NULL_HANDLE);
    }

    VulkanDevice::~VulkanDevice()
    {
        assert(deviceMask == 0);
        assert(handle == VK_NULL_HANDLE);
    }

//#if HE_ENBALE_STREAMLINE_SUPPORT
//    void NVSDK_CONV NGXLogSink(const char* InNGXMessage, NVSDK_NGX_Logging_Level InLoggingLevel, NVSDK_NGX_Feature InSourceComponent)
//    {
//        const char* NGXComponent = "Unknown";
//        switch (InSourceComponent)
//        {
//        case NVSDK_NGX_Feature_SuperSampling: NGXComponent = "DLSS";    break;
//        case NVSDK_NGX_Feature_Reserved_SDK:  NGXComponent = "SDK";        break;
//        case NVSDK_NGX_Feature_Reserved_Core: NGXComponent = "Core";    break;
//        }
//
//        LogVerbose(GLogger, std::format("[{}]: {}", NGXComponent, InNGXMessage));
//    }
//#endif

    bool VulkanDevice::Init(VulkanRenderBackend* backend, VulkanPhysicalDevice* physicalDevice, const VulkanBindlessConfig& bindlessConfig)
    {
        this->backend = backend;
        this->physicalDevice = physicalDevice;
        this->instance = backend->instance;
        this->deviceMask = ~uint32(0);

//#if HE_ENBALE_STREAMLINE_SUPPORT
//        NVSDK_NGX_FeatureDiscoveryInfo featureDiscoveryInfo = {};
//        uint32 extensionCount = 0;
//        NGX_CHECK(NVSDK_NGX_VULKAN_GetFeatureDeviceExtensionRequirements(instance, physicalDevice->handle, nullptr, &extensionCount, nullptr));
//        //std::vector<VkExtensionProperties*> extensionProperties(extensionCount);
//        //NGX_CHECK(NVSDK_NGX_VULKAN_GetFeatureDeviceExtensionRequirements(instance, physicalDevice->handle, nullptr, &extensionCount, extensionProperties.data()));
//        VkExtensionProperties* extensionProperties = nullptr;
//        NGX_CHECK(NVSDK_NGX_VULKAN_GetFeatureDeviceExtensionRequirements(instance, physicalDevice->handle, &featureDiscoveryInfo, &extensionCount, &extensionProperties));
//#endif

        // Create logical device
        {
            std::vector<const char*> requiredDeviceExtensions;
            requiredDeviceExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_DEVICE_GROUP_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_PRESENT_WAIT_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_PRESENT_ID_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME);

#if HE_ENBALE_STREAMLINE_SUPPORT
            requiredDeviceExtensions.push_back(VK_NVX_IMAGE_VIEW_HANDLE_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_NVX_BINARY_IMPORT_EXTENSION_NAME);

            requiredDeviceExtensions.push_back(VK_NV_OPTICAL_FLOW_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
#endif

            if (backend->enableRayTracingSupport)
            {
                requiredDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                requiredDeviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
                requiredDeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
                requiredDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
                requiredDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
                requiredDeviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
            }

            for (const auto& requiredExtension : requiredDeviceExtensions)
            {
                if (CheckInstanceExtensionSupport(requiredExtension, physicalDevice->extensionProperties))
                {
                    enabledDeviceExtensions.push_back(requiredExtension);
                    LogInfo(GLogger, std::format("Enabled device extension: {}.", requiredExtension));
                }
                else
                {
                    LogError(GLogger, std::format("The device doesn't support the required extension: {}.", requiredExtension));
                    return false;
                }
            }

            std::vector<VkDeviceQueueCreateInfo> queueInfos = {};
            std::vector<float> queuePriorities[RENDER_BACKEND_NUM_QUEUE_FAMILIES];
            for (uint32 family = 0; family < RENDER_BACKEND_NUM_QUEUE_FAMILIES; family++)
            {
                const uint32 queueCount = numCommandQueues[family];
                // Set all priorities to 1.0 for now.
                queuePriorities[family].resize(queueCount, 1.0f);
                commandQueues[family].resize(queueCount);

                VkDeviceQueueCreateInfo queueInfo = {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = physicalDevice->queueFamilyIndices[family],
                    .queueCount = queueCount,
                    .pQueuePriorities = queuePriorities[family].data()
                };
                if (queueInfo.queueCount > 0)
                {
                    queueInfos.push_back(queueInfo);
                }
            }

            VkDeviceCreateInfo deviceInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = physicalDevice->featuresEntry, // Vulkan 1.2
                .queueCreateInfoCount = (uint32)(queueInfos.size()),
                .pQueueCreateInfos = queueInfos.data(),
                .enabledExtensionCount = (uint32)(enabledDeviceExtensions.size()),
                .ppEnabledExtensionNames = enabledDeviceExtensions.data(),
                .pEnabledFeatures = &physicalDevice->enabledFeatures
            };

            VK_CHECK(vkCreateDevice(physicalDevice->handle, &deviceInfo, VULKAN_ALLOCATION_CALLBACKS, &handle));

            SetDebugUtilsObjectName(VK_OBJECT_TYPE_DEVICE, (uint64)handle, physicalDevice->properties.deviceName);
        }

#if HE_ENBALE_STREAMLINE_SUPPORT
        {
            sl::VulkanInfo slVulkanInfo = {};
            slVulkanInfo.instance = instance;
            slVulkanInfo.device = handle;
            slVulkanInfo.physicalDevice = physicalDevice->handle;
            slVulkanInfo.computeQueueIndex = commandQueues[(uint32)RenderBackendQueueFamily::Compute][0].queueIndex;
            slVulkanInfo.computeQueueFamily = commandQueues[(uint32)RenderBackendQueueFamily::Compute][0].familyIndex;
            slVulkanInfo.graphicsQueueIndex = commandQueues[(uint32)RenderBackendQueueFamily::Graphics][0].queueIndex;
            slVulkanInfo.graphicsQueueFamily = commandQueues[(uint32)RenderBackendQueueFamily::Graphics][0].familyIndex;
            slVulkanInfo.opticalFlowQueueIndex = commandQueues[(uint32)RenderBackendQueueFamily::OpticalFlow][0].queueIndex;
            slVulkanInfo.opticalFlowQueueFamily = commandQueues[(uint32)RenderBackendQueueFamily::OpticalFlow][0].familyIndex;
            sl::Result slResult = slSetVulkanInfo(slVulkanInfo);

            if (true)
            {
                // Set reflex consts to a default config. This can be changed at runtime in the UI.
                sl::ReflexOptions reflexOptions = {};
                reflexOptions.mode = sl::ReflexMode::eLowLatency;
                reflexOptions.frameLimitUs = 0;
                reflexOptions.useMarkersToOptimize = true;
                //reflexOptions.virtualKey = VK_F13;
                if (SL_FAILED(result, slReflexSetOptions(reflexOptions)))
                {
                    LogError(GLogger, std::format("slReflexSetOptions, error code: {}", (int32)result));
                }
            }
        }
        //// Init NGX
        //{
        //    bool bNGXInitialized = false;
        //    const wchar_t* path = L"TODO";
        //    const std::wstring wpath(path);

        //    NVSDK_NGX_FeatureCommonInfo FeatureInfo = {};
        //    /*    FeatureInfo.PathListInfo.Path = const_cast<wchar_t**>(&path);
        //        FeatureInfo.PathListInfo.Length = wpath.size();*/
        //        // logging
        //    {
        //        FeatureInfo.LoggingInfo.DisableOtherLoggingSinks = true;
        //        FeatureInfo.LoggingInfo.LoggingCallback = &NGXLogSink;
        //        FeatureInfo.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_VERBOSE;
        //    }

        //    NVSDK_NGX_Result ResultNGXInit = NVSDK_NGX_VULKAN_Init_with_ProjectID(
        //        "a0f57b54-1daf-4934-90ae-c4035c19df04",
        //        NVSDK_NGX_ENGINE_TYPE_CUSTOM,
        //        "HorizonEngine",
        //        path,
        //        instance,
        //        physicalDevice->handle,
        //        handle,
        //        nullptr,
        //        nullptr,
        //        &FeatureInfo,
        //        NVSDK_NGX_Version_API);
        //    LogInfo(GLogger, std::format("NVSDK_NGX_VULKAN_Init_with_ProjectID {}", (int)ResultNGXInit));

        //    /*
        //    const wchar_t* path = L"TODO";
        //    NVSDK_NGX_Result ResultNGXInit = NVSDK_NGX_VULKAN_Init_with_ProjectID(
        //        "a0f57b54-1daf-4934-90ae-c4035c19df04",
        //        NVSDK_NGX_ENGINE_TYPE_CUSTOM,
        //        "HorizonEngine",
        //        path,
        //        instance,
        //        physicalDevice->handle,
        //        handle);
        //    LogInfo(GLogger, std::format("NVSDK_NGX_VULKAN_Init_with_ProjectID {}", (int)ResultNGXInit));*/

        //    assert(NVSDK_NGX_SUCCEED(ResultNGXInit));

        //    if (NVSDK_NGX_SUCCEED(ResultNGXInit))
        //    {
        //        bNGXInitialized = true;

        //        NVSDK_NGX_Parameter* CapabilityParameters = nullptr;

        //        NVSDK_NGX_Result ResultGetCapabilityParameters = NVSDK_NGX_VULKAN_GetCapabilityParameters(&CapabilityParameters);
        //        LogInfo(GLogger, std::format("NVSDK_NGX_VULKAN_GetCapabilityParameters {}", (int)ResultGetCapabilityParameters));

        //        if (NVSDK_NGX_SUCCEED(ResultGetCapabilityParameters)) // Query DLSS Support
        //        {
        //            assert(CapabilityParameters != nullptr);

        //            int NeedsUpdatedDriver = 0;
        //            unsigned int MinDriverVersionMajor = 0;
        //            unsigned int MinDriverVersionMinor = 0;

        //            NVSDK_NGX_Result ResultUpdatedDriver = CapabilityParameters->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &NeedsUpdatedDriver);
        //            NVSDK_NGX_Result ResultMinDriverVersionMajor = CapabilityParameters->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &MinDriverVersionMajor);
        //            NVSDK_NGX_Result ResultMinDriverVersionMinor = CapabilityParameters->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor, &MinDriverVersionMinor);

        //            LogInfo(GLogger, std::format("NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver {} {}", (int)ResultUpdatedDriver, NeedsUpdatedDriver));
        //            LogInfo(GLogger, std::format("NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor {} {}", (int)ResultMinDriverVersionMajor, MinDriverVersionMajor));
        //            LogInfo(GLogger, std::format("NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor {} {}", (int)ResultMinDriverVersionMinor, MinDriverVersionMinor));

        //            if (NVSDK_NGX_SUCCEED(ResultUpdatedDriver))
        //            {
        //                if (NeedsUpdatedDriver)
        //                {
        //                    // NVIDIA DLSS cannot be loaded due to outdated driver.
        //                    if (NVSDK_NGX_SUCCEED(ResultMinDriverVersionMajor) && NVSDK_NGX_SUCCEED(ResultMinDriverVersionMinor))
        //                    {
        //                        // Min Driver Version required: minDriverVersionMajor.minDriverVersionMinor
        //                        LogWarning(GLogger, std::format("NVIDIA DLSS cannot be loaded due to outdated driver."));
        //                    }
        //                }
        //                else
        //                {
        //                    // driver update is not required �C so application is not expected to
        //                    // query minDriverVersion in this case
        //                }
        //            }
        //            else
        //            {
        //                LogInfo(GLogger, std::format("NVIDIA NGX DLSS Minimum driver version was not reported"));
        //            }

        //            int DlssAvailable = 0;
        //            NVSDK_NGX_Result ResultDlssAvailable = CapabilityParameters->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &DlssAvailable);
        //            LogInfo(GLogger, std::format("NVSDK_NGX_Parameter_SuperSampling_Available {} {}", (int)ResultDlssAvailable, NeedsUpdatedDriver));

        //            if (NVSDK_NGX_FAILED(ResultDlssAvailable) || !DlssAvailable)
        //            {
        //                LogInfo(GLogger, std::format("NVIDIA DLSS not available on this hardward/platform."));
        //            }

        //            NVSDK_NGX_Result ResultFeatureInitResult = CapabilityParameters->Get(NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult, &DlssAvailable);
        //            if (NVSDK_NGX_FAILED(ResultFeatureInitResult) || !DlssAvailable)
        //            {
        //                LogInfo(GLogger, std::format("NVIDIA DLSS is denied on for this application."));
        //            }
        //        }

        //        unsigned int RenderWidth, RenderHeight;
        //        float Sharpness = 0.0f;
        //        unsigned int TargetWidth = 1280;
        //        unsigned int TargetHeight = 720;
        //        unsigned int RecommendedOptimalRenderWidth = 0;
        //        unsigned int RecommendedOptimalRenderHeight = 0;
        //        unsigned int DynamicMaximumRenderSizeWidth = 0;
        //        unsigned int DynamicMaximumRenderSizeHeight = 0;
        //        unsigned int DynamicMinimumRenderSizeWidth = 0;
        //        unsigned int DynamicMinimumRenderSizeHeight = 0;
        //        NVSDK_NGX_PerfQuality_Value PerfQualityValue = NVSDK_NGX_PerfQuality_Value_MaxPerf;
        //        NVSDK_NGX_Result DLSSMode = NGX_DLSS_GET_OPTIMAL_SETTINGS(
        //            CapabilityParameters,
        //            TargetWidth, TargetHeight,
        //            PerfQualityValue,
        //            &RecommendedOptimalRenderWidth, &RecommendedOptimalRenderHeight,
        //            &DynamicMaximumRenderSizeWidth, &DynamicMaximumRenderSizeHeight,
        //            &DynamicMinimumRenderSizeWidth, &DynamicMinimumRenderSizeHeight,
        //            &Sharpness);

        //        if (RecommendedOptimalRenderWidth == 0 || RecommendedOptimalRenderHeight == 0)
        //        {
        //            // This PerfQuality mode has not been made available yet.
        //            // Please request another PerfQuality mode.
        //        }
        //        else
        //        {
        //            LogInfo(GLogger, std::format("NVSDK_NGX_PerfQuality_Value_MaxPerf"));
        //            LogInfo(GLogger, std::format("TargetWidth {} TargetHeight {}", TargetWidth, TargetHeight));
        //            LogInfo(GLogger, std::format("DynamicMaximumRenderSizeWidth {} DynamicMaximumRenderSizeHeight {}", DynamicMaximumRenderSizeWidth, DynamicMaximumRenderSizeHeight));
        //            LogInfo(GLogger, std::format("DynamicMinimumRenderSizeWidth {} DynamicMinimumRenderSizeHeight {}", DynamicMinimumRenderSizeWidth, DynamicMinimumRenderSizeHeight));
        //            // Use DLSS for this combination
        //            // - Create feature with RecommendedOptimalRenderWidth, RecommendedOptimalRenderHeight
        //            // - Render to (RenderWidth, RenderHeight) between Min and Max inclusive
        //            // - Call DLSS to upscale to (TargetWidth, TargetHeight)
        //        }

        //        PerfQualityValue = NVSDK_NGX_PerfQuality_Value_Balanced;
        //        DLSSMode = NGX_DLSS_GET_OPTIMAL_SETTINGS(
        //            CapabilityParameters,
        //            TargetWidth, TargetHeight,
        //            PerfQualityValue,
        //            &RecommendedOptimalRenderWidth, &RecommendedOptimalRenderHeight,
        //            &DynamicMaximumRenderSizeWidth, &DynamicMaximumRenderSizeHeight,
        //            &DynamicMinimumRenderSizeWidth, &DynamicMinimumRenderSizeHeight,
        //            &Sharpness);
        //        LogInfo(GLogger, std::format("NVSDK_NGX_PerfQuality_Value_Balanced"));
        //        LogInfo(GLogger, std::format("TargetWidth {} TargetHeight {}", TargetWidth, TargetHeight));
        //        LogInfo(GLogger, std::format("DynamicMaximumRenderSizeWidth {} DynamicMaximumRenderSizeHeight {}", DynamicMaximumRenderSizeWidth, DynamicMaximumRenderSizeHeight));
        //        LogInfo(GLogger, std::format("DynamicMinimumRenderSizeWidth {} DynamicMinimumRenderSizeHeight {}", DynamicMinimumRenderSizeWidth, DynamicMinimumRenderSizeHeight));

        //        PerfQualityValue = NVSDK_NGX_PerfQuality_Value_MaxQuality;
        //        DLSSMode = NGX_DLSS_GET_OPTIMAL_SETTINGS(
        //            CapabilityParameters,
        //            TargetWidth, TargetHeight,
        //            PerfQualityValue,
        //            &RecommendedOptimalRenderWidth, &RecommendedOptimalRenderHeight,
        //            &DynamicMaximumRenderSizeWidth, &DynamicMaximumRenderSizeHeight,
        //            &DynamicMinimumRenderSizeWidth, &DynamicMinimumRenderSizeHeight,
        //            &Sharpness);
        //        LogInfo(GLogger, std::format("NVSDK_NGX_PerfQuality_Value_MaxQuality"));
        //        LogInfo(GLogger, std::format("TargetWidth {} TargetHeight {}", TargetWidth, TargetHeight));
        //        LogInfo(GLogger, std::format("DynamicMaximumRenderSizeWidth {} DynamicMaximumRenderSizeHeight {}", DynamicMaximumRenderSizeWidth, DynamicMaximumRenderSizeHeight));
        //        LogInfo(GLogger, std::format("DynamicMinimumRenderSizeWidth {} DynamicMinimumRenderSizeHeight {}", DynamicMinimumRenderSizeWidth, DynamicMinimumRenderSizeHeight));

        //        PerfQualityValue = NVSDK_NGX_PerfQuality_Value_UltraPerformance;
        //        DLSSMode = NGX_DLSS_GET_OPTIMAL_SETTINGS(
        //            CapabilityParameters,
        //            TargetWidth, TargetHeight,
        //            PerfQualityValue,
        //            &RecommendedOptimalRenderWidth, &RecommendedOptimalRenderHeight,
        //            &DynamicMaximumRenderSizeWidth, &DynamicMaximumRenderSizeHeight,
        //            &DynamicMinimumRenderSizeWidth, &DynamicMinimumRenderSizeHeight,
        //            &Sharpness);
        //        LogInfo(GLogger, std::format("NVSDK_NGX_PerfQuality_Value_UltraPerformance"));
        //        LogInfo(GLogger, std::format("TargetWidth {} TargetHeight {}", TargetWidth, TargetHeight));
        //        LogInfo(GLogger, std::format("DynamicMaximumRenderSizeWidth {} DynamicMaximumRenderSizeHeight {}", DynamicMaximumRenderSizeWidth, DynamicMaximumRenderSizeHeight));
        //        LogInfo(GLogger, std::format("DynamicMinimumRenderSizeWidth {} DynamicMinimumRenderSizeHeight {}", DynamicMinimumRenderSizeWidth, DynamicMinimumRenderSizeHeight));

        //        PerfQualityValue = NVSDK_NGX_PerfQuality_Value_UltraQuality;
        //        DLSSMode = NGX_DLSS_GET_OPTIMAL_SETTINGS(
        //            CapabilityParameters,
        //            TargetWidth, TargetHeight,
        //            PerfQualityValue,
        //            &RecommendedOptimalRenderWidth, &RecommendedOptimalRenderHeight,
        //            &DynamicMaximumRenderSizeWidth, &DynamicMaximumRenderSizeHeight,
        //            &DynamicMinimumRenderSizeWidth, &DynamicMinimumRenderSizeHeight,
        //            &Sharpness);
        //        LogInfo(GLogger, std::format("NVSDK_NGX_PerfQuality_Value_UltraQuality"));
        //        LogInfo(GLogger, std::format("TargetWidth {} TargetHeight {}", TargetWidth, TargetHeight));
        //        LogInfo(GLogger, std::format("DynamicMaximumRenderSizeWidth {} DynamicMaximumRenderSizeHeight {}", DynamicMaximumRenderSizeWidth, DynamicMaximumRenderSizeHeight));
        //        LogInfo(GLogger, std::format("DynamicMinimumRenderSizeWidth {} DynamicMinimumRenderSizeHeight {}", DynamicMinimumRenderSizeWidth, DynamicMinimumRenderSizeHeight));
        //    }
        //}
#endif

        // Init command queues
        {
            for (uint32 family = 0; family < RENDER_BACKEND_NUM_QUEUE_FAMILIES; family++)
            {
                for (uint32 queueIndex = 0; queueIndex < (uint32)commandQueues[family].size(); queueIndex++)
                {
                    VkQueue queueHandle;
                    vkGetDeviceQueue(handle, physicalDevice->queueFamilyIndices[family], queueIndex, &queueHandle);
                    commandQueues[family][queueIndex] = {
                        .handle = queueHandle,
                        .familyIndex = physicalDevice->queueFamilyIndices[family],
                        .queueIndex = queueIndex
                    };
                }
            }
        }

        commandBufferManager = new VulkanCommandBufferManager(this, RenderBackendQueueFamily::Graphics);

        CreateVmaAllocator();

        CreateBindlessDescriptorManager(bindlessConfig);

        CreateDefaultResources();

        return true;
    }


    void VulkanDevice::Shutdown()
    {
        WaitIdle();
        DestroyBindlessDescriptorManager();
        for (uint32 i = 0; i < (uint32)swapchains.size(); i++)
        {
            DestroySwapChain(i);
        }
        DestroyVmaAllocator();
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroyDevice(handle, VULKAN_ALLOCATION_CALLBACKS);
            handle = VK_NULL_HANDLE;
        }
        deviceMask = 0;
    }

    bool VulkanDevice::IsDeviceExtensionEnabled(const char* extension)
    {
        for (const auto& enabledExtension : enabledDeviceExtensions)
        {
            if (strcmp(extension, enabledExtension) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void VulkanDevice::SetDebugUtilsObjectName(VkObjectType type, uint64 handle, const char* name)
    {
        if (!name || !backend->IsInstanceExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT nameInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = type,
            .objectHandle = handle,
            .pObjectName = name
        };

        VK_CHECK(backend->functions.vkSetDebugUtilsObjectNameEXT(this->handle, &nameInfo));
    }

    bool VulkanDevice::CreateBindlessDescriptorManager(const VulkanBindlessConfig& bindlessConfig)
    {
        const uint32 maxNumSampledImages = physicalDevice->descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindSampledImages;
        const uint32 maxNumSamplers = physicalDevice->descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindSamplers;
        const uint32 maxNumStorageImage = physicalDevice->descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindStorageImages;
        const uint32 maxNumStorageBuffers = physicalDevice->descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindStorageBuffers;
        const uint32 maxNumAccellerationStructures = physicalDevice->accelerationStructureProperties.maxDescriptorSetAccelerationStructures;

        uint32 numSampledImages = Math::Min(bindlessConfig.numSampledImages, maxNumSampledImages);
        uint32 numSamplers = Math::Min(bindlessConfig.numSamplers, maxNumSamplers);
        uint32 numStorageImages = Math::Min(bindlessConfig.numStorageImages, maxNumStorageImage);
        uint32 numStorageBuffers = Math::Min(bindlessConfig.numStorageBuffers, maxNumStorageBuffers);
        uint32 numAccelerationStructures = Math::Min(bindlessConfig.numAccelerationStructures, maxNumAccellerationStructures);

        std::vector<VkDescriptorPoolSize> bindlessPoolSizes;
        std::vector<VkDescriptorSetLayoutBinding> bindlessDescriptorSetLayoutBindings;
        std::vector<VkDescriptorBindingFlags> bindlessDescriptorBindingFlags;
        if (backend->enableRayTracingSupport)
        {
            bindlessPoolSizes = {
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              numSampledImages          },
                { VK_DESCRIPTOR_TYPE_SAMPLER,                    numSamplers               },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              numStorageImages          },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             numStorageBuffers         },
                { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, numAccelerationStructures },
            };
            bindlessDescriptorSetLayoutBindings = {
                { .binding = BindlessBindingSamplers,               .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,                    .descriptorCount = numSamplers,               .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BindlessBindingSampledImages,          .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              .descriptorCount = numSampledImages,          .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BindlessBindingStroageImages,          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              .descriptorCount = numStorageImages,          .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BindlessBindingStroageBuffers,         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             .descriptorCount = numStorageBuffers,         .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BindlessBindingAccelerationStructures, .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, .descriptorCount = numAccelerationStructures, .stageFlags = VK_SHADER_STAGE_ALL },
            };
            bindlessDescriptorBindingFlags = {
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
            };
        }
        else
        {
            bindlessPoolSizes = {
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              numSampledImages          },
                { VK_DESCRIPTOR_TYPE_SAMPLER,                    numSamplers               },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              numStorageImages          },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             numStorageBuffers         },
            };
            bindlessDescriptorSetLayoutBindings = {
                { .binding = BindlessBindingSamplers,               .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,                    .descriptorCount = numSamplers,               .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BindlessBindingSampledImages,          .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              .descriptorCount = numSampledImages,          .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BindlessBindingStroageImages,          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              .descriptorCount = numStorageImages,          .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BindlessBindingStroageBuffers,         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             .descriptorCount = numStorageBuffers,         .stageFlags = VK_SHADER_STAGE_ALL },
            };
            bindlessDescriptorBindingFlags = {
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
            };
        }
        assert(bindlessDescriptorSetLayoutBindings.size() == bindlessDescriptorBindingFlags.size());

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1,
            .poolSizeCount = (uint32)bindlessPoolSizes.size(),
            .pPoolSizes = bindlessPoolSizes.data()
        };

        VkResult result = vkCreateDescriptorPool(handle, &descriptorPoolInfo, VULKAN_ALLOCATION_CALLBACKS, &bindlessDescriptorManager.pool);
        if (result != VK_SUCCESS)
        {
            LogError(GLogger, std::format("InitBindlessContext(): Failed to create descriptor pool."));
            return false;
        }

        uint32 numBindings = (uint32)bindlessDescriptorSetLayoutBindings.size();

        VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlagsInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = numBindings,
            .pBindingFlags = bindlessDescriptorBindingFlags.data()
        };

        const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &descriptorSetLayoutBindingFlagsInfo,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = numBindings,
            .pBindings = bindlessDescriptorSetLayoutBindings.data()
        };
        result = vkCreateDescriptorSetLayout(handle, &descriptorSetLayoutInfo, VULKAN_ALLOCATION_CALLBACKS, &bindlessDescriptorManager.layout);
        if (result != VK_SUCCESS)
        {
            LogError(GLogger, std::format("InitBindlessContext(): Failed to create descriptor set layout."));
            return false;
        }

        const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = bindlessDescriptorManager.pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &bindlessDescriptorManager.layout
        };
        result = vkAllocateDescriptorSets(handle, &descriptorSetAllocateInfo, &bindlessDescriptorManager.set);
        if (result != VK_SUCCESS)
        {
            LogError(GLogger, std::format("InitBindlessContext(): Failed to allocate descriptor set."));
            return false;
        }

        // To be able to bind a set once in a frame for all shaders, all pipeline layouts have to be compatible
        bindlessDescriptorManager.pushConstantSize = 128;
        bindlessDescriptorManager.compatibleGraphicsPipelineLayout = FindOrCreatePipelineLayout(bindlessDescriptorManager.pushConstantSize, RenderBackendPipelineType::Graphics);
        bindlessDescriptorManager.compatibleComputePipelineLayout = FindOrCreatePipelineLayout(bindlessDescriptorManager.pushConstantSize, RenderBackendPipelineType::Compute);
        if (backend->enableRayTracingSupport)
        {
            bindlessDescriptorManager.compatibleRayTracingPipelineLayout = FindOrCreatePipelineLayout(bindlessDescriptorManager.pushConstantSize, RenderBackendPipelineType::RayTracing);
        }

        bindlessDescriptorManager.config = {
            .numSamplers = numSamplers,
            .numSampledImages = numSampledImages,
            .numStorageImages = numStorageImages,
            .numStorageBuffers = numStorageBuffers,
            .numAccelerationStructures = numAccelerationStructures,
        };

        for (int32 i = numSampledImages - 1; i >= 0; i--)
        {
            bindlessDescriptorManager.freeSampledImages.push_back(i);
        }
        for (int32 i = numSamplers - 1; i >= 0; i--)
        {
            bindlessDescriptorManager.freeSamplers.push_back(i);
        }
        for (int32 i = numStorageImages - 1; i >= 0; i--)
        {
            bindlessDescriptorManager.freeStorageImages.push_back(i);
        }
        for (int32 i = numStorageBuffers - 1; i >= 0; i--)
        {
            bindlessDescriptorManager.freeStorageBuffers.push_back(i);
        }
        for (int32 i = numAccelerationStructures - 1; i >= 0; i--)
        {
            bindlessDescriptorManager.freeAccelerationStructures.push_back(i);
        }

        return true;
    }

    void VulkanDevice::DestroyBindlessDescriptorManager()
    {
        if (bindlessDescriptorManager.pool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(handle, bindlessDescriptorManager.pool, VULKAN_ALLOCATION_CALLBACKS);
            bindlessDescriptorManager.pool = VK_NULL_HANDLE;
        }
        if (bindlessDescriptorManager.layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(handle, bindlessDescriptorManager.layout, VULKAN_ALLOCATION_CALLBACKS);
            bindlessDescriptorManager.layout = VK_NULL_HANDLE;
        }
    }

    void VulkanDevice::CreateDefaultResources()
    {
        // Sampled images
        //for (uint32 i = 0; i != (uint32)TextureType::Count; i++)
        //{
        //    defaultSampledImages[i] = CreateImage();
        //    VkImageView defaultSampledImage = images[defaultSampledImage[i].GetIndex()].views[0];
        //}

        //// Sampler
        //{
        //    VkSamplerCreateInfo defaultSamplerInfo = {
        //        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        //        .magFilter = VK_FILTER_LINEAR,
        //        .minFilter = VK_FILTER_LINEAR,
        //        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        //        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        //        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        //        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        //        .mipLodBias = 0.f,
        //        .anisotropyEnable = VK_TRUE,
        //        .maxAnisotropy = 16.f,
        //        .compareEnable = VK_FALSE,
        //        .compareOp = VK_COMPARE_OP_NEVER,
        //        .minLod = 0.f,
        //        .maxLod = 16.f,
        //        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        //        .unnormalizedCoordinates = VK_FALSE
        //    };
        //    defaultSampler = CreateSampler(&defaultSamplerInfo, "Default Sampler");
        //    VkSampler sampler = samplers[defaultSampler.GetIndex()].handle;
        //    VkDescriptorImageInfo sampleInfo = {
        //        .sampler = sampler,
        //        .imageView = VK_NULL_HANDLE,
        //        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
        //    };
        //    VkWriteDescriptorSet writeDescriptorSet = {
        //        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //        .dstSet = bindlessManager->set,
        //        .dstBinding = VULKAN_BINDLESS_DESCRIPTOR_SLOT_SAMPLERS,
        //        .dstArrayElement = 0,
        //        .descriptorCount = 1,
        //        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        //        .pImageInfo = &sampleInfo,
        //    };
        //    vkUpdateDescriptorSets(handle, 1, &writeDescriptorSet, 0, 0);
        //}

        // Storage Images
        /*for (uint32 i = 0; i != (uint32)TextureType::Count; i++)
        {
            VkImageView defaultStorageImage = images[device->resource_manager.handle_indirection[decode_index(device->null_uav_images[i])]].views[0];
            VkDescriptorImageInfo imageInfo = {
                .sampler = VK_NULL_HANDLE,
                .imageView = defaultStorageImage,
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            };
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = bindlessManager->set,
                .dstBinding = VULKAN_BINDLESS_DESCRIPTOR_SLOT_STORAGE_IMAGES,
                .dstArrayElement = i,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &imageInfo,
            };
            vkUpdateDescriptorSets(handle, 1, &write, 0, 0);
        }*/

        // Storage buffers
        {
            char data[512];
            memset(data, 0xff, 512);
            RenderBackendBufferDesc desc = RenderBackendBufferDesc::CreateByteAddress(512);
            uint32 bufferIndex = CreateBuffer(&desc, data, "Default Storage Buffer");
        }
    }

    void VulkanDevice::CreateVmaAllocator()
    {
        VmaVulkanFunctions vmaVulkanFunctions = {
            .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
            .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
            .vkAllocateMemory = vkAllocateMemory,
            .vkFreeMemory = vkFreeMemory,
            .vkMapMemory = vkMapMemory,
            .vkUnmapMemory = vkUnmapMemory,
            .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
            .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
            .vkBindBufferMemory = vkBindBufferMemory,
            .vkBindImageMemory = vkBindImageMemory,
            .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
            .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
            .vkCreateBuffer = vkCreateBuffer,
            .vkDestroyBuffer = vkDestroyBuffer,
            .vkCreateImage = vkCreateImage,
            .vkDestroyImage = vkDestroyImage,
            .vkCmdCopyBuffer = vkCmdCopyBuffer,
        };

        VmaAllocatorCreateFlags flags = 0;
        if (IsDeviceExtensionEnabled(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) && IsDeviceExtensionEnabled(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
        {
            flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
            vmaVulkanFunctions.vkGetBufferMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetBufferMemoryRequirements2KHR>(vkGetInstanceProcAddr(instance, "vkGetBufferMemoryRequirements2KHR"));
            vmaVulkanFunctions.vkGetImageMemoryRequirements2KHR = reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>(vkGetInstanceProcAddr(instance, "vkGetImageMemoryRequirements2KHR"));
        }
        else
        {
            LogWarning(GLogger, std::format("Missing device extensions: {} and {}.", VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME));
        }

        if (IsDeviceExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
        {
            flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        VmaAllocatorCreateInfo allocatorInfo = {
            .flags = flags,
            .physicalDevice = physicalDevice->handle,
            .device = handle,
            .pVulkanFunctions = &vmaVulkanFunctions,
            .instance = instance,
        };

        VK_CHECK(vmaCreateAllocator(&allocatorInfo, &vmaAllocator));
    }

    void VulkanDevice::DestroyVmaAllocator()
    {
        if (vmaAllocator != VK_NULL_HANDLE)
        {
            VmaTotalStatistics statistics;
            vmaCalculateStatistics(vmaAllocator, &statistics);
            LogInfo(GLogger, std::format("Total device memory leaked: {} bytes.", (uint32)statistics.total.statistics.allocationBytes));
            vmaDestroyAllocator(vmaAllocator);
            vmaAllocator = VK_NULL_HANDLE;
        }
    }

    void VulkanDevice::WaitIdle()
    {
        VK_CHECK(vkDeviceWaitIdle(handle));
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
        bool CompileRenderBackendCommand(const RenderBackendCommandClearTextureUAV& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBarriers& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandTransitions& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBeginTimingQuery& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandEndTimingQuery& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandResolveTimingQueryResults& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatch& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchIndirect& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBuildBottomLevelAS& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBuildTopLevelAS& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandTraceRays& command);
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
        bool CompileRenderBackendCommand(const RenderBackendCommandEvaluateDLSS& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandFSR2Dispatch& command);
    private:
        void ApplyTransitions();
        bool PrepareForDispatch(RenderBackendShaderHandle shader, const RenderBackendShaderArguments& shaderArguments);
        bool PrepareForDraw(RenderBackendShaderHandle shader, const RenderBackendGraphicsPipelineState& pipelineState, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendShaderArguments& shaderArguments);
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

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandCopyBuffer& command)
    {
        const auto& srcBuffer = device->GetBuffer(command.srcBuffer);
        const auto& dstBuffer = device->GetBuffer(command.dstBuffer);
        VkBufferCopy copyRegion = {
            .srcOffset = command.srcOffset,
            .dstOffset = command.dstOffset,
            .size = command.bytes,
        };
        vkCmdCopyBuffer(commandBuffer, srcBuffer->handle, dstBuffer->handle, 1, &copyRegion);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandCopyTexture& command)
    {
        const auto& srcTexture = device->GetTexture(command.srcTexture);
        const auto& dstTexture = device->GetTexture(command.dstTexture);

        assert(!HAS_ANY_FLAGS(srcTexture->flags, RenderBackendTextureCreateFlags::Readback));
        if (HAS_ANY_FLAGS(dstTexture->flags, RenderBackendTextureCreateFlags::Readback))
        {
            VkBufferImageCopy copy[RenderBackendMaxNumTextureMipLevels] = {};
            uint64 bufferOffset = 0;
            uint32 width = srcTexture->width;
            uint32 height = srcTexture->height;
            uint32 depth = srcTexture->depth;

            const VulkanCpuReadbackBuffer* cpuReadbackBuffer = dstTexture->cpuReadbackBuffer;
            for (uint32 mipLevel = 0; mipLevel < srcTexture->mipLevels; mipLevel++)
            {
                copy[mipLevel].bufferOffset = cpuReadbackBuffer->mipOffsets[mipLevel];
                //copy[mipLevel].bufferRowLength = width;
                //copy[mipLevel].bufferImageHeight = height;
                copy[mipLevel].imageSubresource.baseArrayLayer = command.srcSubresourceLayers.firstLayer;
                copy[mipLevel].imageSubresource.mipLevel = mipLevel;
                copy[mipLevel].imageSubresource.layerCount = 1;
                copy[mipLevel].imageSubresource.aspectMask = srcTexture->aspectMask;
                copy[mipLevel].imageExtent.width = width;
                copy[mipLevel].imageExtent.height = height;
                copy[mipLevel].imageExtent.depth = depth;

                width = std::max(1u, width / 2);
                height = std::max(1u, height / 2);
                depth = std::max(1u, depth / 2);
            }

            vkCmdCopyImageToBuffer(
                commandBuffer,
                srcTexture->handle,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                cpuReadbackBuffer->handle,
                srcTexture->mipLevels,
                copy);
        }
        else
        {
            VkImageCopy copyRegion = {
                .srcSubresource = {
                    .aspectMask = srcTexture->aspectMask,
                    .mipLevel = command.srcSubresourceLayers.mipLevel,
                    .baseArrayLayer = command.srcSubresourceLayers.firstLayer,
                    .layerCount = command.srcSubresourceLayers.arrayLayers,
                },
                .srcOffset = { command.srcOffset.x, command.srcOffset.y, command.srcOffset.z },
                .dstSubresource = {
                    .aspectMask = dstTexture->aspectMask,
                    .mipLevel = command.dstSubresourceLayers.mipLevel,
                    .baseArrayLayer = command.dstSubresourceLayers.firstLayer,
                    .layerCount = command.dstSubresourceLayers.arrayLayers,
                },
                .dstOffset = { command.dstOffset.x, command.dstOffset.y, command.dstOffset.z },
                .extent = { command.extent.width, command.extent.height, command.extent.depth },
            };
            vkCmdCopyImage(commandBuffer, srcTexture->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTexture->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        }
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandUpdateBuffer& command)
    {
        const auto& buffer = device->GetBuffer(command.buffer);
        vkCmdUpdateBuffer(commandBuffer, buffer->handle, command.offset, command.size, command.data);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandUpdateTexture& command)
    {
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandClearTextureUAV& command)
    {
        const auto& texture = device->GetTexture(command.uav.texture);

        VkClearColorValue color = {};
        color.uint32[0] = command.clearValue.colorValue.uint32[0];
        color.uint32[1] = command.clearValue.colorValue.uint32[1];
        color.uint32[2] = command.clearValue.colorValue.uint32[2];
        color.uint32[3] = command.clearValue.colorValue.uint32[3];
        color.float32[0] = command.clearValue.colorValue.float32[0];
        color.float32[1] = command.clearValue.colorValue.float32[1];
        color.float32[2] = command.clearValue.colorValue.float32[2];
        color.float32[3] = command.clearValue.colorValue.float32[3];

        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseArrayLayer = 0;
        range.baseMipLevel = command.uav.mipLevel;
        range.layerCount = VK_REMAINING_ARRAY_LAYERS;
        range.levelCount = 1;

        vkCmdClearColorImage(
            commandBuffer,
            texture->handle,
            VK_IMAGE_LAYOUT_GENERAL,
            &color,
            1,
            &range);

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBarriers& command)
    {
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandTransitions& command)
    {
        for (uint32 i = 0; i < command.numTransitions; i++)
        {
            const auto& transition = command.transitions[i];
            assert(transition.stateBefore != transition.stateAfter);

            // TODO: remove this
            if (transition.stateAfter == RenderBackendResourceState::Undefined)
            {
                continue;
            }

            switch (transition.type)
            {
            case RenderBackendBarrier::Type::Texture:
            {
                VulkanTexture* texture = device->GetTexture(transition.texture);
                VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                VkAccessFlags2 srcAccessMask, dstAccessMask;
                VkImageLayout oldLayout, newLayout;
                GetBarrierInfo2(
                    transition.stateBefore,
                    transition.stateAfter,
                    &oldLayout,
                    &newLayout,
                    &srcStageMask,
                    &dstStageMask,
                    &srcAccessMask,
                    &dstAccessMask);
                VkImageMemoryBarrier2 imageBarrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .srcStageMask = srcStageMask,
                    .srcAccessMask = srcAccessMask,
                    .dstStageMask = dstStageMask,
                    .dstAccessMask = dstAccessMask,
                    .oldLayout = oldLayout,
                    .newLayout = newLayout,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = texture->handle,
                    .subresourceRange = {
                        .aspectMask = texture->aspectMask,
                        .baseMipLevel = transition.textureRange.firstLevel,
                        .levelCount = transition.textureRange.mipLevels,
                        .baseArrayLayer = transition.textureRange.firstLayer,
                        .layerCount = transition.textureRange.arrayLayers,
                    },
                };
                imageBarriers.push_back(std::move(imageBarrier));
            }
            break;
            case RenderBackendBarrier::Type::Buffer:
            {
                VulkanBuffer* buffer = device->GetBuffer(transition.buffer);
                VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                VkAccessFlags2 srcAccessMask, dstAccessMask;
                GetBarrierInfo2(
                    transition.stateBefore,
                    transition.stateAfter,
                    nullptr,
                    nullptr,
                    &srcStageMask,
                    &dstStageMask,
                    &srcAccessMask,
                    &dstAccessMask);
                VkBufferMemoryBarrier2 bufferMemoryBarrier = {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask = srcStageMask,
                    .srcAccessMask = srcAccessMask,
                    .dstStageMask = dstStageMask,
                    .dstAccessMask = dstAccessMask,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = buffer->handle,
                    .offset = transition.bufferRange.offset,
                    .size = transition.bufferRange.size,
                };
                bufferBarriers.push_back(std::move(bufferMemoryBarrier));
            }
            break;
            }
        }

        if (!bufferBarriers.empty() || !imageBarriers.empty())
        {
            VkDependencyInfo dependency = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .bufferMemoryBarrierCount = (uint32)bufferBarriers.size(),
                .pBufferMemoryBarriers = bufferBarriers.data(),
                .imageMemoryBarrierCount = (uint32)imageBarriers.size(),
                .pImageMemoryBarriers = imageBarriers.data(),
            };
            vkCmdPipelineBarrier2(commandBuffer, &dependency);
            imageBarriers.clear();
            bufferBarriers.clear();
        }

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBeginTimingQuery& command)
    {
        const auto& timingQueryHeap = device->GetTimingQueryHeap(command.timingQueryHeap);
        uint32 queryIndex = command.region * 2 + 0;
        assert(queryIndex < timingQueryHeap.maxQueryCount);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timingQueryHeap.handle, queryIndex);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandEndTimingQuery& command)
    {
        const auto& timingQueryHeap = device->GetTimingQueryHeap(command.timingQueryHeap);
        uint32 queryIndex = command.region * 2 + 1;
        assert(queryIndex < timingQueryHeap.maxQueryCount);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timingQueryHeap.handle, queryIndex);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandResolveTimingQueryResults& command)
    {
        const auto& timingQueryHeap = device->GetTimingQueryHeap(command.timingQueryHeap);
        const auto& buffer = device->GetBuffer(command.buffer);

        uint32 queryStart = 2 * command.regionStart;
        uint32 queryCount = 2 * command.regionCount;

        vkCmdCopyQueryPoolResults(
            commandBuffer,
            timingQueryHeap.handle,
            queryStart,
            queryCount,
            buffer->handle,
            command.offset,
            sizeof(uint64),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

        vkCmdResetQueryPool(commandBuffer, timingQueryHeap.handle, queryStart, queryCount);
        return true;
    }

    void VulkanRenderBackendCommandListContext::ApplyTransitions()
    {
        if (!bufferBarriers.empty() || !imageBarriers.empty())
        {
            VkDependencyInfoKHR dependencyInfo = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
                .bufferMemoryBarrierCount = (uint32)bufferBarriers.size(),
                .pBufferMemoryBarriers = bufferBarriers.data(),
                .imageMemoryBarrierCount = (uint32)imageBarriers.size(),
                .pImageMemoryBarriers = imageBarriers.data(),
            };
            device->GetBackend()->functions.vkCmdPipelineBarrier2KHR(commandBuffer, &dependencyInfo);
            bufferBarriers.clear();
            imageBarriers.clear();
        }
    }

    bool VulkanRenderBackendCommandListContext::PrepareForDispatch(RenderBackendShaderHandle shader, const RenderBackendShaderArguments& shaderArguments)
    {
        VulkanPushConstants pushConstants;
        for (uint32 i = 0; i < 16; i++)
        {
            if (shaderArguments.slots[i].type == 1)
            {
                VulkanTexture* texture = device->GetTexture(shaderArguments.slots[i].srvSlot.srv.texture);
                if (shaderArguments.slots[i].srvSlot.srv.numMipLevels == 1)
                {
                    pushConstants.indices[i] = texture->srvs[shaderArguments.slots[i].srvSlot.srv.baseMipLevel].srvIndex;
                }
                else
                {
                    pushConstants.indices[i] = texture->srvIndex;
                }
            }
            else if (shaderArguments.slots[i].type == 2)
            {
                VulkanTexture* texture = device->GetTexture(shaderArguments.slots[i].uavSlot.uav.texture);
                pushConstants.indices[i] = texture->uavs[shaderArguments.slots[i].uavSlot.uav.mipLevel].uavIndex;
            }
            else if (shaderArguments.slots[i].type == 3)
            {
                VulkanBuffer* buffer = device->GetBuffer(shaderArguments.slots[i].bufferSlot.handle);
                pushConstants.indices[i] = (buffer->uavIndex << 16) | (uint16)shaderArguments.slots[i].bufferSlot.offset;
            }
        }
        for (uint32 i = 0; i < 16; i++)
        {
            pushConstants.data[i] = shaderArguments.data[i];
        }
        const void* pushConstantValue = &pushConstants;
        uint32 pushConstantSize = device->bindlessDescriptorManager.pushConstantSize;

        VulkanPipeline* pipeline = device->FindOrCreateComputePipeline(device->GetShader(shader), pushConstantSize);
        if (pipeline->handle != activeComputePipeline)
        {
            VkDescriptorSet set = device->GetBindlessGlobalSet();
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->handle);
            activeComputePipeline = pipeline->handle;
        }
        if (pushConstantSize > 0)
        {
            vkCmdPushConstants(commandBuffer, pipeline->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, pushConstantSize, pushConstantValue);
        }
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatch& command)
    {
        if (!PrepareForDispatch(command.shader, command.shaderArguments))
        {
            return false;
        }
        vkCmdDispatch(commandBuffer, command.threadGroupCountX, command.threadGroupCountY, command.threadGroupCountZ);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchIndirect& command)
    {
        if (!PrepareForDispatch(command.shader, command.shaderArguments))
        {
            return false;
        }
        VulkanBuffer* argumentBuffer = device->GetBuffer(command.argumentBuffer);
        vkCmdDispatchIndirect(commandBuffer, argumentBuffer->handle, command.argumentBufferOffset);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBuildBottomLevelAS& command)
    {
        const auto& srcBLAS = command.srcBLAS ? device->GetAccelerationStructure(command.srcBLAS) : nullptr;
        const auto& dstBLAS = device->GetAccelerationStructure(command.dstBLAS);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags = dstBLAS->buildFlags,
            .mode = srcBLAS ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .srcAccelerationStructure = srcBLAS ? srcBLAS->handle : VK_NULL_HANDLE,
            .dstAccelerationStructure = dstBLAS->handle,
            .geometryCount = (uint32)dstBLAS->geometries.size(),
            .pGeometries = dstBLAS->geometries.data(),
            .scratchData = dstBLAS->scratchBuffer.deviceAddress
        };

        uint32 numGeometries = (uint32)dstBLAS->blasDesc.numGeometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges(numGeometries);
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos(numGeometries);
        for (uint32 i = 0; i < numGeometries; i++)
        {
            if (dstBLAS->blasDesc.geometryDescs->type == RenderBackendRayTracingGeometryType::Triangles)
            {
                VkAccelerationStructureBuildRangeInfoKHR buildRange = {
                    .primitiveCount = dstBLAS->blasDesc.geometryDescs[i].triangleDesc.numIndices / 3,
                    .primitiveOffset = 0,
                    .firstVertex = 0,
                    .transformOffset = dstBLAS->blasDesc.geometryDescs[i].triangleDesc.transformOffset,
                };
                ranges[i] = buildRange;
                buildRangeInfos[i] = &ranges[i];
            }
            else if (dstBLAS->blasDesc.geometryDescs->type == RenderBackendRayTracingGeometryType::AABBs)
            {
                VulkanBuffer* buffer = device->GetBuffer(dstBLAS->blasDesc.geometryDescs[i].aabbDesc.buffer);
                VkAccelerationStructureBuildRangeInfoKHR buildRange = {
                    .primitiveCount = (uint32)(buffer->size / sizeof(VkAabbPositionsKHR)),
                    .primitiveOffset = dstBLAS->blasDesc.geometryDescs[i].aabbDesc.offset,
                    .firstVertex = 0,
                    .transformOffset = 0,
                };
                ranges[i] = buildRange;
                buildRangeInfos[i] = &ranges[i];
            }
        }

        device->GetBackend()->functions.vkCmdBuildAccelerationStructuresKHR(
            commandBuffer,
            1,
            &accelerationStructureBuildGeometryInfo,
            buildRangeInfos.data());

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBuildTopLevelAS& command)
    {
        const auto& srcTLAS = command.srcTLAS ? device->GetAccelerationStructure(command.srcTLAS) : nullptr;
        const auto& dstTLAS = device->GetAccelerationStructure(command.dstTLAS);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
            .flags = dstTLAS->buildFlags,
            .mode = srcTLAS ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .srcAccelerationStructure = srcTLAS ? srcTLAS->handle : VK_NULL_HANDLE,
            .dstAccelerationStructure = dstTLAS->handle,
            .geometryCount = (uint32)dstTLAS->geometries.size(),
            .pGeometries = dstTLAS->geometries.data(),
            .scratchData = dstTLAS->scratchBuffer.deviceAddress,
        };

        VkAccelerationStructureBuildRangeInfoKHR range = {
            .primitiveCount = dstTLAS->tlasDesc.numInstances,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0,
        };
        VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfo = &range;

        device->GetBackend()->functions.vkCmdBuildAccelerationStructuresKHR(
            commandBuffer,
            1,
            &accelerationStructureBuildGeometryInfo,
            &buildRangeInfo);

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandTraceRays& command)
    {
        VulkanPushConstants pushConstants;
        for (uint32 i = 0; i < 16; i++)
        {
            if (command.shaderArguments.slots[i].type == 1)
            {
                VulkanTexture* texture = device->GetTexture(command.shaderArguments.slots[i].srvSlot.srv.texture);
                pushConstants.indices[i] = texture->srvIndex;
            }
            else if (command.shaderArguments.slots[i].type == 2)
            {
                VulkanTexture* texture = device->GetTexture(command.shaderArguments.slots[i].uavSlot.uav.texture);
                pushConstants.indices[i] = texture->uavs[command.shaderArguments.slots[i].uavSlot.uav.mipLevel].uavIndex;
            }
            else if (command.shaderArguments.slots[i].type == 3)
            {
                VulkanBuffer* buffer = device->GetBuffer(command.shaderArguments.slots[i].bufferSlot.handle);
                pushConstants.indices[i] = (buffer->uavIndex << 16) | (uint16)command.shaderArguments.slots[i].bufferSlot.offset;
            }
            else if (command.shaderArguments.slots[i].type == 4)
            {
                VulkanRayTracingAccelerationStructure* as = device->GetAccelerationStructure(command.shaderArguments.slots[i].asSlot.handle);
                pushConstants.indices[i] = as->descriptorIndex;
            }
        }
        for (uint32 i = 0; i < 16; i++)
        {
            pushConstants.data[i] = command.shaderArguments.data[i];
        }
        const void* pushConstantValue = &pushConstants;
        uint32 pushConstantSize = device->bindlessDescriptorManager.pushConstantSize;

        VulkanRayTracingPipelineState* pipelineState = device->GetRayTracingPipelineState(command.pipelineState);

        if (pipelineState->handle != activeRayTracingPipeline)
        {
            VkDescriptorSet set = device->GetBindlessGlobalSet();
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineState->handle);
            activeComputePipeline = pipelineState->handle;
        }

        if (pushConstantSize > 0)
        {
            vkCmdPushConstants(commandBuffer, pipelineState->pipelineLayout, VK_SHADER_STAGE_ALL, 0, pushConstantSize, pushConstantValue);
        }

        VulkanBuffer* sbtBuffer = device->GetBuffer(command.shaderBindingTable);

        device->GetBackend()->functions.vkCmdTraceRaysKHR(
            commandBuffer,
            &sbtBuffer->shaderBindingTable->rayGenShaderBindingTable,
            &sbtBuffer->shaderBindingTable->missShaderBindingTable,
            &sbtBuffer->shaderBindingTable->hitShaderBindingTable,
            &sbtBuffer->shaderBindingTable->callableShaderBindingTable,
            command.width,
            command.height,
            command.depth);

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBeginRenderPass& command)
    {
#if VULKAN_RENDER_BACKEND_DYNAMIC_RENDERING
        GetRenderingInfo(device, command.renderPassInfo, &renderingInfo);
        vkCmdBeginRendering(commandBuffer, &renderingInfo.renderingInfo);
        insideRenderPass = true;
#else
        VkClearValue clearValues[RenderBackendMaxNumSimultaneousColorRenderTargets + 1] = {};
        VulkanRenderPassDesc renderPassDesc = {};
        GetRenderPassDescAndClearValues(device, command.renderPassInfo, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &renderPassDesc, clearValues);

        const auto& renderPass = device->FindOrCreateRenderPass(renderPassDesc);
        const auto& framebuffer = device->FindOrCreateFramebuffer(command.renderPassInfo, renderPassDesc, renderPass);
        if (!renderPass || !framebuffer)
        {
            return false;
        }

        VkRenderPassBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = framebuffer->handle,
            .renderArea = { 0, 0, framebuffer->width, framebuffer->height },
            .clearValueCount = framebuffer->numAttachments,
            .pClearValues = clearValues,
        };

        vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
        activeRenderPass = renderPass;
        activeColorAttachmentCount = framebuffer->numColorAttachments;
#endif
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandEndRenderPass& command)
    {
#if VULKAN_RENDER_BACKEND_DYNAMIC_RENDERING
        vkCmdEndRendering(commandBuffer);
        insideRenderPass = false;
#else
        vkCmdEndRenderPass(commandBuffer);
        activeRenderPass = VK_NULL_HANDLE;
#endif
        return true;
    }

    bool VulkanRenderBackendCommandListContext::PrepareForDraw(RenderBackendShaderHandle shader, const RenderBackendGraphicsPipelineState& pipelineState, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendShaderArguments& shaderArguments)
    {
        VulkanPushConstantsTest pushConstantsTest;
        VulkanPushConstants pushConstants;
        const void* pushConstantValue = nullptr;
        if (shaderArguments.test)
        {
            for (uint32 i = 0; i < 16; i++)
            {
                if (shaderArguments.slots[i].type == 1 && shaderArguments.slots[i].srvSlot.srv.texture)
                {
                    VulkanTexture* texture = device->GetTexture(shaderArguments.slots[i].srvSlot.srv.texture);
                    pushConstantsTest.indices[i] = texture->srvIndex;
                }
                else if (shaderArguments.slots[i].type == 2 && shaderArguments.slots[i].uavSlot.uav.texture)
                {
                    VulkanTexture* texture = device->GetTexture(shaderArguments.slots[i].uavSlot.uav.texture);
                    pushConstantsTest.indices[i] = texture->uavs[shaderArguments.slots[i].uavSlot.uav.mipLevel].uavIndex;
                }
                else if (shaderArguments.slots[i].type == 3 && shaderArguments.slots[i].bufferSlot.handle)
                {
                    VulkanBuffer* buffer = device->GetBuffer(shaderArguments.slots[i].bufferSlot.handle);
                    pushConstantsTest.indices[i] = ((buffer->uavIndex & 0xffff) << 16) | (shaderArguments.slots[i].bufferSlot.offset & 0xffff);
                }
            }
            memcpy(pushConstantsTest.data, shaderArguments.testData, 64);
            pushConstantValue = &pushConstantsTest;
        }
        else
        {
            for (uint32 i = 0; i < 16; i++)
            {
                if (shaderArguments.slots[i].type == 1 && shaderArguments.slots[i].srvSlot.srv.texture)
                {
                    VulkanTexture* texture = device->GetTexture(shaderArguments.slots[i].srvSlot.srv.texture);
                    pushConstants.indices[i] = texture->srvIndex;
                }
                else if (shaderArguments.slots[i].type == 2 && shaderArguments.slots[i].uavSlot.uav.texture)
                {
                    VulkanTexture* texture = device->GetTexture(shaderArguments.slots[i].uavSlot.uav.texture);
                    pushConstants.indices[i] = texture->uavs[shaderArguments.slots[i].uavSlot.uav.mipLevel].uavIndex;
                }
                else if (shaderArguments.slots[i].type == 3 && shaderArguments.slots[i].bufferSlot.handle)
                {
                    VulkanBuffer* buffer = device->GetBuffer(shaderArguments.slots[i].bufferSlot.handle);
                    pushConstants.indices[i] = ((buffer->uavIndex & 0xffff) << 16) | (shaderArguments.slots[i].bufferSlot.offset & 0xffff);
                }
            }
            memcpy(pushConstants.data, shaderArguments.data, sizeof(shaderArguments.data));
            pushConstantValue = &pushConstants;
        }

        uint32 pushConstantSize = device->bindlessDescriptorManager.pushConstantSize;
#if VULKAN_RENDER_BACKEND_DYNAMIC_RENDERING
        assert(insideRenderPass);
        VulkanPipeline* pipeline = device->FindOrCreateGraphicsPipeline(device->GetShader(shader), pipelineState, pushConstantSize, topology, true, &renderingInfo, VK_NULL_HANDLE, 0);
#else
        assert(activeRenderPass != VK_NULL_HANDLE);
        VulkanPipeline* pipeline = device->FindOrCreateGraphicsPipeline(device->GetShader(shader), pipelineState, pushConstantSize, topology, false, nullptr, activeRenderPass, activeColorAttachmentCount);
#endif
        if (pipeline->handle != activeGraphicsPipeline)
        {
            VkDescriptorSet set = device->GetBindlessGlobalSet();
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle);
            activeGraphicsPipeline = pipeline->handle;
        }
        if (pushConstantSize > 0)
        {
            vkCmdPushConstants(commandBuffer, pipeline->layout, VK_SHADER_STAGE_ALL, 0, pushConstantSize, pushConstantValue);
        }
        if (indexBuffer)
        {
            vkCmdBindIndexBuffer(commandBuffer, device->GetBuffer(indexBuffer)->handle, 0, VK_INDEX_TYPE_UINT32);
        }
        ApplyTransitions();
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDraw& command)
    {
        OPTICK_EVENT();

        if (!PrepareForDraw(command.shader, command.pipelineState, command.topology, command.indexBuffer, command.shaderArguments))
        {
            return false;
        }
        if (!command.indexBuffer)
        {
            vkCmdDraw(
                commandBuffer,
                command.numVertices,
                command.numInstances,
                command.firstVertex,
                command.firstInstance);
        }
        else
        {
            vkCmdDrawIndexed(
                commandBuffer,
                command.numIndices,
                command.numInstances,
                command.firstIndex,
                command.vertexOffset,
                command.firstInstance);
        }
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDrawIndirect& command)
    {
        if (!PrepareForDraw(command.shader, command.pipelineState, command.topology, command.indexBuffer, command.shaderArguments))
        {
            return false;
        }
        if (!command.indexBuffer)
        {
            vkCmdDrawIndirect(
                commandBuffer,
                device->GetBuffer(command.argumentBuffer)->handle,
                command.argumentBufferOffset,
                command.numDraws,
                sizeof(VkDrawIndirectCommand));
        }
        else
        {
            vkCmdDrawIndexedIndirect(
                commandBuffer,
                device->GetBuffer(command.argumentBuffer)->handle,
                command.argumentBufferOffset,
                command.numDraws,
                sizeof(VkDrawIndexedIndirectCommand));
        }
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchMesh& command)
    {
        if (!PrepareForDraw(command.shader, command.pipelineState, command.topology, RenderBackendBufferHandle::Null, command.shaderArguments))
        {
            return false;
        }
        device->backend->functions.vkCmdDrawMeshTasksEXT(
            commandBuffer,
            command.threadGroupCountX,
            command.threadGroupCountY,
            command.threadGroupCountZ);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchMeshIndirect& command)
    {
        if (!PrepareForDraw(command.shader, command.pipelineState, command.topology, RenderBackendBufferHandle::Null, command.shaderArguments))
        {
            return false;
        }
        VulkanBuffer* argumentBuffer = device->GetBuffer(command.argumentBuffer);
        vkCmdDispatchIndirect(commandBuffer, argumentBuffer->handle, command.argumentBufferOffset);
        device->backend->functions.vkCmdDrawMeshTasksIndirectEXT(
            commandBuffer,
            argumentBuffer->handle,
            command.argumentBufferOffset,
            command.numDraws,
            sizeof(RenderBackendDispatchMeshIndirectArguments));
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandSetStencilReference& command)
    {
        vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FRONT_AND_BACK, command.stencilReference);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandSetScissor& command)
    {
        VkRect2D scissors[RenderBackendMaxNumViewports];
        for (uint32 i = 0; i < command.numScissors; i++)
        {
            scissors[i] = {
                .offset = {.x = command.scissors[i].left, .y = command.scissors[i].top },
                .extent = {.width = command.scissors[i].width, .height = command.scissors[i].height }
            };
        }
        vkCmdSetScissor(commandBuffer, 0, command.numScissors, scissors);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandSetViewport& command)
    {
        VkViewport viewports[RenderBackendMaxNumViewports];
        for (uint32 i = 0; i < command.numViewports; i++)
        {
            viewports[i] = {
                .x = command.viewports[i].x,
                .y = command.viewports[i].y + command.viewports[i].height,
                .width = command.viewports[i].width,
                .height = -command.viewports[i].height,
                .minDepth = command.viewports[i].minDepth,
                .maxDepth = command.viewports[i].maxDepth
            };
        }
        vkCmdSetViewport(commandBuffer, 0, command.numViewports, viewports);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBeginDebugLabel& command)
    {
#if !HE_ENBALE_STREAMLINE_SUPPORT
        VkDebugUtilsLabelEXT lableInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
            .pLabelName = command.labelName,
            .color = { command.color[0], command.color[1], command.color[2], command.color[3] }
        };
        device->backend->functions.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &lableInfo);
#endif
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandEndDebugLabel& command)
    {
#if !HE_ENBALE_STREAMLINE_SUPPORT
        device->backend->functions.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandEvaluateDLSS& command)
    {
#if HE_ENBALE_STREAMLINE_SUPPORT

        //static NVSDK_NGX_Parameter* NewNGXParameter = nullptr;
        //static NVSDK_NGX_Handle* NewNGXFeatureHandle = nullptr;

        //if (NewNGXFeatureHandle == nullptr)
        //{
        //    bool bReleaseMemoryOnDelete = true;
        //    bool bUseAutoExposure = false;
        //    NVSDK_NGX_PerfQuality_Value PerfQuality = NVSDK_NGX_PerfQuality_Value_MaxQuality;
        //    uint32 DLAAPreset = NVSDK_NGX_DLSS_Hint_Render_Preset_Default;
        //    uint32 DLSSPreset = NVSDK_NGX_DLSS_Hint_Render_Preset_A;
        //    uint32 renderWidth = command.renderWidth;
        //    uint32 renderHeight = command.renderHeight;
        //    uint32 targetWidth = renderWidth;
        //    uint32 targetHeight = renderHeight;
        //    bool depthInverted = true;

        //    NVSDK_NGX_Result ResultAllocateParameters = NVSDK_NGX_VULKAN_AllocateParameters(&NewNGXParameter);
        //    assert(NVSDK_NGX_SUCCEED(ResultAllocateParameters));

        //    NVSDK_NGX_Parameter_SetI(NewNGXParameter, NVSDK_NGX_Parameter_FreeMemOnReleaseFeature, bReleaseMemoryOnDelete ? 1 : 0);
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, static_cast<uint32>(DLAAPreset));
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, static_cast<uint32>(DLSSPreset));
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, static_cast<uint32>(DLSSPreset));
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, static_cast<uint32>(DLSSPreset));
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, static_cast<uint32>(DLSSPreset));

        //    NVSDK_NGX_PerfQuality_Value PerfQualityValue = static_cast<NVSDK_NGX_PerfQuality_Value>(PerfQuality);
        //    assert((PerfQualityValue >= NVSDK_NGX_PerfQuality_Value_MaxPerf) && (PerfQualityValue <= NVSDK_NGX_PerfQuality_Value_UltraQuality));

        //    uint32 FeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_None;
        //    FeatureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_IsHDR;
        //    FeatureCreateFlags |= depthInverted ? NVSDK_NGX_DLSS_Feature_Flags_DepthInverted : 0;
        //    FeatureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;
        //    FeatureCreateFlags |= bUseAutoExposure ? NVSDK_NGX_DLSS_Feature_Flags_AutoExposure : 0;
        //    //FeatureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;

        //    NVSDK_NGX_DLSS_Create_Params DlssCreateParams = {};
        //    DlssCreateParams.Feature.InWidth = renderWidth;
        //    DlssCreateParams.Feature.InHeight = renderHeight;
        //    DlssCreateParams.Feature.InTargetWidth = targetWidth;
        //    DlssCreateParams.Feature.InTargetHeight = targetHeight;
        //    DlssCreateParams.Feature.InPerfQualityValue = PerfQualityValue;
        //    DlssCreateParams.InFeatureCreateFlags = FeatureCreateFlags;
        //    DlssCreateParams.InEnableOutputSubrects = true;

        //    uint32 CreationNodeMask = 1;
        //    uint32 VisibilityMask = 1;

        //    NVSDK_NGX_Result ResultCreateDLSS = NGX_VULKAN_CREATE_DLSS_EXT(
        //        commandBuffer,
        //        CreationNodeMask,
        //        VisibilityMask,
        //        &NewNGXFeatureHandle,
        //        NewNGXParameter,
        //        &DlssCreateParams);
        //    assert(NVSDK_NGX_SUCCEED(ResultCreateDLSS));
        //}

        // slAllocateResources(commandBuffer, sl::kFeatureDLSS, viewport1->handle);
        // TODO: slFreeResources(sl::kFeatureDLSS, viewport->handle);

        uint32 targetWidth = command.targetWidth;
        uint32 targetHeight = command.targetHeight;

        sl::ViewportHandle viewport = 0;

        sl::Result result;

        static bool first = true;
        if (first)
        {
            device->WaitIdle();
            slFreeResources(sl::kFeatureDLSS_G, viewport);
            first = false;
        }

        sl::DLSSGOptions options{};
        // These are populated based on user selection in the UI
        options.mode = sl::DLSSGMode::eOn;
        options.onErrorCallback = myAPIErrorCallback;

        // IMPORTANT: Note that we are using IDENTICAL viewport as when tagging our resources
        result = slDLSSGSetOptions(viewport, options);
        if (result != sl::Result::eOk)
        {
            printf("slDLSSGSetOptions\n");
            // Handle error here, check the logs
        }

        sl::DLSSGState state = {};
        result = slDLSSGGetState(viewport, state, &options);
        if (result != sl::Result::eOk)
        {
            printf("slDLSSGGetState\n");
        }

        sl::ReflexState reflexState = {};
        result = slReflexGetState(reflexState);
        if (result != sl::Result::eOk)
        {
            printf("slReflexGetState\n");
        }

        VulkanTexture* outputVulkanTexture = device->GetTexture(command.output);
        VulkanTexture* colorVulkanTexture = device->GetTexture(command.color);
        VulkanTexture* depthVulkanTexture = device->GetTexture(command.depth);
        VulkanTexture* motionVectorsVulkanTexture = device->GetTexture(command.motionVectors);

        auto SLTextureFromVulkanTexture = [](VulkanTexture* texture, bool uav)
        {
            sl::Resource slTexture = {};
            slTexture.type = sl::ResourceType::eTex2d;
            slTexture.native = texture->handle;
            slTexture.memory = texture->deivceMemory;
            slTexture.view = uav ? texture->uavs[0].uav : texture->srv;
            slTexture.state = uav ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            slTexture.width = texture->width;
            slTexture.height = texture->height;
            slTexture.nativeFormat = texture->format;
            slTexture.mipLevels = texture->mipLevels;
            slTexture.arrayLayers = texture->arrayLayers;
            //slTexture.gpuVirtualAddress = texture->;
            slTexture.flags = texture->info.flags;
            slTexture.usage = texture->info.usage;
            return slTexture;
        };

        /*sl::Resource output = sl::Resource(sl::ResourceType::eTex2d, outputVulkanTexture->handle, &outputVulkanTexture->deivceMemory, &outputVulkanTexture->uavs[0].uav, VK_IMAGE_LAYOUT_GENERAL);
        sl::Resource color = sl::Resource(sl::ResourceType::eTex2d, colorVulkanTexture->handle, &colorVulkanTexture->deivceMemory, &colorVulkanTexture->srv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        sl::Resource depth = sl::Resource(sl::ResourceType::eTex2d, depthVulkanTexture->handle, &depthVulkanTexture->deivceMemory, &depthVulkanTexture->srv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        sl::Resource motionVectors = sl::Resource(sl::ResourceType::eTex2d, motionVectorsVulkanTexture->handle, &motionVectorsVulkanTexture->deivceMemory, &motionVectorsVulkanTexture->srv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);*/
        sl::Resource output = SLTextureFromVulkanTexture(outputVulkanTexture, true);
        sl::Resource color = SLTextureFromVulkanTexture(colorVulkanTexture, false);
        sl::Resource depth = SLTextureFromVulkanTexture(depthVulkanTexture, false);
        sl::Resource motionVectors = SLTextureFromVulkanTexture(motionVectorsVulkanTexture, false);

        sl::Extent renderExtent = {};
        renderExtent.top = 0;
        renderExtent.left = 0;
        renderExtent.width = colorVulkanTexture->width;
        renderExtent.height = colorVulkanTexture->height;

        sl::Extent targetExtent = {};
        targetExtent.top = 0;
        targetExtent.left = 0;
        targetExtent.width = outputVulkanTexture->width;
        targetExtent.height = outputVulkanTexture->height;

        {
            sl::ResourceTag outputTag = sl::ResourceTag(&output, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
            sl::ResourceTag colorTag = sl::ResourceTag(&color, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
            sl::ResourceTag depthTag = sl::ResourceTag(&depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
            sl::ResourceTag motionVectorsTag = sl::ResourceTag(&motionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
            sl::ResourceTag tags[] = { outputTag, colorTag, depthTag, motionVectorsTag };
            slSetTag(viewport, tags, _countof(tags), commandBuffer);
        }

        {
            //sl::ResourceTag hudLessColor = sl::ResourceTag(nullptr, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, nullptr);
            sl::ResourceTag hudLessColor = sl::ResourceTag(&output, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
            sl::ResourceTag tags[] = { hudLessColor };
            slSetTag(viewport, tags, _countof(tags), commandBuffer);
        }

        {
            sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(nullptr, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, nullptr);
            //sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(&output, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
            sl::ResourceTag tags[] = { uiColorAndAlpha };
            slSetTag(viewport, tags, _countof(tags), commandBuffer);
        }

        //{
        //    // Note: here `precisionInfo` refers to the transform needed to be applied to the buffer values to convert from a low-precision format (e.g. 8-bits) to a high-precision format (e.g. 16-bits). Refer to
        //    sl::ResourceTag bidirectionalDistortionTag = sl::ResourceTag(nullptr, sl::kBufferTypeBidirectionalDistortionField, sl::ResourceLifecycle::eValidUntilPresent, &fullExtent, &precisionInfo); // valid all the time
        //    sl::Resource inputs[] = { bidirectionalDistortionTag };
        //    slSetTag(viewport, inputs, _countof(inputs), cmdList);
        //}

        sl::FrameToken* frameToken = nullptr;
        if (SL_FAILED(result, slGetNewFrameToken(frameToken, &command.frameIndex)))
        {
            LogError(GLogger, std::format("slGetNewFrameToken, error code: {}", (int32)result));
            return false;
        }

        // Inform SL that DLSS should be injected at this point for the given viewport
        const sl::BaseStructure* inputs[] = { &viewport };
        if (SL_FAILED(result, slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, _countof(inputs), commandBuffer)))
        {
            LogError(GLogger, std::format("slEvaluateFeature, error code: {}", (int32)result));
            return false;
        }

        //auto NGXTextureFromVulkanTexture = [](VulkanTexture* Texture, bool uav)
        //{
        //    // DLSS_TODO Figure out where to get those from if the textures are arrayed or mipped.
        //    assert(Texture->depth == 1);

        //    VkImageSubresourceRange SubresourceRange = {};
        //    SubresourceRange.aspectMask = Texture->aspectMask;
        //    SubresourceRange.baseArrayLayer = 0;
        //    SubresourceRange.layerCount = 1;
        //    SubresourceRange.baseMipLevel = 0;
        //    SubresourceRange.levelCount = 1;

        //    assert(SubresourceRange.layerCount == 1);
        //    assert(SubresourceRange.levelCount == 1);

        //    NVSDK_NGX_Resource_VK NGXTexture = {};
        //    NGXTexture.Type = NVSDK_NGX_RESOURCE_VK_TYPE_VK_IMAGEVIEW;
        //    NGXTexture.Resource.ImageViewInfo.ImageView = uav ? Texture->uavs[0].uav : Texture->srv;
        //    NGXTexture.Resource.ImageViewInfo.Image = Texture->handle;
        //    NGXTexture.Resource.ImageViewInfo.Format = Texture->format;
        //    NGXTexture.Resource.ImageViewInfo.Width = Texture->width;
        //    NGXTexture.Resource.ImageViewInfo.Height = Texture->height;
        //    NGXTexture.ReadWrite = HAS_ANY_FLAGS(Texture->flags, RenderBackendTextureCreateFlags::UnorderedAccess);
        //    NGXTexture.Resource.ImageViewInfo.SubresourceRange = SubresourceRange;

        //    return NGXTexture;
        //};

        //VulkanTexture* output = device->GetTexture(command.output);
        //VulkanTexture* color = device->GetTexture(command.color);
        //VulkanTexture* depth = device->GetTexture(command.depth);
        //VulkanTexture* motionVectors = device->GetTexture(command.motionVectors);
        //// VulkanTexture* exposureTexture = device->GetTexture(command.exposureTexture);

        //NVSDK_NGX_VK_DLSS_Eval_Params DlssEvalParams = {};

        //NVSDK_NGX_Resource_VK InOutput = NGXTextureFromVulkanTexture(output, true);
        //assert(InOutput.ReadWrite == true);
        //NVSDK_NGX_Resource_VK InColor = NGXTextureFromVulkanTexture(color, false);
        //NVSDK_NGX_Resource_VK InDepth = NGXTextureFromVulkanTexture(depth, false);
        //NVSDK_NGX_Resource_VK InMotionVectors = NGXTextureFromVulkanTexture(motionVectors, false);
        //// NVSDK_NGX_Resource_VK InExposureTexture = NGXTextureFromVulkanTexture(exposureTexture, false);

        //DlssEvalParams.Feature.pInOutput = &InOutput;
        //DlssEvalParams.InOutputSubrectBase.X = 0;
        //DlssEvalParams.InOutputSubrectBase.Y = 0;

        //DlssEvalParams.Feature.pInColor = &InColor;
        //DlssEvalParams.InColorSubrectBase.X = 0;
        //DlssEvalParams.InColorSubrectBase.Y = 0;

        //DlssEvalParams.InRenderSubrectDimensions.Width = command.renderWidth;
        //DlssEvalParams.InRenderSubrectDimensions.Height = command.renderHeight;

        //DlssEvalParams.pInDepth = &InDepth;
        //DlssEvalParams.InDepthSubrectBase.X = 0;
        //DlssEvalParams.InDepthSubrectBase.Y = 0;

        //DlssEvalParams.pInMotionVectors = &InMotionVectors;
        //DlssEvalParams.InMVSubrectBase.X = 0;
        //DlssEvalParams.InMVSubrectBase.Y = 0;

        //// DlssEvalParams.pInExposureTexture = command.useAutoExposure ? nullptr : &InExposureTexture;
        //DlssEvalParams.pInExposureTexture = 0;
        ////DlssEvalParams.InPreExposure = InArguments.PreExposure;
        //DlssEvalParams.Feature.InSharpness = 0.0; // Sharpening is deprecated
        //DlssEvalParams.InJitterOffsetX = command.jitterOffsetX;
        //DlssEvalParams.InJitterOffsetY = command.jitterOffsetY;
        //DlssEvalParams.InMVScaleX = command.motionVectorScaleX;
        //DlssEvalParams.InMVScaleY = command.motionVectorScaleY;
        //DlssEvalParams.InReset = command.reset ? 1 : 0;
        //DlssEvalParams.InFrameTimeDeltaInMsec = command.deltaTime * 1000.0f;

        //NVSDK_NGX_Result ResultEvaluate = NGX_VULKAN_EVALUATE_DLSS_EXT(
        //    commandBuffer,
        //    NewNGXFeatureHandle,
        //    NewNGXParameter,
        //    &DlssEvalParams);

        //if (NVSDK_NGX_FAILED(ResultEvaluate))
        //{
        //    return false;
        //}

#endif
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandFSR2Dispatch& command)
    {
        FfxFsr2ContextDescription& fsr2InitializationParameters = device->fsr2InitializationParameters;
        FfxFsr2Context& fsr2Context = device->fsr2Context;

        // Setup interface.
        static uint32 previousRenderWidth  = 0;
        static uint32 previousRenderHeight = 0;
        static uint32 previousTargetWidth  = 0;
        static uint32 previousTargetHeight = 0;

        if (previousRenderWidth  != command.renderWidth  ||
            previousRenderHeight != command.renderHeight ||
            previousTargetWidth  != command.targetWidth  ||
            previousTargetHeight != command.targetHeight)
        {
            // Only destroy contexts which are live
            if (fsr2InitializationParameters.callbacks.scratchBuffer != nullptr)
            {
                ffxFsr2ContextDestroy(&fsr2Context);
                free(fsr2InitializationParameters.callbacks.scratchBuffer);
                fsr2InitializationParameters.callbacks.scratchBuffer = nullptr;
            }

            const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeVK(device->GetPhysicalDeviceHandle());
            void* scratchBuffer = malloc(scratchBufferSize);
            FfxErrorCode errorCode = ffxFsr2GetInterfaceVK(&fsr2InitializationParameters.callbacks, scratchBuffer, scratchBufferSize, device->GetPhysicalDeviceHandle(), vkGetDeviceProcAddr);
            FFX_ASSERT(errorCode == FFX_OK);

            fsr2InitializationParameters.device = ffxGetDeviceVK(device->GetHandle());
            fsr2InitializationParameters.maxRenderSize.width = command.renderWidth;
            fsr2InitializationParameters.maxRenderSize.height = command.renderHeight;
            fsr2InitializationParameters.displaySize.width = command.targetWidth;
            fsr2InitializationParameters.displaySize.height = command.targetHeight;
            //fsr2InitializationParameters.flags = FFX_FSR2_ENABLE_AUTO_EXPOSURE;

            // if (m_bInvertedDepth)
            {
                fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_DEPTH_INVERTED;
            }

            if (device->fsr2EnableDebugCheck)
            {
                fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
                fsr2InitializationParameters.fpMessage = &FSR2MessageCallBack;
            }

            // Input data is HDR
            fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;

            //const uint64_t memoryUsageBefore = getMemoryUsageSnapshot(device->GetPhysicalDeviceHandle());
            errorCode = ffxFsr2ContextCreate(&fsr2Context, &fsr2InitializationParameters);
            FFX_ASSERT(errorCode == FFX_OK);
            //const uint64_t memoryUsageAfter = getMemoryUsageSnapshot(device->GetPhysicalDeviceHandle());
            //memoryUsageInMegabytes = (memoryUsageAfter - memoryUsageBefore) * 0.000001f;

            previousRenderWidth  = command.renderWidth;
            previousRenderHeight = command.renderHeight;
            previousTargetWidth  = command.targetWidth;
            previousTargetHeight = command.targetHeight;
        }

        VulkanTexture* output = device->GetTexture(command.output);
        VulkanTexture* color = device->GetTexture(command.color);
        VulkanTexture* depth = device->GetTexture(command.depth);
        VulkanTexture* motionVectors = device->GetTexture(command.motionVectors);

        FfxFsr2DispatchDescription fsr2DispatchParameters = {};

        fsr2DispatchParameters.color = ffxGetTextureResourceVK(
            &fsr2Context,
            color->handle,
            color->srv,
            color->width,
            color->height,
            color->format,
            L"FSR2_InputColor",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchParameters.depth = ffxGetTextureResourceVK(
            &fsr2Context,
            depth->handle,
            depth->srv,
            depth->width,
            depth->height,
            depth->format,
            L"FSR2_InputDepth",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchParameters.motionVectors = ffxGetTextureResourceVK(
            &fsr2Context,
            motionVectors->handle,
            motionVectors->srv,
            motionVectors->width,
            motionVectors->height,
            motionVectors->format,
            L"FSR2_InputMotionVectors",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchParameters.output = ffxGetTextureResourceVK(
            &fsr2Context,
            output->handle,
            output->uavs[0].uav,
            output->width,
            output->height,
            output->format,
            L"FSR2_OutputUpscaledColor",
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        if (true)
        {
            fsr2DispatchParameters.exposure = ffxGetTextureResourceVK(
                &fsr2Context,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                1,
                1,
                VK_FORMAT_UNDEFINED,
                L"FSR2_InputExposure");
        }
        else
        {
            // TODO
        }

        if (true)
        {
            fsr2DispatchParameters.reactive = ffxGetTextureResourceVK(
                &fsr2Context,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                1,
                1,
                VK_FORMAT_UNDEFINED,
                L"FSR2_EmptyInputReactiveMap");
        }
        else
        {
            // TODO
        }

        if (true)
        {
            fsr2DispatchParameters.transparencyAndComposition = ffxGetTextureResourceVK(
                &fsr2Context,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                1,
                1,
                VK_FORMAT_UNDEFINED,
                L"FSR2_EmptyTransparencyAndCompositionMap");
        }
        else
        {
            // TODO
        }

        fsr2DispatchParameters.commandList = ffxGetCommandListVK(commandBuffer);
        fsr2DispatchParameters.jitterOffset.x = command.jitterOffsetX;
        fsr2DispatchParameters.jitterOffset.y = command.jitterOffsetY;
        fsr2DispatchParameters.motionVectorScale.x = (float)command.renderWidth;
        fsr2DispatchParameters.motionVectorScale.y = (float)command.renderHeight;
        fsr2DispatchParameters.reset = command.reset;
        fsr2DispatchParameters.enableSharpening = command.enableSharpening;
        fsr2DispatchParameters.sharpness = command.sharpeness;
        fsr2DispatchParameters.frameTimeDelta = command.deltaTime * 1000.0f; // @note frameTimeDelta is expressed in milliseconds
        fsr2DispatchParameters.preExposure = 1.0f;
        fsr2DispatchParameters.renderSize.width = command.renderWidth;
        fsr2DispatchParameters.renderSize.height = command.renderHeight;
        fsr2DispatchParameters.cameraFar = command.cameraFarPlane;
        fsr2DispatchParameters.cameraNear = command.cameraNearPlane;
        fsr2DispatchParameters.cameraFovAngleVertical = command.cameraFovAngleVertical;
        fsr2DispatchParameters.viewSpaceToMetersFactor = 1.0f;

        if (fsr2InitializationParameters.flags & FFX_FSR2_ENABLE_DEPTH_INVERTED)
        {
            std::swap(fsr2DispatchParameters.cameraFar, fsr2DispatchParameters.cameraNear);
        }

        FfxErrorCode errorCode = ffxFsr2ContextDispatch(&fsr2Context, &fsr2DispatchParameters);
        FFX_ASSERT(errorCode == FFX_OK);

        // Resotre bindless global descriptor set
        device->BindBindlessDescriptorSets(commandBuffer);

        return true;
    }

    struct BuildCommandBufferJobData
    {
        VulkanRenderBackend* backend;
        VulkanDevice* device;
        RenderBackendQueueFamily queueFamily;
        VkCommandBuffer commandBuffer;
        RenderBackendCommandContainer* commandContainer;
    };

    static void BuildCommandBuffer(BuildCommandBufferJobData* data)
    {
        OPTICK_EVENT();

        VulkanRenderBackendCommandListContext context(data->device, data->queueFamily, data->commandBuffer);
        if (context.CompileRenderBackendCommands(*data->commandContainer))
        {

        }
        else
        {
            // TODO
        }
    }

//      void ParallelBuildCommandBuffers()
 //   {
 //       OPTICK_EVENT();

 //       BuildCommandBufferJobData* jobData = new BuildCommandBufferJobData[numCommandBuffers];
 //       for (uint32 i = 0; i < numCommandBuffers; i++)
 //       {
 //           jobData[i].backend = backend;
 //           jobData[i].device = device;
 //           jobData[i].queueFamily = queueFamily;
 //           jobData[i].commandBuffer = commandBuffers[i];
 //           jobData[i].commandContainer = &commandContainers[i];
 //       }

 //       JobSystemAtomicCounterHandle counter = JobSystem::RunJobs(BuildCommandBuffer, jobData, numCommandBuffers);
 //       JobSystem::WaitForCounter(counter);

 //       vkCmdExecuteCommands(primaryCommandBuffer, numSecondaryCommandBuffers, secondaryCommandBuffers);

 //       for (uint32 i = 0; i < numCommandBuffers; i++)
 //       {
 //           statistics[i] = jobData[i].statistics;
 //       }

 //       delete[] jobData;
 //   }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommands(const RenderBackendCommandContainer& container)
    {
#define COMPILE_RENDER_COMMAND(command, RenderBackendCommandStruct)                                    \
    case RenderBackendCommandStruct::Type:                                                             \
    if (!CompileRenderBackendCommand(*reinterpret_cast<const RenderBackendCommandStruct*>(command)))   \
    {                                                                                                  \
        return false;                                                                                  \
    }                                                                                                  \
    break;

        for (uint32 i = 0; i < container.numCommands; i++)
        {
            switch (container.types[i])
            {
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandCopyBuffer);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandCopyTexture);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandUpdateBuffer);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandUpdateTexture);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandClearTextureUAV);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBarriers);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandTransitions);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBeginTimingQuery);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandEndTimingQuery);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandResolveTimingQueryResults);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatch);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchIndirect);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBuildBottomLevelAS);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBuildTopLevelAS);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandTraceRays);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandSetViewport);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandSetScissor);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandSetStencilReference);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBeginRenderPass);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandEndRenderPass);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDraw);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDrawIndirect);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchMesh);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchMeshIndirect);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBeginDebugLabel);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandEndDebugLabel);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandEvaluateDLSS);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandFSR2Dispatch);
            }
        }
#undef COMPILE_RENDER_COMMAND
        return true;
    }

    void VulkanRenderBackend::CreateRenderDevices(PhysicalDeviceID* physicalDeviceIDs, uint32 numPhysicalDevices, uint32* outDeviceMasks)
    {
        VulkanBindlessConfig bindlessConfig = {
            .numSamplers = 4 * 1024,
            .numSampledImages = 16 * 1024,
            .numStorageImages = 16 * 1024,
            .numStorageBuffers = 8 * 1024,
            .numAccelerationStructures = 8 * 1024
        };

        for (uint32 i = 0; i < numPhysicalDevices; i++)
        {
            VulkanDevice& device = devices[i];
            VulkanPhysicalDevice* physicalDevice = &availablePhysicalDevices[0];
            if (device.Init(this, physicalDevice, bindlessConfig))
            {
                outDeviceMasks[i] = device.GetDeviceMask();
            }
            else
            {
                outDeviceMasks[i] = 0;
            }
        }
    }

    void VulkanRenderBackend::DestroyRenderDevices()
    {
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            device.Shutdown();
        }
    }

    void VulkanRenderBackend::FlushRenderDevices()
    {
        VulkanDevice& device = devices[0];
        device.WaitIdle();
    }

    void VulkanRenderBackend::Tick()
    {
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            device.Tick();
        }
    }

    RenderBackendSwapChainHandle VulkanRenderBackend::CreateSwapChain(uint32 deviceMask, const RenderBackendSwapChainDesc* desc)
    {
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if (device.GetDeviceMask() & deviceMask)
            {
                uint32 index = device.CreateSwapChain(desc);
                RenderBackendSwapChainHandle handle = handleManager.Allocate<RenderBackendSwapChainHandle>(deviceMask);
                device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
                return handle;
            }
        }
        return RenderBackendSwapChainHandle::Null;
    }

    void VulkanRenderBackend::DestroySwapChain(RenderBackendSwapChainHandle handle)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.GetRenderBackendHandleRepresentation(handle.GetIndex());
            device.DestroySwapChain(index);
            break;
        }
    }

    void VulkanRenderBackend::ResizeSwapChain(RenderBackendSwapChainHandle handle, uint32* width, uint32* height)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.GetRenderBackendHandleRepresentation(handle.GetIndex());
            device.ResizeSwapChain(index, width, height);
            break;
        }
    }

    bool VulkanRenderBackend::PresentSwapChain(RenderBackendSwapChainHandle handle)
    {
        OPTICK_EVENT();

        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.GetRenderBackendHandleRepresentation(handle.GetIndex());
            VulkanSwapchain::Status status = device.PresentSwapChain(index, &device.presentSemaphores[index], 1);
            if (status == VulkanSwapchain::Status::Success)
            {
                status = device.AcquireImageIndex(index);
            }
            if (status == VulkanSwapchain::Status::OutOfDate)
            {
                device.RecreateSwapChain(index);
            }
            else if (status == VulkanSwapchain::Status::Error)
            {
                return false;
            }
            break;
        }
        return true;
    }

    RenderBackendTextureHandle VulkanRenderBackend::GetActiveSwapChainBuffer(RenderBackendSwapChainHandle handle)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.GetRenderBackendHandleRepresentation(handle.GetIndex());
            return device.GetActiveSwapChainBackBuffer(index);
        }
        return RenderBackendTextureHandle::Null;
    }

    RenderBackendBufferHandle VulkanRenderBackend::CreateBuffer(uint32 deviceMask, const RenderBackendBufferDesc* desc, const void* data, const char* name)
    {
        RenderBackendBufferHandle handle = handleManager.Allocate<RenderBackendBufferHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.CreateBuffer(desc, data, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::ResizeBuffer(RenderBackendBufferHandle handle, uint64 size)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                continue;
            }
            device.ResizeBuffer(index, size);
        }
    }

    void VulkanRenderBackend::MapBuffer(RenderBackendBufferHandle handle, void** data)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                continue;
            }
            *data = device.MapBuffer(index);
        }
    }

    void VulkanRenderBackend::UnmapBuffer(RenderBackendBufferHandle handle)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                continue;
            }
            device.UnmapBuffer(index);
        }
    }

    void VulkanRenderBackend::UpdateBuffer(RenderBackendBufferHandle handle, uint64 offset, const void* data, uint64 size)
    {
        OPTICK_EVENT();

        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                continue;
            }
            void* bufferAllocation = device.MapBuffer(index);
            memcpy(bufferAllocation, data, size);
            device.UnmapBuffer(index);
        }
    }

    void VulkanRenderBackend::DestroyBuffer(RenderBackendBufferHandle handle)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                continue;
            }
            device.DestroyBuffer(index);
            device.RemoveRenderBackendHandleRepresentation(handle.GetIndex());
        }
    }

    RenderBackendTextureHandle VulkanRenderBackend::CreateTexture(uint32 deviceMask, const RenderBackendTextureDesc* desc, const void* data, const char* name)
    {
        RenderBackendTextureHandle handle = handleManager.Allocate<RenderBackendTextureHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.CreateTexture(desc, data, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::DestroyTexture(RenderBackendTextureHandle handle)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                continue;
            }
            device.DestroyTexture(index);
            device.RemoveRenderBackendHandleRepresentation(handle.GetIndex());
        }
    }

    void VulkanRenderBackend::UploadTexture(RenderBackendTextureHandle handle, const RenderBackendTextureUploadDataDesc& data)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                continue;
            }

            {
                RenderBackendBufferDesc uploadBufferDesc = RenderBackendBufferDesc::Create(1, (uint32)data.totalSize, RenderBackendBufferCreateFlags::CpuOnly | RenderBackendBufferCreateFlags::CreateMapped);
                uint32 bufferIndex = device.CreateBuffer(&uploadBufferDesc, nullptr,  "UploadBuffer");
                VulkanBuffer& uploadBuffer = device.buffers[bufferIndex];
                device.MapBuffer(bufferIndex);

                std::vector<VkBufferImageCopy> copyRegions;
                VkDeviceSize copyOffset = 0;
                for (const auto& subresourceData : data.data)
                {
                    uint64 copySize = subresourceData.size;
                    uint8* copyDst = (uint8*)uploadBuffer.mappedData + copyOffset;
                    memcpy(copyDst, (uint8*)subresourceData.buffer, copySize);

                    uint32 blockWidth = 4;
                    uint32 blockHeight = 4;

                    uint32 rowLength = Math::CeilDiv(subresourceData.width, blockWidth);
                    uint32 imageHeight = Math::CeilDiv(subresourceData.height, blockHeight);

                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset = copyOffset;
                    copyRegion.bufferRowLength = 0;
                    copyRegion.bufferImageHeight = 0;
                    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel = subresourceData.level;
                    copyRegion.imageSubresource.baseArrayLayer = subresourceData.layer;
                    copyRegion.imageSubresource.layerCount = 1;
                    copyRegion.imageOffset = { 0, 0, 0 };
                    copyRegion.imageExtent = { subresourceData.width, subresourceData.height, 1 };

                    copyRegions.emplace_back(copyRegion);
                    copyOffset += copySize;
                }
                device.UnmapBuffer(bufferIndex);

                VkCommandBuffer commandBuffer; VkCommandPool pool;
                VulkanHelper::CreateTemporaryCommandBuffer(device.handle, device.GetQueueFamilyIndex(RenderBackendQueueFamily::Graphics), pool, commandBuffer);

                const VulkanTexture& texture = device.textures[index];
                assert(texture.mipLevels == data.data.size());
                // for (uint32 level = 0; level < texture.mipLevels; level++)
                {
                    VkImageMemoryBarrier barrier = {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = 0,
                        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .image = texture.handle,
                        .subresourceRange = {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = texture.mipLevels,
                            .baseArrayLayer = 0,
                            .layerCount = texture.arrayLayers,
                        },
                    };

                    vkCmdPipelineBarrier(
                        commandBuffer,
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

                    vkCmdCopyBufferToImage(
                        commandBuffer,
                        uploadBuffer.handle,
                        texture.handle,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        (uint32)copyRegions.size(),
                        copyRegions.data());

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = 0;
                    vkCmdPipelineBarrier(
                        commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);
                }

                VulkanHelper::FlushTemporaryCommandBuffer(device.handle, device.GetCommandQueue(RenderBackendQueueFamily::Graphics, 0)->handle, pool, commandBuffer);
                device.DestroyBuffer(bufferIndex);
            }
        }
    }

    RenderBackendSamplerHandle VulkanRenderBackend::CreateSampler(uint32 deviceMask, const RenderBackendSamplerDesc* desc, const char* name)
    {
        RenderBackendSamplerHandle handle = handleManager.Allocate<RenderBackendSamplerHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.CreateSampler(desc, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::DestroySampler(RenderBackendSamplerHandle handle)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                continue;
            }
            device.DestroySampler(index);
            device.RemoveRenderBackendHandleRepresentation(handle.GetIndex());
        }
    }

    RenderBackendShaderHandle VulkanRenderBackend::CreateShader(uint32 deviceMask, const RenderBackendShaderDesc* desc, const char* name)
    {
        RenderBackendShaderHandle handle = handleManager.Allocate<RenderBackendShaderHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.CreateShader(desc, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::DestroyShader(RenderBackendShaderHandle handle)
    {
        uint32 deviceMask = handle.GetDeviceMask();
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                continue;
            }
            device.DestroyShader(index);
            device.RemoveRenderBackendHandleRepresentation(handle.GetIndex());
        }
    }

    RenderBackendTimingQueryHeapHandle VulkanRenderBackend::CreateTimingQueryHeap(uint32 deviceMask, const RenderBackendTimingQueryHeapDesc* desc, const char* name)
    {
        RenderBackendTimingQueryHeapHandle handle = handleManager.Allocate<RenderBackendTimingQueryHeapHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }

            VulkanTimingQueryHeap timingQueryHeap = {};
            timingQueryHeap.maxQueryCount = desc->maxRegions * 2;
            VkQueryPoolCreateInfo queryPoolInfo = {
                .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                .queryType = VK_QUERY_TYPE_TIMESTAMP,
                .queryCount = timingQueryHeap.maxQueryCount,
            };
            VK_CHECK(vkCreateQueryPool(device.handle, &queryPoolInfo, VULKAN_ALLOCATION_CALLBACKS, &timingQueryHeap.handle));

            vkResetQueryPool(device.handle, timingQueryHeap.handle, 0, timingQueryHeap.maxQueryCount);

            uint32 index = device.timingQueryHeaps.Add(timingQueryHeap);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::DestroyTimingQueryHeap(RenderBackendTimingQueryHeapHandle timingQueryHeap)
    {

    }

    RenderBackendOcclusionQueryHeapHandle VulkanRenderBackend::CreateOcclusionQueryHeap(uint32 deviceMask, const RenderBackendOcclusionQueryHeapDesc* desc, const char* name)
    {
        RenderBackendOcclusionQueryHeapHandle handle = handleManager.Allocate<RenderBackendOcclusionQueryHeapHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }

            VulkanOcclusionQueryHeap occlusionQueryHeap = {};
            occlusionQueryHeap.maxQueryCount = desc->maxQueries;
            VkQueryPoolCreateInfo queryPoolInfo = {
                .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                .queryType = VK_QUERY_TYPE_OCCLUSION,
                .queryCount = occlusionQueryHeap.maxQueryCount,
            };
            VK_CHECK(vkCreateQueryPool(device.handle, &queryPoolInfo, VULKAN_ALLOCATION_CALLBACKS, &occlusionQueryHeap.handle));

            uint32 index = device.occlusionQueryHeaps.Add(occlusionQueryHeap);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::DestroyOcclusionQueryHeap(RenderBackendOcclusionQueryHeapHandle occlusionQueryHeap)
    {

    }

    RenderBackendRayTracingAccelerationStructureHandle VulkanRenderBackend::CreateRayTracingTopLevelAccelerationStructure(uint32 deviceMask, const RenderBackendRayTracingTopLevelAccelerationDesc* desc, const char* name)
    {
        RenderBackendRayTracingAccelerationStructureHandle handle = handleManager.Allocate<RenderBackendRayTracingAccelerationStructureHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.CreateTopLevelAS(desc, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    RenderBackendRayTracingAccelerationStructureHandle VulkanRenderBackend::CreateRayTracingBottomLevelAccelerationStructure(uint32 deviceMask, const RenderBackendRayTracingBottomLevelAccelerationDesc* desc, const char* name)
    {
        RenderBackendRayTracingAccelerationStructureHandle handle = handleManager.Allocate<RenderBackendRayTracingAccelerationStructureHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            uint32 index = device.CreateBottomLevelAS(desc, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::SubmitCommandLists(RenderBackendCommandList** commandLists, uint32 numCommandLists, RenderBackendSwapChainHandle swapChain)
    {
        OPTICK_EVENT();

        if (!commandLists || !numCommandLists)
        {
            return;
        }

        RenderBackendCommandContainer* commandContainer = commandLists[0]->GetCommandContainer();
        uint32 numCommands = commandContainer->numCommands;

        if (numCommands == 0)
        {
            return;
        }

        std::vector<VkSubmitInfo2> submitInfos[RenderBackendMaxNumDevices][RENDER_BACKEND_NUM_QUEUE_FAMILIES];
        VulkanSubmitContext submitContexts[RenderBackendMaxNumDevices] = {};

        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreInfos;
        std::vector<VkSemaphoreSubmitInfo> signalSemaphoreInfos;

        uint32 deviceMask = ~0u;
        uint32 queueFamily = (uint32)RenderBackendQueueFamily::Graphics;

        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            VulkanSubmitContext& submitContext = submitContexts[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }
            VulkanCommandBuffer* primaryCommandBuffer = device.commandBufferManager->PrepareForNextCommandBuffer();
            vkResetFences(device.GetHandle(), 1, &primaryCommandBuffer->fence);

            submitContext.completeFence = primaryCommandBuffer->fence;

            VkCommandBufferBeginInfo commandBufferBeginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
            VK_CHECK(vkBeginCommandBuffer(primaryCommandBuffer->handle, &commandBufferBeginInfo));

            // Bindless, the global descriptor set is only bound once per frame
            device.BindBindlessDescriptorSets(primaryCommandBuffer->handle);

            if (false)
            {
                // TODO
            }
            else
            {
                for (uint32 i = 0; i < numCommandLists; i++)
                {
                    BuildCommandBufferJobData jobData = {
                        .backend = this,
                        .device = &device,
                        .queueFamily = RenderBackendQueueFamily::Graphics,
                        .commandBuffer = primaryCommandBuffer->handle,
                        .commandContainer = commandLists[i]->GetCommandContainer(),
                    };
                    BuildCommandBuffer(&jobData);
                }
            }

            vkEndCommandBuffer(primaryCommandBuffer->handle);

            if (swapChain)
            {
                uint32 swapchainIndex = device.GetRenderBackendHandleRepresentation(swapChain.GetIndex());
                VulkanSwapchain* swapchain = &device.swapchains[swapchainIndex];

                submitContext.completeSemaphore = primaryCommandBuffer->semaphore;
                device.presentSemaphores[0] = submitContext.completeSemaphore;

                VkSemaphoreSubmitInfo waitSemaphoreInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                    .semaphore = swapchain->imageAcquiredSemaphores[swapchain->semaphoreIndex],
                    .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    //.deviceIndex = ,
                };
                waitSemaphoreInfos.emplace_back(waitSemaphoreInfo);

                VkSemaphoreSubmitInfo signalSemaphoreInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                    .semaphore = submitContexts[deviceIndex].completeSemaphore,
                    .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    //.deviceIndex = ,
                };
                signalSemaphoreInfos.emplace_back(signalSemaphoreInfo);
            }

            VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = primaryCommandBuffer->handle,
                //.deviceMask = ,
            };

            VkSubmitInfo2 submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext = nullptr,
                .waitSemaphoreInfoCount = (uint32)waitSemaphoreInfos.size(),
                .pWaitSemaphoreInfos = waitSemaphoreInfos.data(),
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &commandBufferSubmitInfo,
                .signalSemaphoreInfoCount = (uint32)signalSemaphoreInfos.size(),
                .pSignalSemaphoreInfos = signalSemaphoreInfos.data(),
            };
            submitInfos[deviceIndex][queueFamily].emplace_back(submitInfo);
        }

        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            /*if (submitContexts[deviceIndex] == nullptr)
            {
                continue;
            }*/
            if (deviceIndex > 0)
            {
                break;
            }

            VulkanDevice& device = devices[deviceIndex];

            VK_CHECK(vkQueueSubmit2(
                device.GetCommandQueue(queueFamily, 0)->handle,
                (uint32)submitInfos[deviceIndex][queueFamily].size(),
                submitInfos[deviceIndex][queueFamily].data(),
                submitContexts[deviceIndex].completeFence));
        }
    }

    RenderBackendTextureSRVHandle VulkanRenderBackend::CreateTextureSRV(uint32 deviceMask, const RenderBackendTextureSRVDesc* desc, const char* name)
    {
        RenderBackendTextureSRVHandle handle = handleManager.Allocate<RenderBackendTextureSRVHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }

            uint32 textureIndex = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(desc->texture.GetIndex(), &textureIndex))
            {
                continue;
            }

            uint32 index = device.CreateTextureSRV(textureIndex, desc, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    int32 VulkanRenderBackend::GetTextureSRVDescriptorIndex(uint32 deviceMask, RenderBackendTextureHandle srv)
    {
        VulkanDevice& device = devices[0];
        uint32 textureIndex = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(srv.GetIndex(), &textureIndex))
        {
            return 0;
        }
        return device.GetTextureSRVDescriptorIndex(textureIndex);
    }

    RenderBackendTextureUAVHandle VulkanRenderBackend::CreateTextureUAV(uint32 deviceMask, const RenderBackendTextureUAVDesc* desc, const char* name)
    {
        RenderBackendTextureUAVHandle handle = handleManager.Allocate<RenderBackendTextureUAVHandle>(deviceMask);
        for (uint32 deviceIndex = 0; deviceIndex <numDevices; deviceIndex++)
        {
            VulkanDevice& device = devices[deviceIndex];
            if ((device.GetDeviceMask() & deviceMask) == 0)
            {
                continue;
            }

            uint32 textureIndex = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(desc->texture.GetIndex(), &textureIndex))
            {
                continue;
            }

            uint32 index = device.CreateTextureUAV(textureIndex, desc, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    int32 VulkanRenderBackend::GetTextureUAVDescriptorIndex(uint32 deviceMask, RenderBackendTextureHandle handle)
    {
        VulkanDevice& device = devices[0];
        uint32 textureIndex = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &textureIndex))
        {
            return 0;
        }
        return device.GetTextureUAVDescriptorIndex(textureIndex, 0);
    }

    int32 VulkanRenderBackend::GetBufferUAVDescriptorIndex(uint32 deviceMask, RenderBackendBufferHandle handle)
    {
        if (!handle)
        {
            return -1;
        }
        VulkanDevice& device = devices[0];
        uint32 bufferIndex = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &bufferIndex))
        {
            return 0;
        }
        VulkanBuffer& buffer = device.buffers[bufferIndex];
        return buffer.uavIndex;
    }

    RenderBackendRayTracingPipelineStateHandle VulkanRenderBackend::CreateRayTracingPipelineState(uint32 deviceMask, const RenderBackendRayTracingPipelineStateDesc* desc, const char* name)
    {
        VulkanDevice& device = devices[0];
        RenderBackendRayTracingPipelineStateHandle handle = handleManager.Allocate<RenderBackendRayTracingPipelineStateHandle>(deviceMask);

        {
            uint32 numShaders = (uint32)desc->shaders.size();
            uint32 numShaderGroups = (uint32)desc->shaderGroupDescs.size();

            std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(numShaders);
            std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupCreateInfos(numShaderGroups);

            for (uint32 shaderIndex = 0; shaderIndex < numShaders; shaderIndex++)
            {
                shaderStageCreateInfos[shaderIndex] = {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = ConvertToVkShaderStageFlagBits(desc->shaders[shaderIndex].stage),
                    .pName = desc->shaders[shaderIndex].entry.c_str(),
                };
                VkShaderModuleCreateInfo shaderModuleCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                    .codeSize = desc->shaders[shaderIndex].code.size,
                    .pCode = (uint32*)desc->shaders[shaderIndex].code.data,
                };
                VK_CHECK(vkCreateShaderModule(device.GetHandle(), &shaderModuleCreateInfo, VULKAN_ALLOCATION_CALLBACKS, &shaderStageCreateInfos[shaderIndex].module));
                device.SetDebugUtilsObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64)shaderStageCreateInfos[shaderIndex].module, shaderStageCreateInfos[shaderIndex].pName);
            }

            uint32 numRayGenerationShaders = 0;
            uint32 numMissShaders = 0;
            uint32 numHitGroups = 0;

            for (uint32 groupIndex = 0; groupIndex < numShaderGroups; groupIndex++)
            {
                switch (desc->shaderGroupDescs[groupIndex].type)
                {
                case RenderBackendRayTracingShaderGroupType::RayGen:
                    shaderGroupCreateInfos[groupIndex] = {
                        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                        .generalShader = desc->shaderGroupDescs[groupIndex].rayGenerationShader,
                        .closestHitShader = VK_SHADER_UNUSED_KHR,
                        .anyHitShader = VK_SHADER_UNUSED_KHR,
                        .intersectionShader = VK_SHADER_UNUSED_KHR,
                    };
                    numRayGenerationShaders++;
                    break;
                case RenderBackendRayTracingShaderGroupType::Miss:
                    shaderGroupCreateInfos[groupIndex] = {
                        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                        .generalShader = desc->shaderGroupDescs[groupIndex].missShader,
                        .closestHitShader = VK_SHADER_UNUSED_KHR,
                        .anyHitShader = VK_SHADER_UNUSED_KHR,
                        .intersectionShader = VK_SHADER_UNUSED_KHR,
                    };
                    numMissShaders++;
                    break;
                case RenderBackendRayTracingShaderGroupType::TrianglesHitGroup:
                    shaderGroupCreateInfos[groupIndex] = {
                        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                        .closestHitShader = desc->shaderGroupDescs[groupIndex].closestHitShader,
                        .anyHitShader = desc->shaderGroupDescs[groupIndex].anyHitShader,
                        .intersectionShader = desc->shaderGroupDescs[groupIndex].intersectionShader,
                    };
                    numHitGroups++;
                    break;
                default:
                    std::unreachable();
                    break;
                }
            }
            assert(numRayGenerationShaders == 1);

            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties = device.GetRayTracingPipelineProperties();

            VkPipelineLayout pipelineLayout = device.FindOrCreatePipelineLayout(device.bindlessDescriptorManager.pushConstantSize, RenderBackendPipelineType::RayTracing);

            VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
                .stageCount = (uint32)shaderStageCreateInfos.size(),
                .pStages = shaderStageCreateInfos.data(),
                .groupCount = (uint32)shaderGroupCreateInfos.size(),
                .pGroups = shaderGroupCreateInfos.data(),
                .maxPipelineRayRecursionDepth = Math::Min(desc->maxRayRecursionDepth, rayTracingPipelineProperties.maxRayRecursionDepth),
                .layout = pipelineLayout,
            };

            uint32 index = (uint32)device.rayTracingPipelineStates.size();
            VulkanRayTracingPipelineState& rayTracingPipelineState = device.rayTracingPipelineStates.emplace_back();

            rayTracingPipelineState.pipelineLayout = pipelineLayout;
            rayTracingPipelineState.numRayGenerationShaders = numRayGenerationShaders;
            rayTracingPipelineState.numMissShaders = numMissShaders;
            rayTracingPipelineState.numHitGroups = numHitGroups;

            VK_CHECK(functions.vkCreateRayTracingPipelinesKHR(
                device.GetHandle(),
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                1,
                &rayTracingPipelineCreateInfo,
                VULKAN_ALLOCATION_CALLBACKS,
                &rayTracingPipelineState.handle));

            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }

        return handle;
    }

    RenderBackendBufferHandle VulkanRenderBackend::CreateRayTracingShaderBindingTable(uint32 deviceMask, const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name)
    {
        VulkanDevice& device = devices[0];
        RenderBackendBufferHandle handle = handleManager.Allocate<RenderBackendBufferHandle>(deviceMask);
        {
            VulkanRayTracingPipelineState& rayTracingPipelineState = *device.GetRayTracingPipelineState(desc->rayTracingPipelineState);

            uint32 numMissShaders = rayTracingPipelineState.numMissShaders;
            uint32 numHitGroups = rayTracingPipelineState.numHitGroups;
            uint32 numShaderGroups = 1 + numMissShaders + numHitGroups;

            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties = device.GetRayTracingPipelineProperties();

            const uint32 shaderGroupHandleSizeAligned = AlignUp(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
            const uint32 shaderGroupSizeAligned = AlignUp(shaderGroupHandleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);

            std::vector<uint8> shaderGroupHandles(shaderGroupHandleSizeAligned * numShaderGroups);
            VK_CHECK(functions.vkGetRayTracingShaderGroupHandlesKHR(device.GetHandle(), rayTracingPipelineState.handle, 0, numShaderGroups, shaderGroupHandleSizeAligned * numShaderGroups, shaderGroupHandles.data()));

            // TODO
            uint32 rayGenGroupStride = shaderGroupSizeAligned;
            uint32 missGroupStride = shaderGroupSizeAligned;
            uint32 hitGroupStride = shaderGroupSizeAligned;

            uint32 sbtBufferSize = rayGenGroupStride + numMissShaders * missGroupStride + numHitGroups * hitGroupStride;

            RenderBackendBufferDesc sbtBufferDesc = RenderBackendBufferDesc::CreateShaderBindingTable(sbtBufferSize);
            uint32 index = device.CreateBuffer(&sbtBufferDesc, nullptr, "SBT");
            VulkanBuffer& sbtBuffer = device.buffers[index];
            uint8* sbtBufferData = reinterpret_cast<uint8*>(device.MapBuffer(index));

            for (uint32 groupIndex = 0; groupIndex < numShaderGroups; groupIndex++)
            {
                memcpy(sbtBufferData, shaderGroupHandles.data() + groupIndex * shaderGroupHandleSizeAligned, shaderGroupHandleSizeAligned);
                sbtBufferData += shaderGroupSizeAligned;
            }

            VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = sbtBuffer.handle,
            };
            VkDeviceAddress sbtBufferAddress = functions.vkGetBufferDeviceAddressKHR(device.GetHandle(), &bufferDeviceAddressInfo);

            sbtBuffer.shaderBindingTable = new VulkanRayTracingShaderBindingTable();

            sbtBuffer.shaderBindingTable->rayGenShaderBindingTable = {
                .deviceAddress = sbtBufferAddress,
                .stride = rayGenGroupStride,
                .size = rayGenGroupStride
            };

            sbtBuffer.shaderBindingTable->missShaderBindingTable = {
                .deviceAddress = sbtBufferAddress + sbtBuffer.shaderBindingTable->rayGenShaderBindingTable.size,
                .stride = missGroupStride,
                .size = missGroupStride * numMissShaders
            };

            sbtBuffer.shaderBindingTable->hitShaderBindingTable = {
                .deviceAddress = sbtBufferAddress + sbtBuffer.shaderBindingTable->missShaderBindingTable.size,
                .stride = hitGroupStride,
                .size = hitGroupStride * numHitGroups
            };

            sbtBuffer.shaderBindingTable->callableShaderBindingTable = { 0, 0, 0 };

            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::GetTextureReadbackData(RenderBackendTextureHandle handle, void** data)
    {
        VulkanDevice& device = devices[0];

        uint32 index = device.GetRenderBackendHandleRepresentation(handle.GetIndex());
        VulkanTexture& texture = device.textures[index];

        *data = texture.cpuReadbackBuffer->data;
    }
}

namespace HE
{
    RenderBackend* RenderBackendCreateVulkan(int flags)
    {
        VulkanRenderBackend* vulkanBackend = new VulkanRenderBackend();
        if (!vulkanBackend->Init(flags))
        {
            delete vulkanBackend;
            return nullptr;
        }
        return vulkanBackend;
    }

    void VulkanRenderBackendDestroyBackend(RenderBackend* backend)
    {
        VulkanRenderBackend* vulkanBackend = (VulkanRenderBackend*)backend;
        vulkanBackend->DestroyRenderDevices();
        vulkanBackend->Exit();
        delete vulkanBackend;
    }
}