#include "VulkanRenderBackendCommon.h"
#include "VulkanRenderBackend.h"
#include "VulkanRenderBackendDefinitions.h"
#include "VulkanRenderBackendUtils.h"
#include "VulkanRenderBackendPrivate.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <optick.h>

namespace Horizon
{
    class VulkanRenderBackend;
    class VulkanCommandBufferManager;

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
        VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT shaderDemoteToHelperInvocationFeatures;
        VkPhysicalDeviceScalarBlockLayoutFeaturesEXT scalarBlockLayoutFeaturesEXT;
        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT;
        VkPhysicalDeviceHostQueryResetFeaturesEXT hostQueryResetFeatures;

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
        uint32 CreateBuffer(const RenderBackendBufferDesc* desc, const void* data, const char* name);
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
            const RenderBackendGraphicsPipelineState& pipelineState,
            uint32 pushConstantsSize,
            RenderBackendPrimitiveTopology topology,
            bool useDynamicRendering,
            VulkanRenderingInfo* renderingInfo,
            VkRenderPass renderPass,
            uint32 activeColorAttachmentCount);
        //VulkanPipeline* FindOrCreateRayTracingPipeline(VulkanShader* shader, uint32 pushConstantsSize);
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

    class VulkanRenderBackend final : public RenderBackend
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

        void Tick() override;
        void CreateRenderDevices(PhysicalDeviceID* physicalDeviceIDs, uint32 numDevices, uint32* outDeviceMasks) override;
        void DestroyRenderDevices() override;
        void FlushRenderDevices() override;
        RenderBackendDevice GetNativeDevice() override;
        void GetRenderBackendVulkanInfo(RenderBackendVulkanInfo* vulkanInfo) override;
        RenderBackendSwapChainHandle CreateSwapChain(const RenderBackendSwapChainDesc* desc) override;
        void DestroySwapChain(RenderBackendSwapChainHandle swapChain) override;
        void ResizeSwapChain(RenderBackendSwapChainHandle swapChain, uint32* width, uint32* height) override;
        bool PresentSwapChain(RenderBackendSwapChainHandle swapChain) override;
        RenderBackendTextureHandle GetActiveSwapChainBuffer(RenderBackendSwapChainHandle swapChain) override;
        RenderBackendBufferHandle CreateBuffer(const RenderBackendBufferDesc* desc, const void* data, const char* name) override;
        void DestroyBuffer(RenderBackendBufferHandle buffer) override;
        void ResizeBuffer(RenderBackendBufferHandle buffer, uint64 size) override;
        void MapBuffer(RenderBackendBufferHandle buffer, void** data) override;
        void UnmapBuffer(RenderBackendBufferHandle buffer) override;
        RenderBackendTextureHandle CreateTexture(const RenderBackendTextureDesc* desc, const void* data, const char* name) override;
        void DestroyTexture(RenderBackendTextureHandle texture) override;
        void UploadTexture(RenderBackendTextureHandle handle, const RenderBackendTextureUploadDataDesc& data) override;
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

        VkInstance instance;

        bool enableValidationLayers = false;

        std::vector<const char*> enabledInstanceLayers;
        std::vector<const char*> enabledInstanceExtensions;

        VkDebugUtilsMessengerEXT debugUtilsMessenger;

        VulkanDevice device;

        std::vector<VulkanPhysicalDevice> availablePhysicalDevices;

        VulkanRenderBackendHandleManager handleManager;

        bool enableMeshShaderSupport = false;
        bool enableRayTracingSupport = false;

        struct VulkanFunctions
        {
            // KHR
            PFN_vkGetBufferDeviceAddressKHR                vkGetBufferDeviceAddressKHR = VK_NULL_HANDLE;
            PFN_vkCreateAccelerationStructureKHR           vkCreateAccelerationStructureKHR = VK_NULL_HANDLE;
            PFN_vkDestroyAccelerationStructureKHR          vkDestroyAccelerationStructureKHR = VK_NULL_HANDLE;
            PFN_vkGetAccelerationStructureBuildSizesKHR    vkGetAccelerationStructureBuildSizesKHR = VK_NULL_HANDLE;
            PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = VK_NULL_HANDLE;
            PFN_vkGetRayTracingShaderGroupHandlesKHR       vkGetRayTracingShaderGroupHandlesKHR = VK_NULL_HANDLE;
            PFN_vkBuildAccelerationStructuresKHR           vkBuildAccelerationStructuresKHR = VK_NULL_HANDLE;
            PFN_vkCreateRayTracingPipelinesKHR             vkCreateRayTracingPipelinesKHR = VK_NULL_HANDLE;
            PFN_vkCmdPipelineBarrier2KHR                   vkCmdPipelineBarrier2KHR = VK_NULL_HANDLE;
            PFN_vkCmdBuildAccelerationStructuresKHR        vkCmdBuildAccelerationStructuresKHR = VK_NULL_HANDLE;
            PFN_vkCmdTraceRaysKHR                          vkCmdTraceRaysKHR = VK_NULL_HANDLE;
            // EXT
            PFN_vkCmdDrawMeshTasksEXT                      vkCmdDrawMeshTasksEXT = VK_NULL_HANDLE;
            PFN_vkCmdDrawMeshTasksIndirectEXT              vkCmdDrawMeshTasksIndirectEXT = VK_NULL_HANDLE;
            PFN_vkSetDebugUtilsObjectNameEXT               vkSetDebugUtilsObjectNameEXT = VK_NULL_HANDLE;
            PFN_vkCmdBeginDebugUtilsLabelEXT               vkCmdBeginDebugUtilsLabelEXT = VK_NULL_HANDLE;
            PFN_vkCmdEndDebugUtilsLabelEXT                 vkCmdEndDebugUtilsLabelEXT = VK_NULL_HANDLE;
        };

        VulkanFunctions vulkanFunctions;
    };

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
        uint32 numPhysicalDevices = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, 0));

        if (numPhysicalDevices == 0)
        {
            LogInfo(GLogger, std::format("No available physical device.\n"));
            return;
        }

        std::vector<VkPhysicalDevice> physicalDeviceHandles(numPhysicalDevices);
        vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, physicalDeviceHandles.data());

        availablePhysicalDevices.resize(numPhysicalDevices);

        for (uint32 index = 0; index < numPhysicalDevices; index++)
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
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR,
                .pNext = &physicalDevice.descriptorIndexingFeatures
            };
            physicalDevice.descriptorIndexingFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
                .pNext = &physicalDevice.dynamicRenderingFeatures
            };
            physicalDevice.dynamicRenderingFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
                .pNext = &physicalDevice.timelineSemaphoreFeatures
            };
            physicalDevice.timelineSemaphoreFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR,
                .pNext = &physicalDevice.synchronization2Features
            };
            physicalDevice.synchronization2Features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
                .pNext = &physicalDevice.maintenance4Features
            };
            physicalDevice.maintenance4Features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES_KHR,
                .pNext = &physicalDevice.shaderFloat16Int8Features
            };
            // physicalDevice.maintenance5Features = {
            //     .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR,
            //     .pNext = &physicalDevice.maintenance6Features
            // };
            // physicalDevice.maintenance6Features = {
            //     .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES_KHR,
            //     .pNext = &physicalDevice.shaderFloat16Int8Features
            // };
            physicalDevice.shaderFloat16Int8Features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR,
                .pNext = &physicalDevice.float16StorageFeatures,
            };
            physicalDevice.float16StorageFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR,
                .pNext = &physicalDevice.hostQueryResetFeatures,
            };
            physicalDevice.hostQueryResetFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT,
                .pNext = &physicalDevice.fragmentShaderBarycentricFeatures,
            };
            physicalDevice.fragmentShaderBarycentricFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR,
                .pNext = &physicalDevice.scalarBlockLayoutFeaturesEXT,
            };
            physicalDevice.scalarBlockLayoutFeaturesEXT = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT,
                .pNext = &physicalDevice.multiviewFeatures,
            };
            physicalDevice.multiviewFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR,
                .pNext = &physicalDevice.separateDepthStencilLayoutsFeatures,
            };
            physicalDevice.separateDepthStencilLayoutsFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES_KHR,
                .pNext = &physicalDevice.shaderDemoteToHelperInvocationFeatures,
            };

            if (enableMeshShaderSupport)
            {
                physicalDevice.shaderDemoteToHelperInvocationFeatures = {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT,
                    .pNext = &physicalDevice.meshShaderFeaturesEXT,
                };
                physicalDevice.meshShaderFeaturesEXT = {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
                    .pNext = nullptr,
                };
            }
            else
            {
                physicalDevice.shaderDemoteToHelperInvocationFeatures = {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT,
                    .pNext = nullptr,
                };
            }

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
            physicalDevice.enabledFeatures = deviceFeatures2;

            // TODO
            physicalDevice.meshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader = false;

            LogInfo(
                GLogger,
                std::format("Found physical device (name: {}, type: {}, vendor id: {}, device id: {}, support vulkan version: {}.{}.{})",
                physicalDevice.properties.deviceName,
                (int32)physicalDevice.properties.deviceType,
                physicalDevice.properties.vendorID,
                physicalDevice.properties.deviceID,
                VK_API_VERSION_MAJOR(physicalDevice.properties.apiVersion),
                VK_API_VERSION_MINOR(physicalDevice.properties.apiVersion),
                VK_API_VERSION_PATCH(physicalDevice.properties.apiVersion)));

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
                LogInfo(
                    GLogger,
                    std::format("Available device layer: {} - vulkan apid version: {}.{}.{} - implemetation version: {} - description: {}.",
                    layerProperties.layerName,
                    VK_API_VERSION_MAJOR(layerProperties.specVersion),
                    VK_API_VERSION_MINOR(layerProperties.specVersion),
                    VK_API_VERSION_PATCH(layerProperties.specVersion),
                    layerProperties.implementationVersion,
                    layerProperties.description));
            }

            uint32 numExtensionProperties = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice.handle, nullptr, &numExtensionProperties, nullptr));
            physicalDevice.extensionProperties.resize(numExtensionProperties);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice.handle, nullptr, &numExtensionProperties, physicalDevice.extensionProperties.data()));
            for (const auto& extensionProperty : physicalDevice.extensionProperties)
            {
                LogInfo(
                    GLogger,
                    std::format("Available device extension: {} - extension version: {}.",
                    extensionProperty.extensionName,
                    extensionProperty.specVersion));
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

        enableValidationLayers = flags & VULKAN_RENDER_BACKEND_CREATE_FLAGS_VALIDATION_LAYERS;

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
            //requiredInstanceLayers.push_back("VK_LAYER_NV_optimus");
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
        requiredInstanceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);
        requiredInstanceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
        requiredInstanceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
        requiredInstanceExtensions.push_back(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME);
        requiredInstanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
        requiredInstanceExtensions.push_back(VK_KHR_GET_DISPLAY_PROPERTIES_2_EXTENSION_NAME);

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
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
            .pfnUserCallback = DebugUtilsMessengerCallback
        };

        VkApplicationInfo applicationInfo =
        {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Horizon",
            .applicationVersion = 0,
            .pEngineName = "Horizon Engine",
            .engineVersion = 0,
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

        vulkanFunctions.vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetInstanceProcAddr(instance, "vkGetBufferDeviceAddressKHR"));
        vulkanFunctions.vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetInstanceProcAddr(instance, "vkCreateAccelerationStructureKHR"));
        vulkanFunctions.vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetInstanceProcAddr(instance, "vkDestroyAccelerationStructureKHR"));
        vulkanFunctions.vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureBuildSizesKHR"));
        vulkanFunctions.vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureDeviceAddressKHR"));
        vulkanFunctions.vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupHandlesKHR"));
        vulkanFunctions.vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetInstanceProcAddr(instance, "vkBuildAccelerationStructuresKHR"));
        vulkanFunctions.vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetInstanceProcAddr(instance, "vkCreateRayTracingPipelinesKHR"));
        vulkanFunctions.vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(vkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier2KHR"));
        vulkanFunctions.vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetInstanceProcAddr(instance, "vkCmdBuildAccelerationStructuresKHR"));
        vulkanFunctions.vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetInstanceProcAddr(instance, "vkCmdTraceRaysKHR"));

        vulkanFunctions.vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksEXT"));
        vulkanFunctions.vkCmdDrawMeshTasksIndirectEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksIndirectEXT>(vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectEXT"));
        vulkanFunctions.vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
        vulkanFunctions.vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
        vulkanFunctions.vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));

        enableRayTracingSupport = flags & VULKAN_RENDER_BACKEND_CREATE_FLAGS_RAY_TRACING;

        EnumeratePhysicalDevices();

        return true;
    }

    void VulkanRenderBackend::Exit()
    {
        FlushRenderDevices();

        DestroyRenderDevices();

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

        for (uint32 index = 0; index < RenderBackendMaxRenderTargetCount; index++)
        {
            const RenderBackendRenderPassInfo::RenderTargetBinding& colorRenderTarget = renderPassInfo.renderTargets[index];

            if (!colorRenderTarget.texture)
            {
                continue;
            }

            VulkanTexture* texture = device->GetTexture(colorRenderTarget.texture);
            uint32 mipLevel = renderPassInfo.renderTargets[index].mipLevel;

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

            VkAttachmentDescription& attachmentDesc = outRenderPassDesc->attachmentDescriptions[outRenderPassDesc->attachmentDescriptionCount];
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.format = texture->format;
            attachmentDesc.loadOp = ConvertToVkAttachmentLoadOp(colorRenderTarget.loadOp);
            attachmentDesc.storeOp = ConvertToVkAttachmentStoreOp(colorRenderTarget.storeOp);
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference& colorReference = outRenderPassDesc->colorReferences[outRenderPassDesc->colorAttachmentCount];
            colorReference.attachment = outRenderPassDesc->attachmentDescriptionCount;
            colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            compatibleHashInfo.attachmentCount++;
            compatibleHashInfo.formats[outRenderPassDesc->colorAttachmentCount] = attachmentDesc.format;
            fullHashInfo.loadOps[outRenderPassDesc->colorAttachmentCount] = attachmentDesc.loadOp;
            fullHashInfo.storeOps[outRenderPassDesc->colorAttachmentCount] = attachmentDesc.storeOp;

            outClearValues[outRenderPassDesc->attachmentDescriptionCount].color = texture->clearValue.color;
            outRenderPassDesc->attachmentDescriptionCount++;
            outRenderPassDesc->colorAttachmentCount++;
        }

        if (renderPassInfo.depthStencil.texture)
        {
            const RenderBackendRenderPassInfo::DepthStencilBinding& depthStencilRenderTarget = renderPassInfo.depthStencil;
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

            VkAttachmentDescription& attachmentDesc = outRenderPassDesc->attachmentDescriptions[outRenderPassDesc->attachmentDescriptionCount];
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.format = texture->format;
            attachmentDesc.loadOp = ConvertToVkAttachmentLoadOp(depthStencilRenderTarget.depthLoadOp);
            attachmentDesc.storeOp = ConvertToVkAttachmentStoreOp(depthStencilRenderTarget.depthStoreOp);
            attachmentDesc.stencilLoadOp = ConvertToVkAttachmentLoadOp(depthStencilRenderTarget.stencilLoadOp);
            attachmentDesc.stencilStoreOp = ConvertToVkAttachmentStoreOp(depthStencilRenderTarget.stencilStoreOp);
            attachmentDesc.initialLayout = depthStencilLayout;
            attachmentDesc.finalLayout = depthStencilLayout;

            outRenderPassDesc->depthStencilReference.attachment = outRenderPassDesc->attachmentDescriptionCount;
            outRenderPassDesc->depthStencilReference.layout = depthStencilLayout;

            compatibleHashInfo.formats[RenderBackendMaxRenderTargetCount] = attachmentDesc.format;
            fullHashInfo.loadOps[RenderBackendMaxRenderTargetCount] = attachmentDesc.loadOp;
            fullHashInfo.storeOps[RenderBackendMaxRenderTargetCount] = attachmentDesc.storeOp;
            fullHashInfo.loadOps[RenderBackendMaxRenderTargetCount + 1] = attachmentDesc.stencilLoadOp;
            fullHashInfo.storeOps[RenderBackendMaxRenderTargetCount + 1] = attachmentDesc.stencilStoreOp;

            outClearValues[outRenderPassDesc->attachmentDescriptionCount].depthStencil = texture->clearValue.depthStencil;
            outRenderPassDesc->hasDepthStencil = true;
            outRenderPassDesc->attachmentDescriptionCount++;
        }

        outRenderPassDesc->depthStencilLayout = depthStencilLayout;
        outRenderPassDesc->renderPassCompatibleHash = CRC32(&compatibleHashInfo, sizeof(compatibleHashInfo));
        outRenderPassDesc->renderPassFullHash = CRC32(&fullHashInfo, sizeof(fullHashInfo), outRenderPassDesc->renderPassCompatibleHash);
    }

    static void GetRenderingInfo(VulkanDevice* device, const RenderBackendRenderPassInfo& renderPassInfo, VulkanRenderingInfo* outRenderingInfo)
    {
        bool bSetExtent = true;

        memset(outRenderingInfo, 0, sizeof(VulkanRenderingInfo));

        uint32 layerCount = 1;

        for (uint32 index = 0; index < RenderBackendMaxRenderTargetCount; index++)
        {
            const RenderBackendRenderPassInfo::RenderTargetBinding& colorRenderTarget = renderPassInfo.renderTargets[index];

            if (!colorRenderTarget.texture)
            {
                continue;
            }

            VulkanTexture* texture = device->GetTexture(colorRenderTarget.texture);

            uint32 mipLevel = renderPassInfo.renderTargets[index].mipLevel;
            layerCount = std::max(layerCount, texture->arrayLayers);

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

            VkRenderingAttachmentInfo& attachmentInfo = outRenderingInfo->colorAttachments[outRenderingInfo->colorAttachmentCount];
            attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachmentInfo.pNext = nullptr;
            attachmentInfo.imageView = texture->renderTargetViews[mipLevel];
            attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentInfo.resolveImageView = VK_NULL_HANDLE;
            attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
            attachmentInfo.loadOp = ConvertToVkAttachmentLoadOp(colorRenderTarget.loadOp);
            attachmentInfo.storeOp = ConvertToVkAttachmentStoreOp(colorRenderTarget.storeOp);
            attachmentInfo.clearValue.color = texture->clearValue.color;

            outRenderingInfo->colorAttachmentFormats[outRenderingInfo->colorAttachmentCount] = texture->format;
            outRenderingInfo->colorAttachmentCount++;
        }

        if (renderPassInfo.depthStencil.texture)
        {
            const RenderBackendRenderPassInfo::DepthStencilBinding& depthStencilRenderTarget = renderPassInfo.depthStencil;
            VulkanTexture* texture = device->GetTexture(depthStencilRenderTarget.texture);

            bool hasStencil = IsStencilFormat(texture->format);
            uint32 mipLevel = renderPassInfo.depthStencil.mipLevel;
            layerCount = std::max(layerCount, texture->arrayLayers);

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
            attachmentInfo.imageView = texture->depthStencilViews[0]; // TODO
            attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachmentInfo.resolveImageView = VK_NULL_HANDLE;
            attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
            attachmentInfo.loadOp = ConvertToVkAttachmentLoadOp(depthStencilRenderTarget.depthLoadOp);
            attachmentInfo.storeOp = ConvertToVkAttachmentStoreOp(depthStencilRenderTarget.depthStoreOp);
            attachmentInfo.clearValue.depthStencil = texture->clearValue.depthStencil;

            outRenderingInfo->depthStencilAttachmentFormat = texture->format;
            outRenderingInfo->hasDepthStencil = true;
        }

        VkRect2D renderArea = { 0, 0, outRenderingInfo->extent.width, outRenderingInfo->extent.height };
        if (outRenderingInfo->colorAttachmentCount == 0 && !outRenderingInfo->hasDepthStencil)
        {
            renderArea.offset.x = renderPassInfo.renderArea.x;
            renderArea.offset.y = renderPassInfo.renderArea.y;
            renderArea.extent.width = renderPassInfo.renderArea.width;
            renderArea.extent.height = renderPassInfo.renderArea.height;
        }

        VkRenderingInfo renderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = renderArea,
            .layerCount = layerCount,
            .colorAttachmentCount = outRenderingInfo->colorAttachmentCount,
            .pColorAttachments = outRenderingInfo->colorAttachments,
            .pDepthAttachment = outRenderingInfo->hasDepthStencil ? &outRenderingInfo->depthStencilAttachment : nullptr,
            .pStencilAttachment = nullptr // TODO: add stencil attachment
            //.pStencilAttachment = &outRenderingInfo->depthStencilAttachment
        };

        VkPipelineRenderingCreateInfo pipelineRenderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .colorAttachmentCount = outRenderingInfo->colorAttachmentCount,
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
        VulkanBuffer buffer =
        {
            .size = desc->size,
            .usageFlags = GetVkBufferUsageFlags(desc->flags),
            .name = name,
        };

        VkBufferCreateInfo bufferCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = buffer.size,
            .usage = buffer.usageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        buffer.allocationFlags = GetVmaAllocationCreateFlags(desc->flags);
        buffer.memeryUsage = GetVmaMemoryUsage(desc->flags);
        buffer.createMapped = (buffer.allocationFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT) ? true : false;
        buffer.createMapped = false;
        buffer.indexType = VK_INDEX_TYPE_NONE_KHR;
        if (buffer.usageFlags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        {
            switch (desc->elementSize)
            {
            case 4:
                buffer.indexType = VK_INDEX_TYPE_UINT32; break;
            case 2:
                buffer.indexType = VK_INDEX_TYPE_UINT16; break;
            case 1:
                buffer.indexType = VK_INDEX_TYPE_UINT8_EXT; break;
            default:
                buffer.indexType = VK_INDEX_TYPE_NONE_KHR; break;
            }
        }

        VmaAllocationCreateInfo memoryInfo = {
            .flags = buffer.allocationFlags,
            .usage = buffer.memeryUsage,
        };

        VmaAllocationInfo allocationInfo = {};
        VK_CHECK(vmaCreateBuffer(vmaAllocator, &bufferCreateInfo, &memoryInfo, &buffer.handle, &buffer.allocation, &allocationInfo));

        if (!buffer.name.empty())
        {
            SetDebugUtilsObjectName(VK_OBJECT_TYPE_BUFFER, (uint64)buffer.handle, buffer.name.c_str());
        }

        if (bufferCreateInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
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

            VK_CHECK(vmaMapMemory(vmaAllocator, uploadBuffer.allocation, &uploadBuffer.mappedData));
            memcpy(uploadBuffer.mappedData, data, bufferSize);
            vmaUnmapMemory(vmaAllocator, uploadBuffer.allocation);

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

        buffer.bindlessResourceDescriptorIndexCBV = -1;
        buffer.bindlessResourceDescriptorIndexSRV = -1;
        buffer.bindlessResourceDescriptorIndexUAV = -1;
        if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::UnorderedAccess))
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
                .dstBinding = BINDLESS_RESOURCE_BINDING_BUFFER_SRV_AND_UAV,
                .dstArrayElement = index,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &descriptorBufferInfo,
            };
            vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
            buffer.bindlessResourceDescriptorIndexSRV = buffer.bindlessResourceDescriptorIndexUAV = index;
        }
        else if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::UniformBuffer))
        {
            uint32 index = bindlessDescriptorManager.AllocateUniformBufferIndex();
            VkDescriptorBufferInfo descriptorBufferInfo = {
                .buffer = buffer.handle,
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = bindlessDescriptorManager.set,
                .dstBinding = BINDLESS_RESOURCE_BINDING_BUFFER_CBV,
                .dstArrayElement = index,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &descriptorBufferInfo,
            };
            vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
            buffer.bindlessResourceDescriptorIndexCBV = index;
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

    void VulkanDevice::ResizeBuffer(uint32 index, uint64 size)
    {
        VulkanBuffer& buffer = buffers[index];
        if (buffer.handle != VK_NULL_HANDLE)
        {
            ResourceToDestroy resource =
            {
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

            if ((buffer.usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
            {
                assert(buffer.bindlessResourceDescriptorIndexSRV == buffer.bindlessResourceDescriptorIndexUAV);
                if (buffer.bindlessResourceDescriptorIndexUAV >= 0)
                {
                    VkDescriptorBufferInfo descriptorBufferInfo = {
                       .buffer = buffer.handle,
                       .offset = 0,
                       .range = VK_WHOLE_SIZE
                    };
                    VkWriteDescriptorSet write = {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = bindlessDescriptorManager.set,
                        .dstBinding = BINDLESS_RESOURCE_BINDING_BUFFER_SRV_AND_UAV,
                        .dstArrayElement = (uint32)buffer.bindlessResourceDescriptorIndexUAV,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                        .pBufferInfo = &descriptorBufferInfo,
                    };
                    vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
                }
            }
        }
        else
        {
            buffer.handle = VK_NULL_HANDLE;
            buffer.allocation = VK_NULL_HANDLE;
        }
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

    uint64 VulkanDevice::GetBufferDeviceAddress(RenderBackendBufferHandle bufferHandle)
    {
        uint32 index = GetRenderBackendHandleRepresentation(bufferHandle.GetIndex());
        VulkanBuffer& buffer = buffers[index];
        VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buffer.handle,
        };
        return backend->vulkanFunctions.vkGetBufferDeviceAddressKHR(handle, &bufferDeviceAddressInfo);
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
            .name = name,
            .swapchainBuffer = false,
            .width = desc->width,
            .height = desc->height,
            .depth = desc->depth,
            .arrayLayers = desc->arrayLayerCount,
            .mipLevels = desc->mipLevelCount,
            .format = format,
            .type = ConvertToVkImageType(desc->type),
            .t = desc->type,
            .flags = desc->flags,
            .aspectMask = GetVkImageAspectFlags(format),
            .clearValue = ConvertToVkClearValue(desc->clearValue),
        };

        if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::Readback))
        {
            texture.cpuReadbackBuffer = new VulkanCpuReadbackBuffer();

            uint32 stride = RenderBackendGetTextureFormatDesc(desc->format).bytes;
            uint32 width = texture.width;
            uint32 height = texture.height;
            uint32 depth = texture.depth;
            uint64 size = 0;
            for (uint32 mipLevel = 0; mipLevel < desc->mipLevelCount; mipLevel++)
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

        if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::Sparse))
        {

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

        if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::ShaderResource))
        {
            VkImageViewCreateInfo imageViewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = texture.handle,
                .viewType = ConvertToVkImageViewType(desc->type, texture.IsArray()),
                .format = texture.format,
                .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                .subresourceRange = { texture.aspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS }
            };

            if (IsDepthStencilFormat(texture.format))
            {
                imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            }

            VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.defaultView));

            uint32 index = bindlessDescriptorManager.AllocateSampledImageIndex();
            VkDescriptorImageInfo descriptorImageInfo = {
                .imageView = texture.defaultView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = bindlessDescriptorManager.set,
                .dstBinding = BINDLESS_RESOURCE_BINDING_TEXTURE_SRV,
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

                if (IsDepthStencilFormat(texture.format))
                {
                    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                }

                VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.srvs[mipLevel].srv));

                uint32 index = bindlessDescriptorManager.AllocateSampledImageIndex();
                VkDescriptorImageInfo descriptorImageInfo = {
                    .imageView = texture.srvs[mipLevel].srv,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };
                VkWriteDescriptorSet write = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = bindlessDescriptorManager.set,
                    .dstBinding = BINDLESS_RESOURCE_BINDING_TEXTURE_SRV,
                    .dstArrayElement = index,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .pImageInfo = &descriptorImageInfo,
                };
                vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
                texture.srvs[mipLevel].srvIndex = index;
            }
        }
        if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::UnorderedAccess))
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
                    .dstBinding = BINDLESS_RESOURCE_BINDING_TEXTURE_UAV,
                    .dstArrayElement = index,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &descriptorImageInfo,
                };
                vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
                texture.uavs[mipLevel].uavIndex = index;
            }
        }
        if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::RenderTarget))
        {
            texture.renderTargetViews.resize(texture.mipLevels);
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
                VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.renderTargetViews[i]));
            }
        }
        if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::DepthStencil))
        {
            VkImageView dsv = VK_NULL_HANDLE;
            VkImageViewCreateInfo imageViewInfo =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = texture.handle,
                .viewType = ConvertToVkImageViewType(desc->type, texture.IsArray()),
                .format = texture.format,
                .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                .subresourceRange = { texture.aspectMask, 0, 1, 0, VK_REMAINING_ARRAY_LAYERS }
            };
            VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &dsv));

            texture.depthStencilViews[0] = dsv;
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
            for (uint32 layer = 0; layer < desc->arrayLayerCount; layer++)
            {
                uint32 width = imageInfo.extent.width;
                uint32 height = imageInfo.extent.height;
                uint32 depth = imageInfo.extent.depth;
                for (uint32 level = 0; level < desc->mipLevelCount; level++)
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
                        .aspectMask = texture.aspectMask,
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
                        .aspectMask = texture.aspectMask,
                        .mipLevel = mipLevel - 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                    .srcOffsets = {
                        { .x = 0, .y = 0, .z = 0 },
                        { .x = std::max((int32)(texture.width >> (mipLevel - 1)), 1), .y = std::max((int32)(texture.height >> (mipLevel - 1)), 1), .z = 1,},
                    },
                    .dstSubresource = {
                        .aspectMask = texture.aspectMask,
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
                        .aspectMask = texture.aspectMask,
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
                        .aspectMask = texture.aspectMask,
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
            .viewType = ConvertToVkImageViewType(texture.t, desc->subresourceRange.arrayLayers > 1),
            .format = texture.format,
            .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
            .subresourceRange = { texture.aspectMask, desc->subresourceRange.firstLevel, desc->subresourceRange.mipLevels, desc->subresourceRange.firstLayer, desc->subresourceRange.arrayLayers }
        };
        VK_CHECK(vkCreateImageView(handle, &imageViewInfo, VULKAN_ALLOCATION_CALLBACKS, &texture.defaultView));

        uint32 index = bindlessDescriptorManager.AllocateSampledImageIndex();
        VkDescriptorImageInfo descriptorImageInfo = {
            .imageView = texture.defaultView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = bindlessDescriptorManager.set,
            .dstBinding = BINDLESS_RESOURCE_BINDING_TEXTURE_SRV,
            .dstArrayElement = index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &descriptorImageInfo,
        };
        vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
        texture.srvIndex = index;
        return index;
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
            .dstBinding = BINDLESS_RESOURCE_BINDING_TEXTURE_SRV,
            .dstArrayElement = index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &descriptorImageInfo,
        };
        vkUpdateDescriptorSets(handle, 1, &write, 0, nullptr);
        texture.uavs[desc->mipLevel].uavIndex = index;
        return index;
    }

    int32 VulkanDevice::GetTextureSRVBindlessResourceDescriptorIndex(uint32 textureIndex)
    {
        VulkanTexture& texture = textures[textureIndex];
        return texture.srvIndex;
    }

    int32 VulkanDevice::GetTextureSRVBindlessResourceDescriptorIndex(uint32 textureIndex, uint32 mipLevel)
    {
        VulkanTexture& texture = textures[textureIndex];
        return texture.srvs[mipLevel].srvIndex;
    }

    int32 VulkanDevice::GetTextureUAVBindlessResourceDescriptorIndex(uint32 textureIndex, uint32 mipLevel)
    {
        VulkanTexture& texture = textures[textureIndex];
        return texture.uavs[mipLevel].uavIndex;
    }

    int32 VulkanDevice::GetBufferCBVBindlessResourceDescriptorIndex(uint32 bufferIndex)
    {
        VulkanBuffer& buffer = buffers[bufferIndex];
        return buffer.bindlessResourceDescriptorIndexCBV;
    }

    int32 VulkanDevice::GetBufferSRVBindlessResourceDescriptorIndex(uint32 bufferIndex)
    {
        VulkanBuffer& buffer = buffers[bufferIndex];
        return buffer.bindlessResourceDescriptorIndexSRV;
    }

    int32 VulkanDevice::GetBufferUAVBindlessResourceDescriptorIndex(uint32 bufferIndex)
    {
        VulkanBuffer& buffer = buffers[bufferIndex];
        return buffer.bindlessResourceDescriptorIndexUAV;
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
            .dstBinding = BINDLESS_RESOURCE_BINDING_SAMPLER,
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

    uint32 VulkanDevice::CreateShader(const RenderBackendShaderDesc* desc, const char* name)
    {
        assert(desc->codeSize != 0);
        assert(desc->code != nullptr);
        assert(desc->entryFunctionName != nullptr);

        uint32 shaderIndex = 0;
        if (!freeShaders.empty())
        {
            shaderIndex = freeShaders.back();
            freeShaders.pop_back();
        }
        else
        {
            shaderIndex = static_cast<uint32>(shaders.size());
            shaders.emplace_back();
        }
        VulkanShader& shader = shaders[shaderIndex];

        VkShaderStageFlagBits stage = ConvertToVkShaderStageFlagBits(desc->stage);

        VkShaderModule shaderModule = VK_NULL_HANDLE;
        VkShaderModuleCreateInfo shaderModuleInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = desc->codeSize,
            .pCode = static_cast<const uint32_t*>(desc->code),
        };
        VK_CHECK(vkCreateShaderModule(handle, &shaderModuleInfo, VULKAN_ALLOCATION_CALLBACKS, &shaderModule));
        SetDebugUtilsObjectName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(shaderModule), name);

        VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage,
            .module = shaderModule,
            .pName = desc->entryFunctionName,
        };

        shader.stageInfo = pipelineShaderStageCreateInfo;
        shader.module = shaderModule;

        return shaderIndex;
    }

    void VulkanDevice::DestroyShader(uint32 index)
    {

    }

    uint32 VulkanDevice::CreateAccelerationStructure(VulkanRayTracingAccelerationStructure* accelerationStructure, VkAccelerationStructureTypeKHR type, uint32* primitiveCounts, const char* name)
    {
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
        {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = type,
            .flags = accelerationStructure->buildFlags,
            .geometryCount = (uint32)accelerationStructure->geometries.size(),
            .pGeometries = accelerationStructure->geometries.data(),
        };

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo =
        {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
        };

        backend->vulkanFunctions.vkGetAccelerationStructureBuildSizesKHR(
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
            accelerationStructure->accelerationStructureBuffer.deviceAddress = backend->vulkanFunctions.vkGetBufferDeviceAddressKHR(handle, &bufferDeviceInfo);
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
            accelerationStructure->scratchBuffer.deviceAddress = backend->vulkanFunctions.vkGetBufferDeviceAddressKHR(handle, &bufferDeviceInfo);
        }

        VkAccelerationStructureCreateInfoKHR accelerationStructureInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = accelerationStructure->accelerationStructureBuffer.buffer,
            .size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
            .type = type
        };
        VK_CHECK(backend->vulkanFunctions.vkCreateAccelerationStructureKHR(
            handle,
            &accelerationStructureInfo,
            VULKAN_ALLOCATION_CALLBACKS,
            &accelerationStructure->handle));
        SetDebugUtilsObjectName(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64)accelerationStructure->handle, name);

        VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo =
        {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .accelerationStructure = accelerationStructure->handle,
        };
        accelerationStructure->deviceAddress = backend->vulkanFunctions.vkGetAccelerationStructureDeviceAddressKHR(handle, &deviceAddressInfo);

        const uint32 geometryCount = uint32(accelerationStructure->geometries.size());

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo =
        {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = type,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .dstAccelerationStructure = accelerationStructure->handle,
            .geometryCount = geometryCount,
            .pGeometries = accelerationStructure->geometries.data(),
            .ppGeometries = nullptr,
            .scratchData = { .deviceAddress = accelerationStructure->scratchBuffer.deviceAddress }
        };

        accelerationStructure->buildRangeInfos.resize(geometryCount);
        for (uint32 geometryIndex = 0; geometryIndex < geometryCount; geometryIndex++)
        {
            accelerationStructure->buildRangeInfos[geometryIndex] =
            {
                .primitiveCount = primitiveCounts[geometryIndex],
                .primitiveOffset = 0,
                .firstVertex = 0,
                .transformOffset = 0,
            };
        }

        std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos(geometryCount);
        for (uint32 geometryIndex = 0; geometryIndex < geometryCount; geometryIndex++)
        {
            pBuildRangeInfos[geometryIndex] = &accelerationStructure->buildRangeInfos[geometryIndex];
        }

        // VkCommandBuffer commandBuffer; VkCommandPool pool;
        // VulkanHelper::CreateTemporaryCommandBuffer(handle, GetQueueFamilyIndex(RenderBackendQueueFamily::Graphics), pool, commandBuffer);
        // backend->vulkanFunctions.vkCmdBuildAccelerationStructuresKHR(
        //     commandBuffer,
        //     1,
        //     &accelerationBuildGeometryInfo,
        //     pBuildRangeInfos.data());
        // VulkanHelper::FlushTemporaryCommandBuffer(handle, GetCommandQueue(RenderBackendQueueFamily::Graphics, 0)->handle, pool, commandBuffer);

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
                .dstBinding = BINDLESS_RESOURCE_BINDING_ACCELERATION_STRUCTURE_SRV,
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

    uint32 VulkanDevice::CreateRayTracingBottomLevelAccelerationStructure(const RenderBackendRayTracingBottomLevelAccelerationStructureDesc* desc, const char* name)
    {
        VulkanRayTracingAccelerationStructure accelerationStructure;
        accelerationStructure.buildFlags = ConvertToVkBuildAccelerationStructureFlagsKHR(desc->buildFlags);

        accelerationStructure.geometries.clear();

        std::vector<uint32> primitiveCounts(desc->geometryCount);
        for (uint32 geometryIndex = 0; geometryIndex < desc->geometryCount; geometryIndex++)
        {
            const RenderBackendRayTracingGeometryDesc& geometryDesc = desc->geometryDescs[geometryIndex];

            VkAccelerationStructureGeometryKHR geometry =
            {
               .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
               .geometryType = (VkGeometryTypeKHR)geometryDesc.type,
               .flags = ConvertToVkGeometryFlagsKHR(geometryDesc.flags),
            };
            if (geometry.geometryType == VK_GEOMETRY_TYPE_TRIANGLES_KHR)
            {
                uint32 maxVertex = geometryDesc.triangleDesc.vertexCount;
                geometry.geometry.triangles =
                {
                    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                    .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
                    .vertexData = GetBufferDeviceAddress(geometryDesc.triangleDesc.vertexBuffer) + geometryDesc.triangleDesc.vertexOffset,
                    .vertexStride = geometryDesc.triangleDesc.vertexStride,
                    .maxVertex = maxVertex,
                    .indexType = VK_INDEX_TYPE_UINT32,
                    .indexData = GetBufferDeviceAddress(geometryDesc.triangleDesc.indexBuffer) + geometryDesc.triangleDesc.indexOffset,
                    .transformData = GetBufferDeviceAddress(geometryDesc.triangleDesc.transformBuffer) + geometryDesc.triangleDesc.transformOffset
                };
                primitiveCounts[geometryIndex] = geometryDesc.triangleDesc.indexCount / 3;
            }
            else if (geometry.geometryType == VK_GEOMETRY_TYPE_AABBS_KHR)
            {
                geometry.geometry.aabbs =
                {
                   .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
                   .data = GetBufferDeviceAddress(geometryDesc.aabbDesc.buffer) + geometryDesc.aabbDesc.offset,
                   .stride = sizeof(VkAabbPositionsKHR)
                };
                VulkanBuffer* buffer = GetBuffer(geometryDesc.aabbDesc.buffer);
                primitiveCounts[geometryIndex] = (uint32)(buffer->size / sizeof(VkAabbPositionsKHR));
            }
            accelerationStructure.geometries.push_back(geometry);
        }
        uint32 index = CreateAccelerationStructure(&accelerationStructure, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, primitiveCounts.data(), name);

        return index;
    }

    uint32 VulkanDevice::CreateRayTracingTopLevelAccelerationStructure(const RenderBackendRayTracingTopLevelAccelerationStructureDesc* desc, const char* name)
    {
        VulkanRayTracingAccelerationStructure accelerationStructure = {};
        accelerationStructure.buildFlags = ConvertToVkBuildAccelerationStructureFlagsKHR(desc->buildFlags);

        std::vector<VkAccelerationStructureInstanceKHR> instances;
        for (uint32 i = 0; i < desc->instanceCount; i++)
        {
            VkTransformMatrixKHR transformMatrix;
            memcpy(&transformMatrix, &desc->instances[i].transformMatrix, sizeof(VkTransformMatrixKHR));
            VkAccelerationStructureInstanceKHR instance =
            {
                .transform = transformMatrix,
                .instanceCustomIndex = desc->instances[i].instanceID,
                .mask = desc->instances[i].instanceMask,
                .instanceShaderBindingTableRecordOffset = desc->instances[i].instanceContributionToHitGroupIndex,
                .flags = ConvertToVkGeometryInstanceFlagsKHR(desc->instances[i].flags),
                .accelerationStructureReference = GetRayTracingAccelerationStructure(desc->instances[i].blas)->deviceAddress,
            };
            instances.emplace_back(instance);
        }
        accelerationStructure.instanceCount = desc->instanceCount;

        VulkanRayTracingAccelerationStructure::Buffer instanceBuffer;
        {
            instanceBuffer.size = desc->instanceCount * sizeof(VkAccelerationStructureInstanceKHR);
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
            instanceBuffer.deviceAddress = backend->vulkanFunctions.vkGetBufferDeviceAddressKHR(handle, &bufferDeviceInfo);
            memcpy(instanceBuffer.allocationInfo.pMappedData, instances.data(), instanceBuffer.size);
            VK_CHECK(vmaFlushAllocation(vmaAllocator, instanceBuffer.allocation, 0, instanceBuffer.size));
            accelerationStructure.resourceBuffers.push_back(instanceBuffer);
        }

        VkAccelerationStructureGeometryKHR geometry =
        {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .geometry =
            {
                .instances =
                {
                    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                    .arrayOfPointers = VK_FALSE,
                    .data =
                    {
                        .deviceAddress = instanceBuffer.deviceAddress
                    },
                },
            },
            .flags = ConvertToVkGeometryFlagsKHR(desc->geometryFlags),
        };
        accelerationStructure.geometries.push_back(geometry);

        uint32 numPrimitives = desc->instanceCount;
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
                .colorAttachmentCount = renderPassDesc.colorAttachmentCount,
                .pColorAttachments = renderPassDesc.colorReferences,
                .pDepthStencilAttachment = renderPassDesc.hasDepthStencil ? &renderPassDesc.depthStencilReference : nullptr,
            };
            VkRenderPassCreateInfo renderPassInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = renderPassDesc.attachmentDescriptionCount,
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
        if (framebuffer.colorAttachmentCount != numColorAttachments)
        {
            return false;
        }

        for (uint32 index = 0; index < framebuffer.colorAttachmentCount; index++)
        {
            VkImage image1 = framebuffer.images[index];
            VkImage image2 = device->GetTexture(renderPassInfo.renderTargets[index].texture)->handle;
            if (image1 != image2)
            {
                return false;
            }
        }

        if ((framebuffer.attachmentCount != framebuffer.colorAttachmentCount) && renderPassInfo.depthStencil.texture)
        {
            VkImage image1 = framebuffer.images[framebuffer.colorAttachmentCount];
            VkImage image2 = device->GetTexture(renderPassInfo.depthStencil.texture)->handle;
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

        uint64 mipLevelsAndArrayLayers[RenderBackendMaxRenderTargetCount + 1];
        for (int32 index = 0; index < RenderBackendMaxRenderTargetCount; index++)
        {
            mipLevelsAndArrayLayers[index] = (/*(uint64)renderPassInfo.renderTargets[index].arrayLayer*/uint64(1) << (uint64)32) | (uint64)renderPassInfo.renderTargets[index].mipLevel;
        }
        mipLevelsAndArrayLayers[RenderBackendMaxRenderTargetCount] = (/*(uint64)renderPassInfo.depthStencil.arrayLayer*/uint64(1) << (uint64)32) | (uint64)renderPassInfo.depthStencil.mipLevel;
        uint32 framebufferHash = CRC32(mipLevelsAndArrayLayers, (RenderBackendMaxRenderTargetCount + 1) * sizeof(uint64), renderPassCompatibleHash);

        FramebufferList* framebufferList = nullptr;
        if (cachedFramebuffers.find(framebufferHash) != cachedFramebuffers.end())
        {
            framebufferList = &cachedFramebuffers[framebufferHash];
            for (uint64 index = 0; index < framebufferList->framebuffers.size(); index++)
            {
                if (Matches(this, framebufferList->framebuffers[index], renderPassInfo, renderPassDesc.colorAttachmentCount))
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

        uint32 numAttachments = renderPassDesc.colorAttachmentCount;

        for (uint32 index = 0; index < numAttachments; index++)
        {
            VulkanTexture* texture = GetTexture(renderPassInfo.renderTargets[index].texture);
            assert(texture->width == width && texture->height == height);

            uint32 mipLevel = renderPassInfo.renderTargets[index].mipLevel;

            framebuffer.images[index] = texture->handle;
            framebuffer.attachments[index] = texture->renderTargetViews[mipLevel];
        }
        if (renderPassDesc.hasDepthStencil)
        {
            VulkanTexture* texture = GetTexture(renderPassInfo.depthStencil.texture);
            assert(texture->width == width && texture->height == height);

            uint32 mipLevel = renderPassInfo.depthStencil.mipLevel;

            bool hasStencil = IsStencilFormat(texture->format);

            framebuffer.images[numAttachments] = texture->handle;
            framebuffer.attachments[numAttachments] = texture->depthStencilViews[0];
            numAttachments++;
        }
        framebuffer.colorAttachmentCount = renderPassDesc.colorAttachmentCount;
        framebuffer.attachmentCount = numAttachments;

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

    VkPipelineLayout VulkanDevice::FindOrCreatePipelineLayout(uint32 pushConstantsSize, RenderBackendPipelineType pipelineType)
    {
        uint64 layoutHash = CRC32(&pushConstantsSize, sizeof(uint32), (uint32)pipelineType);
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
            .size = pushConstantsSize
        };
        VkPipelineLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &bindlessDescriptorManager.layout,
            .pushConstantRangeCount = pushConstantsSize ? 1u : 0u,
            .pPushConstantRanges = pushConstantsSize ? &pushConstantRange : nullptr,
        };
        VkPipelineLayout pipelineLayout;
        VK_CHECK(vkCreatePipelineLayout(handle, &layoutInfo, VULKAN_ALLOCATION_CALLBACKS, &pipelineLayout));

        pipelineManager.pipelineLayoutMap.emplace(layoutHash, pipelineLayout);
        pipelineManager.pipelineLayouts.push_back(pipelineLayout);

        return pipelineLayout;
    }

    VulkanPipeline* VulkanDevice::FindOrCreateComputePipeline(VulkanShader* computeShader, uint32 pushConstantsSize)
    {
        uint32 pipelineHash = CRC32(&computeShader->stageInfo, sizeof(VkPipelineShaderStageCreateInfo), pushConstantsSize);

        if (pipelineManager.pipelineMap.find(pipelineHash) != pipelineManager.pipelineMap.end())
        {
            return &pipelineManager.pipelineMap[pipelineHash];
        }

        VkPipelineLayout pipelineLayout = FindOrCreatePipelineLayout(pushConstantsSize, RenderBackendPipelineType::Compute);

        VkComputePipelineCreateInfo computePipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = computeShader->stageInfo,
            .layout = pipelineLayout,
        };

        VkPipeline pipeline;
        VK_CHECK(vkCreateComputePipelines(handle, pipelineManager.pipelineCache, 1, &computePipelineInfo, VULKAN_ALLOCATION_CALLBACKS, &pipeline));

        pipelineManager.pipelineMap.emplace(pipelineHash, VulkanPipeline{ pipelineHash, pipeline, pipelineLayout });
        pipelineManager.pipelines.push_back(VulkanPipeline{ pipelineHash, pipeline, pipelineLayout });

        return &pipelineManager.pipelineMap[pipelineHash];
    }

    static void InitializeVkPipelineRasterizationStateCreateInfo(const RenderBackendRasterizationState& state, VkPipelineRasterizationStateCreateInfo& outInfo)
    {
        outInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        outInfo.depthClampEnable = ConvertToVkBool(state.depthClampEnable);
        outInfo.depthBiasEnable = (state.depthBiasConstantFactor != 0.0f || state.depthBiasSlopeFactor != 0.0f) ? VK_TRUE : VK_FALSE;
        outInfo.depthBiasConstantFactor = state.depthBiasConstantFactor;
        outInfo.depthBiasClamp = state.depthBiasClamp;
        outInfo.depthBiasSlopeFactor = state.depthBiasSlopeFactor;
        outInfo.rasterizerDiscardEnable = VK_FALSE;
        outInfo.polygonMode = ConvertToVkPolygonMode(state.fillMode);
        outInfo.cullMode = ConvertToVkCullModeFlags(state.cullMode);
        outInfo.frontFace = state.frontFaceCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        outInfo.lineWidth = state.lineWidth;
    }

    static void InitializeVkPipelineDepthStencilStateCreateInfo(const RenderBackendDepthStencilState& state, VkPipelineDepthStencilStateCreateInfo& outInfo)
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

    static void InitializeVkPipelineColorBlendStateCreateInfo(const RenderBackendColorBlendState& state, VkPipelineColorBlendAttachmentState* attachmentStates, uint32 attachmentCount, VkPipelineColorBlendStateCreateInfo& outInfo)
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
            attachmentStates[i].colorWriteMask |= EnumClassHasFlags(state.targetBlends[i].writeMask, RenderBackendColorComponentFlags::R) ? VK_COLOR_COMPONENT_R_BIT : 0;
            attachmentStates[i].colorWriteMask |= EnumClassHasFlags(state.targetBlends[i].writeMask, RenderBackendColorComponentFlags::G) ? VK_COLOR_COMPONENT_G_BIT : 0;
            attachmentStates[i].colorWriteMask |= EnumClassHasFlags(state.targetBlends[i].writeMask, RenderBackendColorComponentFlags::B) ? VK_COLOR_COMPONENT_B_BIT : 0;
            attachmentStates[i].colorWriteMask |= EnumClassHasFlags(state.targetBlends[i].writeMask, RenderBackendColorComponentFlags::A) ? VK_COLOR_COMPONENT_A_BIT : 0;
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

    VulkanPipeline* VulkanDevice::FindOrCreateGraphicsPipeline(VulkanShader* vertexShader, VulkanShader* pixelShader, VulkanShader* taskShader, VulkanShader* meshShader, const RenderBackendGraphicsPipelineState& pipelineState, uint32 pushConstantsSize, RenderBackendPrimitiveTopology topology, bool useDynamicRendering, VulkanRenderingInfo* renderingInfo, VkRenderPass renderPass, uint32 activeColorAttachmentCount)
    {
        assert(useDynamicRendering ^ (renderPass != VK_NULL_HANDLE));

        // TODO
        bool useMeshShader = meshShader != nullptr;
        assert(useMeshShader ? vertexShader == nullptr : taskShader == nullptr);

        uint32 colorAttachmentCount = useDynamicRendering ? renderingInfo->colorAttachmentCount : activeColorAttachmentCount;

        VulkanGraphicsPipelineStateDesc pipelineStateDesc = {};
        InitializeVkPipelineRasterizationStateCreateInfo(pipelineState.rasterizationState, pipelineStateDesc.rasterizationStateCreateInfo);
        InitializeVkPipelineDepthStencilStateCreateInfo(pipelineState.depthStencilState, pipelineStateDesc.depthStencilStateCreateInfo);
        InitializeVkPipelineColorBlendStateCreateInfo(pipelineState.colorBlendState, pipelineStateDesc.colorBlendAttachmentStates, colorAttachmentCount, pipelineStateDesc.colorBlendStateCreateInfo);

        // TODO: Optimize this
        uint64 renderingInfoFullHash = renderingInfo ? CRC32(renderingInfo, sizeof(VulkanRenderingInfo)) : uint64(renderPass);
        uint64 pipelineStateDescHash = CRC32(&pipelineStateDesc, sizeof(VulkanGraphicsPipelineStateDesc));
        uint64 values[] = { renderingInfoFullHash, pipelineStateDescHash, uint64(vertexShader), uint64(pixelShader), uint64(taskShader), uint64(meshShader), uint64(topology), uint64(pushConstantsSize) };
        uint64 pipelineHash = uint64(CRC32(values, ArraySize(values) * sizeof(uint64)));

        if (pipelineManager.pipelineMap.find(pipelineHash) != pipelineManager.pipelineMap.end())
        {
            return &pipelineManager.pipelineMap[pipelineHash];
        }

        VkPipelineLayout pipelineLayout = FindOrCreatePipelineLayout(pushConstantsSize, RenderBackendPipelineType::Graphics);

        std::vector<VkPipelineShaderStageCreateInfo> stages;
        if (useMeshShader)
        {
            if (taskShader)
            {
                stages.emplace_back(taskShader->stageInfo);
            }
            if (meshShader)
            {
                stages.emplace_back(meshShader->stageInfo);
            }
        }
        else
        {
            if (vertexShader)
            {
                stages.emplace_back(vertexShader->stageInfo);
            }
        }
        if (pixelShader)
        {
            stages.emplace_back(pixelShader->stageInfo);
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
            .dynamicStateCount = (uint32)ArraySize(dynamicStates),
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

        VkPipelineMultisampleStateCreateInfo multisamplingStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0.0f,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
        };

        VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = useDynamicRendering ? &renderingInfo->pipelineRenderingInfo : nullptr,
            .stageCount = static_cast<uint32_t>(stages.size()),
            .pStages = stages.data(),
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
        VkImage swapchainImages[RenderBackendMaxSwapChainBufferCount] = { 0 };
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

        VkSurfaceFormatKHR surfaceFormat/*TODO:initialize*/;
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

        VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->handle, surface, &surfaceCapabilities));
        assert(surfaceCapabilities.supportedUsageFlags & imageUsage);

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
            .imageUsage = imageUsage,
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
        VkImage swapchainImages[RenderBackendMaxSwapChainBufferCount] = { 0 };
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

        for (uint32 i = 0; i < swapchain.numSemaphores; i++)
        {
            vkWaitForFences(handle, 1, &swapchain.imageAcquiredFences[i], VK_TRUE, UINT64_MAX);
        }

        vkDestroySwapchainKHR(handle, swapchain.handle, VULKAN_ALLOCATION_CALLBACKS);
        vkDestroySurfaceKHR(instance, swapchain.surface, VULKAN_ALLOCATION_CALLBACKS);
        for (uint32 i = 0; i < swapchain.numSemaphores; i++)
        {
            vkDestroyFence(handle, swapchain.imageAcquiredFences[i], VULKAN_ALLOCATION_CALLBACKS);
            vkDestroySemaphore(handle, swapchain.imageAcquiredSemaphores[i], VULKAN_ALLOCATION_CALLBACKS);
        }
        for (uint32 i = 0; i < swapchain.numBuffers; i++)
        {
            DestroyTexture(swapchain.buffers[i].GetIndex());
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

    bool VulkanDevice::Init(VulkanRenderBackend* backend, VulkanPhysicalDevice* physicalDevice, const VulkanBindlessConfig& bindlessConfig)
    {
        this->backend = backend;
        this->physicalDevice = physicalDevice;
        this->instance = backend->instance;
        this->deviceMask = ~uint32(0);

        // Create logical device
        {
            std::vector<const char*> requiredDeviceExtensions;
            requiredDeviceExtensions.push_back(VK_KHR_MAINTENANCE_1_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_MAINTENANCE_2_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_MAINTENANCE_3_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
            //requiredDeviceExtensions.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME); // RenderDoc currently don't support this extension.
            //requiredDeviceExtensions.push_back(VK_KHR_MAINTENANCE_6_EXTENSION_NAME); // RenderDoc currently don't support this extension.
            requiredDeviceExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
            //requiredDeviceExtensions.push_back(VK_KHR_8BIT_STORAGE_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_DEVICE_GROUP_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_PRESENT_WAIT_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_PRESENT_ID_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
            //requiredDeviceExtensions.push_back(VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME); // Required if VK_KHR_synchronization2 is enabled.
            requiredDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME);

#if HORIZON_EXPERIMENTAL_STREAMLINE // TODO
            requiredDeviceExtensions.push_back(VK_NVX_IMAGE_VIEW_HANDLE_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_NVX_BINARY_IMPORT_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_NV_LOW_LATENCY_EXTENSION_NAME);
            requiredDeviceExtensions.push_back(VK_NV_OPTICAL_FLOW_EXTENSION_NAME);
#endif
            if (backend->enableMeshShaderSupport)
            {
                requiredDeviceExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
            }

            if (backend->enableRayTracingSupport)
            {
                requiredDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                requiredDeviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
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
            std::vector<float> queuePriorities[RenderBackendQueueFamilyCount];
            for (uint32 family = 0; family < RenderBackendQueueFamilyCount; family++)
            {
                // don't support optical flow
                //if (family == RenderBackendQueueFamilyCount - 1)
                //{
                //    continue;
                //}

                const uint32 queueCount = numCommandQueues[family];
                // Set all priorities to 1.0 for now.
                queuePriorities[family].resize(queueCount, 1.0f);
                commandQueues[family].resize(queueCount);

                VkDeviceQueueCreateInfo queueInfo =
                {
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

            VkDeviceCreateInfo deviceInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &physicalDevice->enabledFeatures,
                .queueCreateInfoCount = (uint32)(queueInfos.size()),
                .pQueueCreateInfos = queueInfos.data(),
                .enabledExtensionCount = (uint32)(enabledDeviceExtensions.size()),
                .ppEnabledExtensionNames = enabledDeviceExtensions.data(),
                .pEnabledFeatures = nullptr // If the pNext chain includes a VkPhysicalDeviceFeatures2 structure, then pEnabledFeatures must be NULL.
            };

            VK_CHECK(vkCreateDevice(physicalDevice->handle, &deviceInfo, VULKAN_ALLOCATION_CALLBACKS, &handle));

            SetDebugUtilsObjectName(VK_OBJECT_TYPE_DEVICE, (uint64)handle, physicalDevice->properties.deviceName);
        }

        // Init command queues
        {
            for (uint32 family = 0; family < RenderBackendQueueFamilyCount; family++)
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

        return true;
    }


    void VulkanDevice::Shutdown()
    {
        WaitIdle();
        delete commandBufferManager;
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

    void VulkanDevice::SetDebugUtilsObjectName(VkObjectType type, uint64_t handle, const char* name)
    {
        if (!name || !backend->IsInstanceExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT objectNameInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = type,
            .objectHandle = handle,
            .pObjectName = name
        };

        VK_CHECK(backend->vulkanFunctions.vkSetDebugUtilsObjectNameEXT(this->handle, &objectNameInfo));
    }

    bool VulkanDevice::CreateBindlessDescriptorManager(const VulkanBindlessConfig& bindlessConfig)
    {
        const uint32 maxNumSamplers = physicalDevice->descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindSamplers;
        const uint32 maxNumSampledImages = physicalDevice->descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindSampledImages;
        const uint32 maxNumStorageImage = physicalDevice->descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindStorageImages;
        const uint32 maxNumUniformBuffers = physicalDevice->descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindUniformBuffers;
        const uint32 maxNumStorageBuffers = physicalDevice->descriptorIndexingProperties.maxDescriptorSetUpdateAfterBindStorageBuffers;
        const uint32 maxNumAccelerationStructures = physicalDevice->accelerationStructureProperties.maxDescriptorSetAccelerationStructures;

        uint32 numSamplers = Math::Min(bindlessConfig.numSamplers, maxNumSamplers);
        uint32 numSampledImages = Math::Min(bindlessConfig.numSampledImages, maxNumSampledImages);
        uint32 numStorageImages = Math::Min(bindlessConfig.numStorageImages, maxNumStorageImage);
        uint32 numUniformBuffers = Math::Min(bindlessConfig.numUniformBuffers, maxNumUniformBuffers);
        uint32 numStorageBuffers = Math::Min(bindlessConfig.numStorageBuffers, maxNumStorageBuffers);
        uint32 numAccelerationStructures = Math::Min(bindlessConfig.numAccelerationStructures, maxNumAccelerationStructures);

        std::vector<VkDescriptorPoolSize> bindlessPoolSizes;
        std::vector<VkDescriptorSetLayoutBinding> bindlessDescriptorSetLayoutBindings;
        std::vector<VkDescriptorBindingFlags> bindlessDescriptorBindingFlags;

        if (backend->enableRayTracingSupport)
        {
            bindlessPoolSizes =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER,                    numSamplers               },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              numSampledImages          },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              numStorageImages          },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             numUniformBuffers         },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             numStorageBuffers         },
                { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, numAccelerationStructures },
            };

            bindlessDescriptorSetLayoutBindings =
            {
                { .binding = BINDLESS_RESOURCE_BINDING_SAMPLER,                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,                    .descriptorCount = numSamplers,               .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BINDLESS_RESOURCE_BINDING_TEXTURE_SRV,                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              .descriptorCount = numSampledImages,          .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BINDLESS_RESOURCE_BINDING_TEXTURE_UAV,                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              .descriptorCount = numStorageImages,          .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BINDLESS_RESOURCE_BINDING_BUFFER_CBV,                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             .descriptorCount = numUniformBuffers,         .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BINDLESS_RESOURCE_BINDING_BUFFER_SRV_AND_UAV,         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             .descriptorCount = numStorageBuffers,         .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BINDLESS_RESOURCE_BINDING_ACCELERATION_STRUCTURE_SRV, .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, .descriptorCount = numAccelerationStructures, .stageFlags = VK_SHADER_STAGE_ALL },
            };

            bindlessDescriptorBindingFlags =
            {
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
            };
        }
        else
        {
            bindlessPoolSizes =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER,        numSamplers       },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  numSampledImages  },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  numStorageImages  },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numUniformBuffers },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, numStorageBuffers },
            };

            bindlessDescriptorSetLayoutBindings =
            {
                { .binding = BINDLESS_RESOURCE_BINDING_SAMPLER,            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,        .descriptorCount = numSamplers,       .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BINDLESS_RESOURCE_BINDING_TEXTURE_SRV,        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  .descriptorCount = numSampledImages,  .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BINDLESS_RESOURCE_BINDING_TEXTURE_UAV,        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  .descriptorCount = numStorageImages,  .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BINDLESS_RESOURCE_BINDING_BUFFER_CBV,         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = numUniformBuffers, .stageFlags = VK_SHADER_STAGE_ALL },
                { .binding = BINDLESS_RESOURCE_BINDING_BUFFER_SRV_AND_UAV, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = numStorageBuffers, .stageFlags = VK_SHADER_STAGE_ALL },
            };

            bindlessDescriptorBindingFlags =
            {
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
            };
        }
        assert(bindlessDescriptorSetLayoutBindings.size() == bindlessDescriptorBindingFlags.size());

        VkDescriptorPoolCreateInfo descriptorPoolInfo =
        {
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

        VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlagsInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = numBindings,
            .pBindingFlags = bindlessDescriptorBindingFlags.data()
        };

        const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo =
        {
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

        const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
        {
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

        // To be able to bind a set once in a frame for all shaders_deprecated, all pipeline layouts have to be compatible
        bindlessDescriptorManager.pushConstantsSize = 128;
        bindlessDescriptorManager.compatibleGraphicsPipelineLayout = FindOrCreatePipelineLayout(bindlessDescriptorManager.pushConstantsSize, RenderBackendPipelineType::Graphics);
        bindlessDescriptorManager.compatibleComputePipelineLayout = FindOrCreatePipelineLayout(bindlessDescriptorManager.pushConstantsSize, RenderBackendPipelineType::Compute);
        if (backend->enableRayTracingSupport)
        {
            bindlessDescriptorManager.compatibleRayTracingPipelineLayout = FindOrCreatePipelineLayout(bindlessDescriptorManager.pushConstantsSize, RenderBackendPipelineType::RayTracing);
        }

        bindlessDescriptorManager.config = {
            .numSamplers = numSamplers,
            .numSampledImages = numSampledImages,
            .numStorageImages = numStorageImages,
            .numUniformBuffers = numUniformBuffers,
            .numStorageBuffers = numStorageBuffers,
            .numAccelerationStructures = numAccelerationStructures,
        };

        for (int32 i = numSamplers - 1; i >= 0; i--)
        {
            bindlessDescriptorManager.freeSamplers.push_back(i);
        }
        for (int32 i = numSampledImages - 1; i >= 0; i--)
        {
            bindlessDescriptorManager.freeSampledImages.push_back(i);
        }

        for (int32 i = numStorageImages - 1; i >= 0; i--)
        {
            bindlessDescriptorManager.freeStorageImages.push_back(i);
        }
        for (int32 i = numUniformBuffers - 1; i >= 0; i--)
        {
            bindlessDescriptorManager.freeUniformBuffers.push_back(i);
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
        if (bindlessDescriptorManager.compatibleComputePipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(handle, bindlessDescriptorManager.compatibleComputePipelineLayout, VULKAN_ALLOCATION_CALLBACKS);
            bindlessDescriptorManager.compatibleComputePipelineLayout = VK_NULL_HANDLE;
        }
        if (bindlessDescriptorManager.compatibleGraphicsPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(handle, bindlessDescriptorManager.compatibleGraphicsPipelineLayout, VULKAN_ALLOCATION_CALLBACKS);
            bindlessDescriptorManager.compatibleGraphicsPipelineLayout = VK_NULL_HANDLE;
        }
        if (bindlessDescriptorManager.compatibleRayTracingPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(handle, bindlessDescriptorManager.compatibleRayTracingPipelineLayout, VULKAN_ALLOCATION_CALLBACKS);
            bindlessDescriptorManager.compatibleRayTracingPipelineLayout = VK_NULL_HANDLE;
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
        bool CompileRenderBackendCommand(const RenderBackendCommandClearBufferUAV& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandClearTextureUAV& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBarriers& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandTransitions& command);
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
        bool PrepareForDispatch(RenderBackendShaderHandle computeShader, const RenderBackendShaderConstants& shaderConstants);
        bool PrepareForDraw(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& pipelineState, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendShaderConstants& shaderConstants);
        bool PrepareForMeshShading(RenderBackendShaderHandle amplificationShader, RenderBackendShaderHandle meshShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& pipelineState, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendShaderConstants& shaderConstants);
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

        assert(!EnumClassHasFlags(srcTexture->flags, RenderBackendTextureCreateFlags::Readback));
        if (EnumClassHasFlags(dstTexture->flags, RenderBackendTextureCreateFlags::Readback))
        {
            VkBufferImageCopy copy[RenderBackendMaxMipLevelCount] = {};
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
        const VulkanBuffer* buffer = device->GetBuffer(command.buffer);

        vkCmdUpdateBuffer(commandBuffer, buffer->handle, command.offset, command.size, command.data);

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandUpdateTexture& command)
    {
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandClearBufferUAV& command)
    {
        const VulkanBuffer* buffer = device->GetBuffer(command.buffer);

        vkCmdFillBuffer(
            commandBuffer,
            buffer->handle,
            0,
            buffer->size,
            command.data);

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandClearTextureUAV& command)
    {
        const VulkanTexture* texture = device->GetTexture(command.uav.texture);

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
        for (uint32 i = 0; i < command.transitionCount; i++)
        {
            const auto& transition = command.transitions[i];

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
                        .aspectMask = GetVkImageAspectFlags(texture->format),
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
            VkDependencyInfo dependency =
            {
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
            device->GetBackend()->vulkanFunctions.vkCmdPipelineBarrier2KHR(commandBuffer, &dependencyInfo);
            bufferBarriers.clear();
            imageBarriers.clear();
        }
    }

    bool VulkanRenderBackendCommandListContext::PrepareForDispatch(RenderBackendShaderHandle computeShader, const RenderBackendShaderConstants& shaderConstants)
    {
        uint32 pushConstantsSize = device->bindlessDescriptorManager.pushConstantsSize;
        VulkanPipeline* pipeline = device->FindOrCreateComputePipeline(device->GetShader(computeShader), pushConstantsSize);

        if (pipeline->handle != activeComputePipeline)
        {
            VkDescriptorSet set = device->GetBindlessGlobalSet();
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->handle);
            activeComputePipeline = pipeline->handle;
        }

        if (pushConstantsSize > 0)
        {
            const void* pushConstantsData = &shaderConstants.data;
            vkCmdPushConstants(commandBuffer, pipeline->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, pushConstantsSize, pushConstantsData);
        }

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatch& command)
    {
        if (!PrepareForDispatch(command.computeShader, command.shaderConstants))
        {
            return false;
        }
        vkCmdDispatch(commandBuffer, command.threadGroupCountX, command.threadGroupCountY, command.threadGroupCountZ);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchIndirect& command)
    {
        if (!PrepareForDispatch(command.computeShader, command.shaderConstants))
        {
            return false;
        }
        VulkanBuffer* argumentBuffer = device->GetBuffer(command.argumentBuffer);
        vkCmdDispatchIndirect(commandBuffer, argumentBuffer->handle, command.argumentBufferOffset);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBuildRayTracingBottomLevelAccelerationStructure& command)
    {
        const VulkanRayTracingAccelerationStructure* srcBLAS = command.srcBLAS ? device->GetRayTracingAccelerationStructure(command.srcBLAS) : nullptr;
        const VulkanRayTracingAccelerationStructure* dstBLAS = device->GetRayTracingAccelerationStructure(command.dstBLAS);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
        {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags = dstBLAS->buildFlags,
            .mode = srcBLAS ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .srcAccelerationStructure = srcBLAS ? srcBLAS->handle : VK_NULL_HANDLE,
            .dstAccelerationStructure = dstBLAS->handle,
            .geometryCount = uint32(dstBLAS->geometries.size()),
            .pGeometries = dstBLAS->geometries.data(),
            .ppGeometries = nullptr,
            .scratchData = { .deviceAddress = dstBLAS->scratchBuffer.deviceAddress }
        };

        std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos(uint32(dstBLAS->geometries.size()));
        for (uint32 geometryIndex = 0; geometryIndex < uint32(dstBLAS->geometries.size()); geometryIndex++)
        {
            pBuildRangeInfos[geometryIndex] = &dstBLAS->buildRangeInfos[geometryIndex];
        }

        device->GetBackend()->vulkanFunctions.vkCmdBuildAccelerationStructuresKHR(
            commandBuffer,
            1,
            &accelerationStructureBuildGeometryInfo,
            pBuildRangeInfos.data());

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBuildRayTracingTopLevelAccelerationStructure& command)
    {
        const VulkanRayTracingAccelerationStructure* srcTLAS = command.srcTLAS ? device->GetRayTracingAccelerationStructure(command.srcTLAS) : nullptr;
        const VulkanRayTracingAccelerationStructure* dstTLAS = device->GetRayTracingAccelerationStructure(command.dstTLAS);

        // If type is VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, geometryCount must be 1.
        assert(dstTLAS->geometries.size() == 1);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
        {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
            .flags = dstTLAS->buildFlags,
            .mode = srcTLAS ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .srcAccelerationStructure = srcTLAS ? srcTLAS->handle : VK_NULL_HANDLE,
            .dstAccelerationStructure = dstTLAS->handle,
            .geometryCount = uint32(dstTLAS->geometries.size()),
            .pGeometries = dstTLAS->geometries.data(),
            .ppGeometries = nullptr,
            .scratchData = { .deviceAddress = dstTLAS->scratchBuffer.deviceAddress }
        };

        VkAccelerationStructureBuildRangeInfoKHR range =
        {
            .primitiveCount = dstTLAS->instanceCount,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0,
        };

        VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfo = &range;

        device->GetBackend()->vulkanFunctions.vkCmdBuildAccelerationStructuresKHR(
            commandBuffer,
            1,
            &accelerationStructureBuildGeometryInfo,
            &buildRangeInfo);

        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchRays& command)
    {
        uint32 pushConstantsSize = device->bindlessDescriptorManager.pushConstantsSize;
        VulkanRayTracingPipelineState* pipelineState = device->GetRayTracingPipelineState(command.pipelineState);

        if (pipelineState->handle != activeRayTracingPipeline)
        {
            VkDescriptorSet set = device->GetBindlessGlobalSet();
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineState->handle);
            activeComputePipeline = pipelineState->handle;
        }

        if (pushConstantsSize > 0)
        {
            const void* pushConstantsData = &command.shaderConstants.data;
            vkCmdPushConstants(commandBuffer, pipelineState->pipelineLayout, VK_SHADER_STAGE_ALL, 0, pushConstantsSize, pushConstantsData);
        }

        VulkanBuffer* sbtBuffer = device->GetBuffer(command.shaderBindingTable);

        device->GetBackend()->vulkanFunctions.vkCmdTraceRaysKHR(
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
        GetRenderingInfo(device, command.renderPassInfo, &renderingInfo);
        vkCmdBeginRendering(commandBuffer, &renderingInfo.renderingInfo);
        insideRenderPass = true;
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandEndRenderPass& command)
    {
        vkCmdEndRendering(commandBuffer);
        insideRenderPass = false;
        return true;
    }

    bool VulkanRenderBackendCommandListContext::PrepareForDraw(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& pipelineState, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendShaderConstants& shaderConstants)
    {
        uint32 pushConstantsSize = device->bindlessDescriptorManager.pushConstantsSize;

        assert(insideRenderPass);
        VulkanPipeline* pipeline = device->FindOrCreateGraphicsPipeline(
            device->GetShader(vertexShader),
            device->GetShader(pixelShader),
            nullptr,
            nullptr,
            pipelineState,
            pushConstantsSize,
            topology,
            true,
            &renderingInfo,
            VK_NULL_HANDLE,
            0);

        if (pipeline->handle != activeGraphicsPipeline)
        {
            VkDescriptorSet set = device->GetBindlessGlobalSet();
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle);
            activeGraphicsPipeline = pipeline->handle;
        }
        if (pushConstantsSize > 0)
        {
            const void* pushConstantsData = &shaderConstants.data;
            vkCmdPushConstants(commandBuffer, pipeline->layout, VK_SHADER_STAGE_ALL, 0, pushConstantsSize, pushConstantsData);
        }
        if (indexBuffer)
        {
            VulkanBuffer* buffer = device->GetBuffer(indexBuffer);
            vkCmdBindIndexBuffer(commandBuffer, buffer->handle, 0, buffer->indexType);
        }
        ApplyTransitions();
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDraw& command)
    {
        OPTICK_EVENT();

        if (!PrepareForDraw(command.vertexShader, command.pixelShader, command.pipelineState, command.topology, command.indexBuffer, command.shaderConstants))
        {
            return false;
        }
        if (!command.indexBuffer)
        {
            vkCmdDraw(
                commandBuffer,
                command.draw.vertexCount,
                command.draw.instanceCount,
                command.draw.firstVertex,
                command.draw.firstInstance);
        }
        else
        {
            vkCmdDrawIndexed(
                commandBuffer,
                command.drawIndexed.indexCount,
                command.drawIndexed.instanceCount,
                command.drawIndexed.firstIndex,
                command.drawIndexed.vertexOffset,
                command.drawIndexed.firstInstance);
        }
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDrawIndirect& command)
    {
        if (!PrepareForDraw(command.vertexShader, command.pixelShader, command.pipelineState, command.topology, command.indexBuffer, command.shaderConstants))
        {
            return false;
        }
        if (!command.indexBuffer)
        {
            vkCmdDrawIndirect(
                commandBuffer,
                device->GetBuffer(command.argumentBuffer)->handle,
                command.argumentBufferOffset,
                command.drawCount,
                sizeof(VkDrawIndirectCommand));
        }
        else
        {
            vkCmdDrawIndexedIndirect(
                commandBuffer,
                device->GetBuffer(command.argumentBuffer)->handle,
                command.argumentBufferOffset,
                command.drawCount,
                sizeof(VkDrawIndexedIndirectCommand));
        }
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchMesh& command)
    {
        // if (!PrepareForDraw(nullptr, command.pixelShader, command.pipelineState, command.topology, RenderBackendBufferHandle::Null, command.shaderConstants))
        // {
        //     return false;
        // }
        // device->backend->vulkanFunctions.vkCmdDrawMeshTasksEXT(
        //     commandBuffer,
        //     command.threadGroupCountX,
        //     command.threadGroupCountY,
        //     command.threadGroupCountZ);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchMeshIndirect& command)
    {
        // if (!PrepareForDraw(nullptr, command.pixelShader, command.pipelineState, command.topology, RenderBackendBufferHandle::Null, command.shaderConstants))
        // {
        //     return false;
        // }
        // VulkanBuffer* argumentBuffer = device->GetBuffer(command.argumentBuffer);
        // vkCmdDispatchIndirect(commandBuffer, argumentBuffer->handle, command.argumentBufferOffset);
        // device->backend->vulkanFunctions.vkCmdDrawMeshTasksIndirectEXT(
        //     commandBuffer,
        //     argumentBuffer->handle,
        //     command.argumentBufferOffset,
        //     command.numDraws,
        //     sizeof(RenderBackendDispatchMeshIndirectArguments));
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandSetStencilReference& command)
    {
        vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FRONT_AND_BACK, command.stencilReference);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandSetScissor& command)
    {
        VkRect2D scissors[RenderBackendMaxViewportCount];
        for (uint32 i = 0; i < command.scissorCount; i++)
        {
            scissors[i] =
            {
                .offset = { .x = command.scissors[i].left, .y = command.scissors[i].top },
                .extent = { .width = command.scissors[i].width, .height = command.scissors[i].height }
            };
        }
        vkCmdSetScissor(commandBuffer, 0, command.scissorCount, scissors);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandSetViewport& command)
    {
        VkViewport viewports[RenderBackendMaxViewportCount];
        for (uint32 i = 0; i < command.viewportCount; i++)
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
        vkCmdSetViewport(commandBuffer, 0, command.viewportCount, viewports);
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBeginDebugLabel& command)
    {
#if !HE_ENBALE_STREAMLINE_SUPPORT
        if (device->backend->enableValidationLayers)
        {
            VkDebugUtilsLabelEXT lableInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                .pLabelName = command.labelName,
                .color = { command.color[0], command.color[1], command.color[2], command.color[3] }
            };
            device->backend->vulkanFunctions.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &lableInfo);
        }
#endif
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandEndDebugLabel& command)
    {
#if !HE_ENBALE_STREAMLINE_SUPPORT
        if (device->backend->enableValidationLayers)
        {
            device->backend->vulkanFunctions.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
        }
#endif
        return true;
    }

    bool VulkanRenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchSuperSampling& command)
    {
        VulkanTexture* outputTexture = device->GetTexture(command.output);
        VulkanTexture* colorTexture = device->GetTexture(command.color);
        VulkanTexture* depthTexture = device->GetTexture(command.depth);
        VulkanTexture* motionVectorTexture = device->GetTexture(command.motionVectors);
        VulkanTexture* exposureTexture = device->GetTexture(command.exposure);

        auto GetRenderBackendTextureResourceVulkan = [](VulkanTexture* texture, bool uav)
        {
            RenderBackendTextureResource textureResource = {};
            textureResource.texture = texture->handle;
            textureResource.info = &texture->info;
            textureResource.memory = texture->allocation->GetMemory();
            textureResource.view = uav ? texture->uavs[0].uav : texture->defaultView;
            textureResource.width = texture->width;
            textureResource.height = texture->height;
            textureResource.mipLevels = texture->mipLevels;
            textureResource.arrayLayers = texture->arrayLayers;
            textureResource.format = texture->format;
            textureResource.state = uav ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureResource.flags = texture->info.flags;
            textureResource.usage = texture->info.usage;
            return textureResource;
        };

        RenderBackendTextureResource output = GetRenderBackendTextureResourceVulkan(outputTexture, true);
        RenderBackendTextureResource color = GetRenderBackendTextureResourceVulkan(colorTexture, false);
        RenderBackendTextureResource depth = GetRenderBackendTextureResourceVulkan(depthTexture, false);
        RenderBackendTextureResource motionVectors = GetRenderBackendTextureResourceVulkan(motionVectorTexture, false);
        RenderBackendTextureResource exposure = GetRenderBackendTextureResourceVulkan(exposureTexture, false);

        bool succeed = command.callback(static_cast<void*>(commandBuffer), command.context, output, color, depth, motionVectors, exposure);

        // Restore bindless global descriptor set
        device->BindBindlessDescriptorSets(commandBuffer);

        return succeed;
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
    break

        for (uint32 i = 0; i < container.numCommands; i++)
        {
            switch (container.types[i])
            {
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandCopyBuffer);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandCopyTexture);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandUpdateBuffer);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandUpdateTexture);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandClearBufferUAV);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandClearTextureUAV);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBarriers);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandTransitions);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBeginTimingQuery);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandEndTimingQuery);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandResolveTimingQueryResults);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatch);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchIndirect);
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
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBuildRayTracingBottomLevelAccelerationStructure);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBuildRayTracingTopLevelAccelerationStructure);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchRays);
            COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchSuperSampling);
            default: std::unreachable();
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
            .numUniformBuffers = 16,
            .numStorageBuffers = 8 * 1024,
            .numAccelerationStructures = 8 * 1024
        };

        for (uint32 i = 0; i < numPhysicalDevices; i++)
        {
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
        device.Shutdown();
    }

    void VulkanRenderBackend::FlushRenderDevices()
    {
        device.WaitIdle();
    }

    RenderBackendDevice VulkanRenderBackend::GetNativeDevice()
    {
        RenderBackendDevice d = {};
        d.device = device.handle;
        d.physicalDevice = device.physicalDevice->handle;
        return d;
    }

    void VulkanRenderBackend::GetRenderBackendVulkanInfo(RenderBackendVulkanInfo* vulkanInfo)
    {
        vulkanInfo->instance = instance;
        vulkanInfo->device = device.handle;
        vulkanInfo->physicalDevice = device.physicalDevice->handle;

        vulkanInfo->computeQueueIndex = device.commandQueues[(uint32)RenderBackendQueueFamily::Compute][0].queueIndex;
        vulkanInfo->computeQueueFamily = device.commandQueues[(uint32)RenderBackendQueueFamily::Compute][0].familyIndex;
        vulkanInfo->graphicsQueueIndex = device.commandQueues[(uint32)RenderBackendQueueFamily::Graphics][0].queueIndex;
        vulkanInfo->graphicsQueueFamily = device.commandQueues[(uint32)RenderBackendQueueFamily::Graphics][0].familyIndex;
        //vulkanInfo->opticalFlowQueueIndex = device.commandQueues[(uint32)RenderBackendQueueFamily::OpticalFlow][0].queueIndex;
        //vulkanInfo->opticalFlowQueueFamily = device.commandQueues[(uint32)RenderBackendQueueFamily::OpticalFlow][0].familyIndex;
    }

    void VulkanRenderBackend::Tick()
    {
        device.Tick();
    }

    RenderBackendSwapChainHandle VulkanRenderBackend::CreateSwapChain(const RenderBackendSwapChainDesc* desc)
    {
        uint32 index = device.CreateSwapChain(desc);
        RenderBackendSwapChainHandle handle = handleManager.Allocate<RenderBackendSwapChainHandle>();
        device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    void VulkanRenderBackend::DestroySwapChain(RenderBackendSwapChainHandle handle)
    {
        uint32 index = device.GetRenderBackendHandleRepresentation(handle.GetIndex());
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return;
        }
        device.DestroySwapChain(index);
        device.RemoveRenderBackendHandleRepresentation(handle.GetIndex());
    }

    void VulkanRenderBackend::ResizeSwapChain(RenderBackendSwapChainHandle handle, uint32* width, uint32* height)
    {
        uint32 index = device.GetRenderBackendHandleRepresentation(handle.GetIndex());
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return;
        }
        device.ResizeSwapChain(index, width, height);
    }

    bool VulkanRenderBackend::PresentSwapChain(RenderBackendSwapChainHandle handle)
    {
        OPTICK_EVENT();

        uint32 index = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return false;
        }
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
        return true;
    }

    RenderBackendTextureHandle VulkanRenderBackend::GetActiveSwapChainBuffer(RenderBackendSwapChainHandle handle)
    {
        uint32 index = device.GetRenderBackendHandleRepresentation(handle.GetIndex());
        return device.GetActiveSwapChainBackBuffer(index);
    }

    RenderBackendBufferHandle VulkanRenderBackend::CreateBuffer(const RenderBackendBufferDesc* desc, const void* data, const char* name)
    {
        RenderBackendBufferHandle handle = handleManager.Allocate<RenderBackendBufferHandle>();
        uint32 index = device.CreateBuffer(desc, data, name);
        device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    void VulkanRenderBackend::DestroyBuffer(RenderBackendBufferHandle handle)
    {
        uint32 index = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return;
        }
        device.DestroyBuffer(index);
        device.RemoveRenderBackendHandleRepresentation(handle.GetIndex());
    }

    void VulkanRenderBackend::ResizeBuffer(RenderBackendBufferHandle handle, uint64 size)
    {
        uint32 index = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return;
        }
        device.ResizeBuffer(index, size);
    }

    void VulkanRenderBackend::MapBuffer(RenderBackendBufferHandle handle, void** data)
    {
        uint32 index = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return;
        }
        *data = device.MapBuffer(index);
    }

    void VulkanRenderBackend::UnmapBuffer(RenderBackendBufferHandle handle)
    {
        uint32 index = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return;
        }
        device.UnmapBuffer(index);
    }

    RenderBackendTextureHandle VulkanRenderBackend::CreateTexture(const RenderBackendTextureDesc* desc, const void* data, const char* name)
    {
        RenderBackendTextureHandle handle = handleManager.Allocate<RenderBackendTextureHandle>();
        uint32 index = device.CreateTexture(desc, data, name);
        device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    void VulkanRenderBackend::DestroyTexture(RenderBackendTextureHandle handle)
    {
        uint32 index = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return;
        }
        device.DestroyTexture(index);
        device.RemoveRenderBackendHandleRepresentation(handle.GetIndex());
    }

    void VulkanRenderBackend::UploadTexture(RenderBackendTextureHandle handle, const RenderBackendTextureUploadDataDesc& data)
    {
        {
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                return;
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

                    //uint32 rowLength = ComputeWorkGroupCount(subresourceData.width, blockWidth);
                    //uint32 imageHeight = ComputeWorkGroupCount(subresourceData.height, blockHeight);

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
                // for (uint32 level = 0; level < texture.mipLevelCount; level++)
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

    RenderBackendSamplerHandle VulkanRenderBackend::CreateSampler(const RenderBackendSamplerDesc* desc, const char* name)
    {
        RenderBackendSamplerHandle handle = handleManager.Allocate<RenderBackendSamplerHandle>();
        {
            uint32 index = device.CreateSampler(desc, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::DestroySampler(RenderBackendSamplerHandle handle)
    {
        {
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                return;
            }
            device.DestroySampler(index);
            device.RemoveRenderBackendHandleRepresentation(handle.GetIndex());
        }
    }

    RenderBackendShaderHandle VulkanRenderBackend::CreateShader(const RenderBackendShaderDesc* desc, const char* name)
    {
        RenderBackendShaderHandle handle = handleManager.Allocate<RenderBackendShaderHandle>();
        {
            uint32 index = device.CreateShader(desc, name);
            device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        }
        return handle;
    }

    void VulkanRenderBackend::DestroyShader(RenderBackendShaderHandle handle)
    {
        {
            uint32 index = 0;
            if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
            {
                return;
            }
            device.DestroyShader(index);
            device.RemoveRenderBackendHandleRepresentation(handle.GetIndex());
        }
    }

    RenderBackendTimingQueryHeapHandle VulkanRenderBackend::CreateTimingQueryHeap(const RenderBackendTimingQueryHeapDesc* desc, const char* name)
    {
        RenderBackendTimingQueryHeapHandle handle = handleManager.Allocate<RenderBackendTimingQueryHeapHandle>();
        {
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

    RenderBackendRayTracingAccelerationStructureHandle VulkanRenderBackend::CreateRayTracingTopLevelAccelerationStructure(const RenderBackendRayTracingTopLevelAccelerationStructureDesc* desc, const char* name)
    {
        RenderBackendRayTracingAccelerationStructureHandle handle = handleManager.Allocate<RenderBackendRayTracingAccelerationStructureHandle>();
        uint32 index = device.CreateRayTracingTopLevelAccelerationStructure(desc, name);
        device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    RenderBackendRayTracingAccelerationStructureHandle VulkanRenderBackend::CreateRayTracingBottomLevelAccelerationStructure(const RenderBackendRayTracingBottomLevelAccelerationStructureDesc* desc, const char* name)
    {
        RenderBackendRayTracingAccelerationStructureHandle handle = handleManager.Allocate<RenderBackendRayTracingAccelerationStructureHandle>();
        uint32 index = device.CreateRayTracingBottomLevelAccelerationStructure(desc, name);
        device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    void VulkanRenderBackend::SubmitCommandLists(RenderBackendCommandList** commandLists, uint32 numCommandLists, RenderBackendSwapChainHandle swapChain)
    {
        OPTICK_EVENT();

        if (!commandLists || numCommandLists == 0)
        {
            return;
        }

        RenderBackendCommandContainer* commandContainer = commandLists[0]->GetCommandContainer();
        uint32 numCommands = commandContainer->numCommands;

        //if (numCommands == 0)
        //{
        //    return;
        //}

        std::vector<VkSubmitInfo2> submitInfos[RenderBackendQueueFamilyCount];
        VulkanSubmitContext submitContext = {};

        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreInfos;
        std::vector<VkSemaphoreSubmitInfo> signalSemaphoreInfos;

        uint32 deviceMask = ~0u;
        uint32 queueFamily = (uint32)RenderBackendQueueFamily::Graphics;

        //for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            //VulkanDevice& device = devices[deviceIndex];
            //VulkanSubmitContext& submitContext = submitContexts[deviceIndex];
            //if ((device.GetDeviceMask() & deviceMask) == 0)
            //{
            //    continue;
            //}
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
                    .semaphore = submitContext.completeSemaphore,
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
            submitInfos[queueFamily].emplace_back(submitInfo);
        }

        //for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            //if (submitContexts[deviceIndex] == nullptr)
            //{
            //    continue;
            //}
            //if (deviceIndex > 0)
            //{
            //    break;
            //}

            VK_CHECK(vkQueueSubmit2(
                device.GetCommandQueue(queueFamily, 0)->handle,
                (uint32)submitInfos[queueFamily].size(),
                submitInfos[queueFamily].data(),
                submitContext.completeFence));
        }
    }

    //RenderBackendTextureSRVHandle VulkanRenderBackend::CreateTextureSRV(const RenderBackendTextureSRVDesc* desc, const char* name)
    //{
    //    RenderBackendTextureSRVHandle handle = handleManager.Allocate<RenderBackendTextureSRVHandle>(deviceMask);
    //    for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
    //    {
    //        VulkanDevice& device = devices[deviceIndex];
    //        if ((device.GetDeviceMask() & deviceMask) == 0)
    //        {
    //            continue;
    //        }

    //        uint32 textureIndex = 0;
    //        if (!device.TryGetRenderBackendHandleRepresentation(desc->texture.GetIndex(), &textureIndex))
    //        {
    //            continue;
    //        }

    //        uint32 index = device.CreateTextureSRV(textureIndex, desc, name);
    //        device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
    //    }
    //    return handle;
    //}

    //RenderBackendTextureUAVHandle VulkanRenderBackend::CreateTextureUAV(const RenderBackendTextureUAVDesc* desc, const char* name)
    //{
    //    RenderBackendTextureUAVHandle handle = handleManager.Allocate<RenderBackendTextureUAVHandle>(deviceMask);
    //    for (uint32 deviceIndex = 0; deviceIndex <numDevices; deviceIndex++)
    //    {
    //        VulkanDevice& device = devices[deviceIndex];
    //        if ((device.GetDeviceMask() & deviceMask) == 0)
    //        {
    //            continue;
    //        }

    //        uint32 textureIndex = 0;
    //        if (!device.TryGetRenderBackendHandleRepresentation(desc->texture.GetIndex(), &textureIndex))
    //        {
    //            continue;
    //        }

    //        uint32 index = device.CreateTextureUAV(textureIndex, desc, name);
    //        device.SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
    //    }
    //    return handle;
    //}

    int32 VulkanRenderBackend::GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle)
    {
        uint32 textureIndex = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &textureIndex))
        {
            return -1;
        }
        return device.GetTextureSRVBindlessResourceDescriptorIndex(textureIndex);
    }

    int32 VulkanRenderBackend::GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle, uint32 mipLevel)
    {
        uint32 textureIndex = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &textureIndex))
        {
            return -1;
        }
        return device.GetTextureSRVBindlessResourceDescriptorIndex(textureIndex, mipLevel);
    }

    int32 VulkanRenderBackend::GetTextureUAVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle, uint32 mipLevel)
    {
        uint32 textureIndex = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &textureIndex))
        {
            return -1;
        }
        return device.GetTextureUAVBindlessResourceDescriptorIndex(textureIndex, mipLevel);
    }

    int32 VulkanRenderBackend::GetBufferCBVBindlessResourceDescriptorIndex(RenderBackendBufferHandle handle)
    {
        uint32 bufferIndex = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &bufferIndex))
        {
            return -1;
        }
        return device.GetBufferCBVBindlessResourceDescriptorIndex(bufferIndex);
    }

    int32 VulkanRenderBackend::GetBufferSRVBindlessResourceDescriptorIndex(RenderBackendBufferHandle handle)
    {
        uint32 bufferIndex = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &bufferIndex))
        {
            return -1;
        }
        return device.GetBufferSRVBindlessResourceDescriptorIndex(bufferIndex);
    }

    int32 VulkanRenderBackend::GetBufferUAVBindlessResourceDescriptorIndex(RenderBackendBufferHandle handle)
    {
        uint32 bufferIndex = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &bufferIndex))
        {
            return -1;
        }
        return device.GetBufferUAVBindlessResourceDescriptorIndex(bufferIndex);
    }

    int32 VulkanRenderBackend::GetAccelerationStructureSRVBindlessResourceDescriptorIndex(RenderBackendRayTracingAccelerationStructureHandle handle)
    {
        uint32 index = 0;
        if (!device.TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return -1;
        }
        return device.accelerationStructures[index].descriptorIndex;
    }

    RenderBackendRayTracingPipelineStateHandle VulkanRenderBackend::CreateRayTracingPipelineState(const RenderBackendRayTracingPipelineStateDesc* desc, const char* name)
    {
        RenderBackendRayTracingPipelineStateHandle handle = handleManager.Allocate<RenderBackendRayTracingPipelineStateHandle>();
        {
            uint32 numShaders = (uint32)desc->shaders.size();
            uint32 numShaderGroups = (uint32)desc->shaderGroupDescs.size();

            std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(numShaders);
            std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupCreateInfos(numShaderGroups);

            for (uint32 shaderIndex = 0; shaderIndex < numShaders; shaderIndex++)
            {
                VulkanShader* shader = device.GetShader(desc->shaders[shaderIndex]);
                shaderStageCreateInfos[shaderIndex] = shader->stageInfo;
            }

            uint32 numRayGenerationShaders = 0;
            uint32 numMissShaders = 0;
            uint32 numHitGroups = 0;

            for (uint32 groupIndex = 0; groupIndex < numShaderGroups; groupIndex++)
            {
                switch (desc->shaderGroupDescs[groupIndex].type)
                {
                case RenderBackendRayTracingShaderGroupType::RayGen:
                    shaderGroupCreateInfos[groupIndex] =
                    {
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
                    shaderGroupCreateInfos[groupIndex] =
                    {
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
                    shaderGroupCreateInfos[groupIndex] =
                    {
                        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                        .generalShader = VK_SHADER_UNUSED_KHR,
                        .closestHitShader = desc->shaderGroupDescs[groupIndex].closestHitShader,
                        .anyHitShader = desc->shaderGroupDescs[groupIndex].anyHitShader,
                        .intersectionShader = desc->shaderGroupDescs[groupIndex].intersectionShader,
                    };
                    numHitGroups++;
                    break;
                case RenderBackendRayTracingShaderGroupType::ProceduralHitGroup:
                    std::unreachable();
                default:
                    std::unreachable();
                    break;
                }
            }
            assert(numRayGenerationShaders == 1);

            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties = device.GetRayTracingPipelineProperties();

            VkPipelineLayout pipelineLayout = device.FindOrCreatePipelineLayout(device.bindlessDescriptorManager.pushConstantsSize, RenderBackendPipelineType::RayTracing);

            VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo =
            {
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

            VK_CHECK(vulkanFunctions.vkCreateRayTracingPipelinesKHR(
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

    RenderBackendBufferHandle VulkanRenderBackend::CreateRayTracingShaderBindingTable(const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name)
    {
        RenderBackendBufferHandle handle = handleManager.Allocate<RenderBackendBufferHandle>();
        {
            VulkanRayTracingPipelineState& rayTracingPipelineState = *device.GetRayTracingPipelineState(desc->rayTracingPipelineState);

            uint32 numMissShaders = rayTracingPipelineState.numMissShaders;
            uint32 numHitGroups = rayTracingPipelineState.numHitGroups;
            uint32 numShaderGroups = 1 + numMissShaders + numHitGroups;

            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties = device.GetRayTracingPipelineProperties();

            const uint32 shaderGroupHandleSizeAligned = AlignUp(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
            const uint32 shaderGroupSizeAligned = AlignUp(shaderGroupHandleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment);

            std::vector<uint8> shaderGroupHandles(shaderGroupHandleSizeAligned * numShaderGroups);
            VK_CHECK(vulkanFunctions.vkGetRayTracingShaderGroupHandlesKHR(device.GetHandle(), rayTracingPipelineState.handle, 0, numShaderGroups, shaderGroupHandleSizeAligned * numShaderGroups, shaderGroupHandles.data()));

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

            VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = sbtBuffer.handle,
            };
            VkDeviceAddress sbtBufferAddress = vulkanFunctions.vkGetBufferDeviceAddressKHR(device.GetHandle(), &bufferDeviceAddressInfo);

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
        uint32 index = device.GetRenderBackendHandleRepresentation(handle.GetIndex());
        VulkanTexture& texture = device.textures[index];

        *data = texture.cpuReadbackBuffer->data;
    }
}

namespace Horizon
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

    void RenderBackendDestroyVulkan(RenderBackend* backend)
    {
        VulkanRenderBackend* vulkanBackend = (VulkanRenderBackend*)backend;
        vulkanBackend->Exit();
        delete vulkanBackend;
    }
}