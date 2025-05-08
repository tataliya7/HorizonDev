#pragma once

#include "RenderBackendCommon.h"
#include "RenderBackendConfig.h"
#include "RenderBackendHandles.h"
#include "RenderBackendTextureFormat.h"

namespace Horizon
{
    /**
     * @see https://pcisig.com/membership/member-companies
     */
    enum class RenderBackendGPUVendorID
    {
        Unknown = 0,
        NIVIDIA = 0x10DE,
        AMD     = 0x1002,
        Intel   = 0x8086,
    };

    enum class RenderBackendQueueFamily
    {
        Copy = 0,
        Compute = 1,
        Graphics = 2,
        OpticalFlow = 3,
        Count
    };
    static_assert((uint32)RenderBackendQueueFamily::Graphics == 2, "Graphics queue index should be 2.");
    static_assert((uint32)RenderBackendQueueFamily::Count == RenderBackendQueueFamilyCount);

    struct RenderBackendExtent2D
    {
        uint32 width;
        uint32 height;
    };

    struct RenderBackendExtent3D
    {
        uint32 width;
        uint32 height;
        uint32 depth;
    };

    struct RenderBackendOffset2D
    {
        int32 x;
        int32 y;
    };

    struct RenderBackendOffset3D
    {
        int32 x;
        int32 y;
        int32 z;
    };

    struct RenderBackendShaderBlob
    {
        uint64 size;
        const uint8* data;
    };

    enum class RenderBackendResourceState
    {
        Undefined = 0,
        Present = 1,
        IndirectArgument = 2,
        VertexBuffer = 3,
        IndexBuffer = 4,
        ShaderResource = 5,
        CopySrc = 6,
        DepthStencilReadOnly = 7,
        RenderTarget = 8,
        CopyDst = 9,
        UnorderedAccess = 10,
        DepthStencil = 11,
        RayTracingAccelerationStructureReadOnly = 12,
        RayTracingAccelerationStructure = 13,
    };

    enum class RenderBackendResourceStateFlags
    {
        None = 0,
        Undefined = (1 << 0),
        // Read only
        Present = (1 << 1),
        IndirectArgument = (1 << 2),
        VertexBuffer = (1 << 3),
        IndexBuffer = (1 << 4),
        ShaderResource = (1 << 5),
        CopySrc = (1 << 6),
        DepthStencilReadOnly = (1 << 7),
        // Write only
        RenderTarget = (1 << 8),
        CopyDst = (1 << 9),
        // Read-write
        UnorderedAccess = (1 << 10),
        DepthStencil = (1 << 11),

        ReadOnlyMask = Present | IndirectArgument | VertexBuffer | IndexBuffer | ShaderResource | CopySrc | DepthStencilReadOnly,
        WriteOnlyMask = RenderTarget | CopyDst,
        ReadWriteMask = UnorderedAccess | DepthStencil,
        WritableMask = RenderTarget | CopyDst | UnorderedAccess | DepthStencil,
    };
    HORIZON_OVERLOAD_ENUM_CLASS_OPERATORS(RenderBackendResourceStateFlags);

    enum class RenderBackendBufferCreateFlags
    {
        None = 0,
        // Update
        Upload = (1 << 0),
        Readback = (1 << 1),
        // Usage
        UnorderedAccess = (1 << 2),
        CopySrc = (1 << 3),
        CopyDst = (1 << 4),
        IndirectArguments = (1 << 5),
        ShaderResource = (1 << 6),
        VertexBuffer = (1 << 7),
        IndexBuffer = (1 << 8),
        UniformBuffer = (1 << 9),
        StructuredBuffer = (1 << 10),
        RayTracingAccelerationStructure = (1 << 11),
        ShaderBindingTable = (1 << 12),
        // Memory access
        CreateMapped = (1 << 13),
        CpuOnly = (1 << 14),
        GpuOnly = (1 << 15),
        CpuToGpu = (1 << 16),
        GpuToCpu = (1 << 17),
    };
    HORIZON_OVERLOAD_ENUM_CLASS_OPERATORS(RenderBackendBufferCreateFlags);

    struct RenderBackendBufferSubresourceRange
    {
        static const uint64 WholeSize;
        static const RenderBackendBufferSubresourceRange Whole;

        RenderBackendBufferSubresourceRange(uint64 offset, uint64 size)
            : offset(offset)
            , size(size)
        {

        }

        uint64 offset;
        uint64 size;
    };

    struct RenderBackendBufferDesc
    {
        static RenderBackendBufferDesc Create(uint32 elementSize, uint32 elementCount, RenderBackendBufferCreateFlags flags)
        {
            return RenderBackendBufferDesc(elementSize, elementCount, flags);
        }
        static RenderBackendBufferDesc CreateUpload(uint64 bytes)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::Upload | RenderBackendBufferCreateFlags::CreateMapped;
            return RenderBackendBufferDesc(4, (uint32)(bytes >> 2), flags);
        }
        static RenderBackendBufferDesc CreateUpload(uint64 bytes, RenderBackendBufferCreateFlags flags)
        {
            flags |= RenderBackendBufferCreateFlags::Upload | RenderBackendBufferCreateFlags::CreateMapped;
            return RenderBackendBufferDesc(4, (uint32)(bytes >> 2), flags);
        }
        static RenderBackendBufferDesc CreateReadback(uint64 bytes)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::Readback | RenderBackendBufferCreateFlags::CreateMapped;
            return RenderBackendBufferDesc(4, (uint32)(bytes >> 2), flags);
        }
        static RenderBackendBufferDesc CreateIndirectArguments(uint64 bytes)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::IndirectArguments | RenderBackendBufferCreateFlags::UnorderedAccess | RenderBackendBufferCreateFlags::ShaderResource;
            return RenderBackendBufferDesc(4, (uint32)(bytes >> 2), flags);
        }
        static RenderBackendBufferDesc CreateIndirectArguments(uint32 elementSize, uint32 elementCount)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::IndirectArguments | RenderBackendBufferCreateFlags::UnorderedAccess | RenderBackendBufferCreateFlags::ShaderResource;
            return RenderBackendBufferDesc(elementSize, elementCount, flags);
        }
        static RenderBackendBufferDesc CreateIndex(uint32 indexStride, uint32 indexCount)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::IndexBuffer | RenderBackendBufferCreateFlags::ShaderResource;
            // TODO: Remove this
            flags |= RenderBackendBufferCreateFlags::UnorderedAccess;
            return RenderBackendBufferDesc(indexStride, indexCount, flags);
        }
        static RenderBackendBufferDesc CreateConstant(uint64 bytes)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::UniformBuffer | RenderBackendBufferCreateFlags::CpuToGpu;
            return RenderBackendBufferDesc(4, (uint32)(bytes >> 2), flags);
        }
        static RenderBackendBufferDesc CreateByteAddress(uint64 bytes, bool dynamic = false)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::UnorderedAccess | RenderBackendBufferCreateFlags::ShaderResource;
            if (dynamic)
            {
                flags |= RenderBackendBufferCreateFlags::CpuToGpu;
            }
            return RenderBackendBufferDesc(4, (uint32)(bytes >> 2), flags);
        }
        static RenderBackendBufferDesc CreateStructured(uint32 elementSize, uint32 elementCount)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::UnorderedAccess | RenderBackendBufferCreateFlags::ShaderResource | RenderBackendBufferCreateFlags::StructuredBuffer;
            return RenderBackendBufferDesc(elementSize, elementCount, flags);
        }
        static RenderBackendBufferDesc CreateShaderBindingTable(uint64 bytes)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::ShaderBindingTable | RenderBackendBufferCreateFlags::CpuOnly | RenderBackendBufferCreateFlags::CreateMapped;
            return RenderBackendBufferDesc(4, (uint32)(bytes >> 2), flags);
        }
        static RenderBackendBufferDesc CreateShaderBindingTable(uint64 handleSize, uint32 handleCount)
        {
            RenderBackendBufferCreateFlags flags = RenderBackendBufferCreateFlags::ShaderBindingTable | RenderBackendBufferCreateFlags::CpuOnly | RenderBackendBufferCreateFlags::CreateMapped;
            return RenderBackendBufferDesc(4, (uint32)((handleSize * handleCount) >> 2), flags);
        }
        RenderBackendBufferDesc() = default;
        RenderBackendBufferDesc(uint32 elementSize, uint32 elementCount, RenderBackendBufferCreateFlags flags)
            : elementSize(elementSize)
            , elementCount(elementCount)
            , size(elementSize * elementCount)
            , flags(flags) {}

        bool operator==(const RenderBackendBufferDesc& rhs) const
        {
            return size == rhs.size
                && elementSize == rhs.elementSize
                && elementCount == rhs.elementCount
                && flags == rhs.flags;
        }

        uint64 size;
        uint32 elementSize;
        uint32 elementCount;
        RenderBackendBufferCreateFlags flags;
    };

    enum class RenderBackendTextureType
    {
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        Count
    };

    enum class RenderBackendTextureCreateFlags
    {
        None = 0,
        RenderTarget = (1 << 0),
        DepthStencil = (1 << 1),
        ShaderResource = (1 << 2),
        UnorderedAccess = (1 << 3),
        Present = (1 << 4),
        SRGB = (1 << 5),
        NoTilling = (1 << 6),
        Dynamic = (1 << 7),
        Readback = (1 << 8),
        Sparse = (1 << 9),
    };
    HORIZON_OVERLOAD_ENUM_CLASS_OPERATORS(RenderBackendTextureCreateFlags);

    enum class RenderBackendTextureAddressMode
    {
        Warp = 0,
        Mirror = 1,
        Clamp = 2,
        Border = 3,
    };

    struct RenderBackendTextureClearValue
    {
        bool test = false;

        enum class ClearOp
        {
            None,
            Color,
            DepthStencil
        };
        ClearOp clearOp = ClearOp::None;

        static RenderBackendTextureClearValue CreateColorValueFloat4(float r, float g, float b, float a)
        {
            return RenderBackendTextureClearValue(r, g, b, a);
        }
        static RenderBackendTextureClearValue CreateColorValueUnit4(uint32 r, uint32 g, uint32 b, uint32 a)
        {
            return RenderBackendTextureClearValue(r, g, b, a);
        }
        static RenderBackendTextureClearValue CreateDepthValue(float depth)
        {
            return RenderBackendTextureClearValue(depth, 0);
        }
        static RenderBackendTextureClearValue CreateDepthStencilValue(float depth, uint32 stencil)
        {
            return RenderBackendTextureClearValue(depth, stencil);
        }

        union
        {
            struct ClearColorValue
            {
                union
                {
                    float float32[4];
                    int32 int32[4];
                    uint32 uint32[4];
                };
            } colorValue;
            struct ClearDepthStencilValue
            {
                float depth;
                uint32 stencil;
            } depthStencilValue;
        };

        RenderBackendTextureClearValue()
            : clearOp(ClearOp::None)
        {
            colorValue.float32[0] = 0.0f;
            colorValue.float32[1] = 0.0f;
            colorValue.float32[2] = 0.0f;
            colorValue.float32[3] = 0.0f;

            depthStencilValue.depth = 0.0f;
            depthStencilValue.stencil = 0;
        }

        RenderBackendTextureClearValue(float r, float g, float b, float a)
            : clearOp(ClearOp::Color)
        {
            colorValue.float32[0] = r;
            colorValue.float32[1] = g;
            colorValue.float32[2] = b;
            colorValue.float32[3] = a;
        }

        RenderBackendTextureClearValue(uint32 r, uint32 g, uint32 b, uint32 a)
            : clearOp(ClearOp::Color)
        {
            colorValue.uint32[0] = r;
            colorValue.uint32[1] = g;
            colorValue.uint32[2] = b;
            colorValue.uint32[3] = a;
        }

        RenderBackendTextureClearValue(float depth, uint32 stencil)
            : clearOp(ClearOp::DepthStencil)
        {
            depthStencilValue.depth = depth;
            depthStencilValue.stencil = stencil;
        }

        bool operator==(const RenderBackendTextureClearValue& rhs) const
        {
            if ((clearOp == ClearOp::None) && (rhs.clearOp == ClearOp::None))
            {
                return true;
            }
            else if ((clearOp == ClearOp::Color) && (rhs.clearOp == ClearOp::Color))
            {
                return colorValue.float32[0] == rhs.colorValue.float32[0]
                    && colorValue.float32[1] == rhs.colorValue.float32[1]
                    && colorValue.float32[2] == rhs.colorValue.float32[2]
                    && colorValue.float32[3] == rhs.colorValue.float32[3]
                    && colorValue.uint32[0] == rhs.colorValue.uint32[0]
                    && colorValue.uint32[1] == rhs.colorValue.uint32[1]
                    && colorValue.uint32[2] == rhs.colorValue.uint32[2]
                    && colorValue.uint32[3] == rhs.colorValue.uint32[3];
            }
            else if ((clearOp == ClearOp::DepthStencil) && (rhs.clearOp == ClearOp::DepthStencil))
            {
                return depthStencilValue.depth == rhs.depthStencilValue.depth
                    && depthStencilValue.stencil == rhs.depthStencilValue.stencil;
            }
            return false;
        }

        static const RenderBackendTextureClearValue None;
        static const RenderBackendTextureClearValue Black;
        static const RenderBackendTextureClearValue White;
        static const RenderBackendTextureClearValue DepthOne;
        static const RenderBackendTextureClearValue DepthZero;
    };

    enum class RenderBackendTextureFilter
    {
        MinMagMipPoint,
        MinMagPointMipLinear,
        MinPointMagLinearMipPoint,
        MinPointMagMipLinear,
        MinLinearMagMipPoint,
        MinLinearMagPointMipLinear,
        MinMagLinearMipPoint,
        MinMagMipLinear,
        Anisotropic,
        ComparisonMinMagMipPoint,
        ComparisonMinMagPointMipLinear,
        ComparisonMinPointMagLinearMipPoint,
        ComparisonMinPointMagMipLinear,
        ComparisonMinLinearMagMipPoint,
        ComparisonMinLinearMagPointMipLinear,
        ComparisonMinMagLinearMipPoint,
        ComparisonMinMagMipLinear,
        ComparisonAnisotropic,
        MinimumMinMagMipPoint,
        MinimumMinMagPointMipLinear,
        MinimumMinPointMagLinearMipPoint,
        MinimumMinPointMagMipLinear,
        MinimumMinLinearMagMipPoint,
        MinimumMinLinearMagPointMipLinear,
        MinimumMinMagLinearMipPoint,
        MinimumMinMagMipLinear,
        MinimumAnisotropic,
        MaximumMinMagMipPoint,
        MaximumMinMagPointMipLinear,
        MaximumMinPointMagLinearMipPoint,
        MaximumMinPointMagMipLinear,
        MaximumMinLinearMagMipPoint,
        MaximumMinLinearMagPointMipLinear,
        MaximumMinMagLinearMipPoint,
        MaximumMinMagMipLinear,
        MaximumAnisotropic,
    };

    struct RenderBackendTextureSubresourceRange
    {
        static const uint32 RemainingMipLevels;
        static const uint32 RemainingArrayLayers;
        static const RenderBackendTextureSubresourceRange All;

        RenderBackendTextureSubresourceRange()
            : firstLevel(0), mipLevels(RemainingMipLevels), firstLayer(0), arrayLayers(RemainingArrayLayers) {}

        RenderBackendTextureSubresourceRange(uint32 firstLevel, uint32 mipLevels, uint32 firstLayer, uint32 arrayLayers)
            : firstLevel(firstLevel), mipLevels(mipLevels), firstLayer(firstLayer), arrayLayers(arrayLayers) {}

        bool IsAll() const
        {
            return (firstLevel == 0)
                && (mipLevels == RemainingMipLevels)
                && (firstLayer == 0)
                && (arrayLayers == RemainingArrayLayers);
        }

        bool IsArray() const
        {
            return (arrayLayers > 0) && (arrayLayers != RemainingArrayLayers);
        }

        bool operator==(const RenderBackendTextureSubresourceRange& rhs) const
        {
            return (firstLevel == rhs.firstLevel)
                && (mipLevels == rhs.mipLevels)
                && (firstLayer == rhs.firstLayer)
                && (arrayLayers == rhs.arrayLayers);
        }

        uint32 firstLevel;
        uint32 mipLevels;
        uint32 firstLayer;
        uint32 arrayLayers;
    };

    struct RenderBackendTextureSubresourceLayers
    {
        uint32 mipLevel;
        uint32 firstLayer;
        uint32 arrayLayers;
    };

    enum class RenderBackendPipelineType
    {
        Graphics,
        Compute,
        RayTracing,
    };

    enum class RenderBackendPrimitiveTopology
    {
        PointList     = 0,
        LineList      = 1,
        LineStrip     = 2,
        TriangleList  = 3,
        TriangleStrip = 4,
        TriangleFan   = 5,
    };

    enum class RenderBackendRasterizationCullMode
    {
        None  = 0,
        Front = 1,
        Back  = 2,
    };

    enum class RenderBackendRasterizationFillMode
    {
        Wireframe = 0,
        Solid     = 1,
    };

    enum class RenderBackendStencilOp
    {
        Keep             = 0,
        Zero             = 1,
        Replace          = 2,
        IncreaseAndClamp = 3,
        DecreaseAndClamp = 4,
        Invert           = 5,
        IncreaseAndWrap  = 6,
        DecreaseAndWrap  = 7,
    };

    enum class RenderBackendCompareOp
    {
        Never          = 0,
        Less           = 1,
        Equal          = 2,
        LessOrEqual    = 3,
        Greater        = 4,
        NotEqual       = 5,
        GreaterOrEqual = 6,
        Always         = 7,
    };

    enum class RenderBackendBlendOp
    {
        Add             = 0,
        Subtract        = 1,
        ReverseSubtract = 2,
        Min             = 3,
        Max             = 4,
    };

    enum class RenderBackendBlendFactor
    {
        Zero                        = 0,
        One                         = 1,
        SrcColor                    = 2,
        OneMinusSrcColor            = 3,
        DstColor                    = 4,
        OneMinusDstColor            = 5,
        SrcAlpha                    = 6,
        OneMinusSrcAlpha            = 7,
        DstAlpha                    = 8,
        OneMinusDstAlpha            = 9,
        ConstantBlendFactor         = 10,
        OneMinusConstantBlendFactor = 11,
        SrcAlphaSaturate            = 12,
        Src1Color                   = 13,
        OneMinusSrc1Color           = 14,
        Src1Alpha                   = 15,
        OneMinusSrc1Alpha           = 16,
    };

    enum class RenderBackendColorComponentFlags : uint32
    {
        R = (1 << 0),
        G = (1 << 1),
        B = (1 << 2),
        A = (1 << 3),

        None = 0,
        RGB  = R | G | B,
        RGBA = R | G | B | A,
    };
    HORIZON_OVERLOAD_ENUM_CLASS_OPERATORS(RenderBackendColorComponentFlags);

    struct RenderBackendTextureDesc
    {
        static RenderBackendTextureDesc CreateTexture2D(
            uint32 width,
            uint32 height,
            uint32 mipLevels,
            RenderBackendTextureFormat format,
            RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::None,
            RenderBackendResourceState initalState = RenderBackendResourceState::Undefined)
        {
            RenderBackendTextureCreateFlags flags = RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget;
            return RenderBackendTextureDesc(width, height, 1, mipLevels, 1, 1, RenderBackendTextureType::Texture2D, format, flags, clearValue, initalState);
        }

        static RenderBackendTextureDesc Create1D(
            uint32 width,
            RenderBackendTextureFormat format,
            RenderBackendTextureCreateFlags flags,
            RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::None,
            uint32 mipLevels = 1,
            uint32 samples = 1,
            RenderBackendResourceState initalState = RenderBackendResourceState::Undefined)
        {
            return RenderBackendTextureDesc(width, 1, 1, mipLevels, 1, 1, RenderBackendTextureType::Texture2D, format, flags, clearValue, initalState);
        }

        static RenderBackendTextureDesc Create2D(
            uint32 width,
            uint32 height,
            RenderBackendTextureFormat format,
            RenderBackendTextureCreateFlags flags,
            RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::None,
            uint32 mipLevels = 1,
            uint32 samples = 1,
            RenderBackendResourceState initalState = RenderBackendResourceState::Undefined)
        {
            return RenderBackendTextureDesc(width, height, 1, mipLevels, 1, 1, RenderBackendTextureType::Texture2D, format, flags, clearValue, initalState);
        }

        static RenderBackendTextureDesc Create2DArray(
            uint32 width,
            uint32 height,
            RenderBackendTextureFormat format,
            RenderBackendTextureCreateFlags flags,
            uint32 arraySize,
            RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::None,
            uint32 mipLevels = 1,
            uint32 samples = 1,
            RenderBackendResourceState initalState = RenderBackendResourceState::Undefined)
        {
            return RenderBackendTextureDesc(width, height, 1, mipLevels, arraySize, samples, RenderBackendTextureType::Texture2D, format, flags, clearValue, initalState);
        }

        static RenderBackendTextureDesc Create3D(
            uint32 width,
            uint32 height,
            uint32 depth,
            RenderBackendTextureFormat format,
            RenderBackendTextureCreateFlags flags,
            uint32 mipLevels = 1,
            uint32 samples = 1,
            RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::None,
            RenderBackendResourceState initalState = RenderBackendResourceState::Undefined)
        {
            return RenderBackendTextureDesc(width, height, depth, mipLevels, 1, samples, RenderBackendTextureType::Texture3D, format, flags, clearValue, initalState);
        }

        static RenderBackendTextureDesc CreateCube(
            uint32 sizeInPixels,
            RenderBackendTextureFormat format,
            RenderBackendTextureCreateFlags flags,
            uint32 mipLevels = 1,
            uint32 samples = 1,
            RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::None,
            RenderBackendResourceState initalState = RenderBackendResourceState::Undefined)
        {
            return RenderBackendTextureDesc(sizeInPixels, sizeInPixels, 1, mipLevels, 6, samples, RenderBackendTextureType::TextureCube, format, flags, clearValue, initalState);
        }

        static RenderBackendTextureDesc CreateCubeArray(
            uint32 sizeInPixels,
            RenderBackendTextureFormat format,
            RenderBackendTextureCreateFlags flags,
            uint32 arraySize,
            RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::None,
            uint32 mipLevels = 1,
            uint32 samples = 1,
            RenderBackendResourceState initalState = RenderBackendResourceState::Undefined)
        {
            return RenderBackendTextureDesc(sizeInPixels, sizeInPixels, 1, mipLevels, 6 * arraySize, samples, RenderBackendTextureType::TextureCube, format, flags, clearValue, initalState);
        }

        RenderBackendTextureDesc()
            : width(1)
            , height(1)
            , depth(1)
            , mipLevelCount(1)
            , arrayLayerCount(1)
            , sampleCount(1)
            , type(RenderBackendTextureType::Texture2D)
            , format(RenderBackendTextureFormat::Unknown)
            , flags(RenderBackendTextureCreateFlags::None)
            , clearValue(RenderBackendTextureClearValue::None)
            , initialState(RenderBackendResourceState::Undefined)
        {

        }

        RenderBackendTextureDesc(
            uint32 width,
            uint32 height,
            uint32 depth,
            uint32 mipLevels,
            uint32 arraySize,
            uint32 samples,
            RenderBackendTextureType type,
            RenderBackendTextureFormat format,
            RenderBackendTextureCreateFlags flags,
            RenderBackendTextureClearValue clearValue,
            RenderBackendResourceState initialState)
            : width(width)
            , height(height)
            , depth(depth)
            , mipLevelCount(mipLevels)
            , arrayLayerCount(arraySize)
            , sampleCount(samples)
            , type(type)
            , format(format)
            , flags(flags)
            , clearValue(clearValue)
            , initialState(initialState) {}

        bool operator==(const RenderBackendTextureDesc& rhs) const
        {
            return width == rhs.width
                && height == rhs.height
                && depth == rhs.depth
                && mipLevelCount == rhs.mipLevelCount
                && arrayLayerCount == rhs.arrayLayerCount
                && sampleCount == rhs.sampleCount
                && type == rhs.type
                && format == rhs.format
                && flags == rhs.flags
                && clearValue == rhs.clearValue
                && initialState == rhs.initialState;
        }

        uint32 width;
        uint32 height;
        uint32 depth;
        uint32 mipLevelCount;
        uint32 arrayLayerCount;
        uint32 sampleCount;
        RenderBackendTextureType type;
        RenderBackendTextureFormat format;
        RenderBackendTextureCreateFlags flags;
        RenderBackendTextureClearValue clearValue;
        RenderBackendResourceState initialState;
    };

    struct RenderBackendTextureSRVDesc
    {
        static RenderBackendTextureSRVDesc Create(RenderBackendTextureHandle texture)
        {
            return RenderBackendTextureSRVDesc(texture, 0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers);
        }

        static RenderBackendTextureSRVDesc Create(RenderBackendTextureHandle texture, uint32 baseMipLevel, uint32 mipLevelCount, uint32 baseArrayLayer, uint32 arrayLayerCount)
        {
            return RenderBackendTextureSRVDesc(texture, baseMipLevel, mipLevelCount, baseArrayLayer, arrayLayerCount);
        }

        static RenderBackendTextureSRVDesc CreateForMipLevel(RenderBackendTextureHandle texture, uint32 mipLevel, uint32 baseArrayLayer = 0, uint32 arrayLayerCount = 1)
        {
            return RenderBackendTextureSRVDesc(texture, mipLevel, 1, baseArrayLayer, arrayLayerCount);
        }

        RenderBackendTextureSRVDesc() = default;

        RenderBackendTextureSRVDesc(RenderBackendTextureHandle texture, uint32 baseMipLevel, uint32 mipLevelCount, uint32 baseArrayLayer, uint32 arrayLayerCount)
            : texture(texture) , subresourceRange(baseMipLevel, mipLevelCount, baseArrayLayer, arrayLayerCount) {}

        bool IsValid() const
        {
            return texture.IsValid();
        }

        RenderBackendTextureHandle texture;
        RenderBackendTextureSubresourceRange subresourceRange;
    };

    struct RenderBackendTextureUAVDesc
    {
        static RenderBackendTextureUAVDesc Create(RenderBackendTextureHandle texture, uint32 mipLevel = 0)
        {
            return RenderBackendTextureUAVDesc(texture, mipLevel);
        }
        RenderBackendTextureUAVDesc() = default;
        RenderBackendTextureUAVDesc(RenderBackendTextureHandle texture, uint32 mipLevel)
            : texture(texture), mipLevel(mipLevel) {}

        bool IsValid() const
        {
            return texture.IsValid();
        }

        RenderBackendTextureHandle texture = RenderBackendTextureHandle::Null;
        uint32 mipLevel = 0;
    };

    struct RenderBackendTextureUploadDataDesc
    {
        uint64 totalSize;

        struct SubresourceData
        {
            uint32 level;
            uint32 layer;
            const void* buffer;
            uint32 width;
            uint32 height;
            uint64 size;
            uint64 row_pitch_bytes;
        };

        std::vector<SubresourceData> data;

        RenderBackendTextureUploadDataDesc()
            : totalSize(0)
            , data({})
        {

        }

        void AddSubresourceData(uint32 level, uint32 layer, const void* buffer, uint32 width, uint32 height, uint64 size, uint64 row_pitch_bytes)
        {
            SubresourceData subresourceData =
            {
                .level = level,
                .layer = layer,
                .buffer = buffer,
                .width = width,
                .height = height,
                .size = size,
                .row_pitch_bytes = row_pitch_bytes,
            };

            data.push_back(subresourceData);
            totalSize += size;
        }
    };

    struct RenderBackendSamplerDesc
    {
        static RenderBackendSamplerDesc CreateLinearWarp(float mipLodBias, float minLod, float maxLod, uint32 maxAnisotropy)
        {
            return RenderBackendSamplerDesc(
                RenderBackendTextureFilter::MinMagMipLinear,
                RenderBackendTextureAddressMode::Warp,
                RenderBackendTextureAddressMode::Warp,
                RenderBackendTextureAddressMode::Warp,
                mipLodBias,
                minLod,
                maxLod,
                maxAnisotropy,
                RenderBackendCompareOp::Never);
        }
        static RenderBackendSamplerDesc CreateLinearClamp(float mipLodBias, float minLod, float maxLod, uint32 maxAnisotropy)
        {
            return RenderBackendSamplerDesc(
                RenderBackendTextureFilter::MinMagMipLinear,
                RenderBackendTextureAddressMode::Clamp,
                RenderBackendTextureAddressMode::Clamp,
                RenderBackendTextureAddressMode::Clamp,
                mipLodBias,
                minLod,
                maxLod,
                maxAnisotropy,
                RenderBackendCompareOp::Never);
        }
        static RenderBackendSamplerDesc CreateLinearBorder(float mipLodBias, float minLod, float maxLod, uint32 maxAnisotropy)
        {
            return RenderBackendSamplerDesc(
                RenderBackendTextureFilter::MinMagMipLinear,
                RenderBackendTextureAddressMode::Border,
                RenderBackendTextureAddressMode::Border,
                RenderBackendTextureAddressMode::Border,
                mipLodBias,
                minLod,
                maxLod,
                maxAnisotropy,
                RenderBackendCompareOp::Never);
        }
        static RenderBackendSamplerDesc CreatePointWarp(float mipLodBias, float minLod, float maxLod, uint32 maxAnisotropy)
        {
            return RenderBackendSamplerDesc(
                RenderBackendTextureFilter::MinMagMipPoint,
                RenderBackendTextureAddressMode::Warp,
                RenderBackendTextureAddressMode::Warp,
                RenderBackendTextureAddressMode::Warp,
                mipLodBias,
                minLod,
                maxLod,
                maxAnisotropy,
                RenderBackendCompareOp::Never);
        }
        static RenderBackendSamplerDesc CreatePointClamp(float mipLodBias, float minLod, float maxLod, uint32 maxAnisotropy)
        {
            return RenderBackendSamplerDesc(
                RenderBackendTextureFilter::MinMagMipPoint,
                RenderBackendTextureAddressMode::Clamp,
                RenderBackendTextureAddressMode::Clamp,
                RenderBackendTextureAddressMode::Clamp,
                mipLodBias,
                minLod,
                maxLod,
                maxAnisotropy,
                RenderBackendCompareOp::Never);
        }
        static RenderBackendSamplerDesc CreatePointBorder(float mipLodBias, float minLod, float maxLod, uint32 maxAnisotropy)
        {
            return RenderBackendSamplerDesc(
                RenderBackendTextureFilter::MinMagMipPoint,
                RenderBackendTextureAddressMode::Border,
                RenderBackendTextureAddressMode::Border,
                RenderBackendTextureAddressMode::Border,
                mipLodBias,
                minLod,
                maxLod,
                maxAnisotropy,
                RenderBackendCompareOp::Never);
        }
        static RenderBackendSamplerDesc CreateComparisonLinearClamp(float mipLodBias, float minLod, float maxLod, uint32 maxAnisotropy, RenderBackendCompareOp compareOp)
        {
            return RenderBackendSamplerDesc(
                RenderBackendTextureFilter::ComparisonMinMagMipLinear,
                RenderBackendTextureAddressMode::Clamp,
                RenderBackendTextureAddressMode::Clamp,
                RenderBackendTextureAddressMode::Clamp,
                mipLodBias,
                minLod,
                maxLod,
                maxAnisotropy,
                compareOp);
        }

        RenderBackendSamplerDesc(
            RenderBackendTextureFilter filter,
            RenderBackendTextureAddressMode addressModeU,
            RenderBackendTextureAddressMode addressModeV,
            RenderBackendTextureAddressMode addressModeW,
            float mipLodBias,
            float minLod,
            float maxLod,
            uint32 maxAnisotropy,
            RenderBackendCompareOp compareOp)
            : filter(filter)
            , addressModeU(addressModeU)
            , addressModeV(addressModeV)
            , addressModeW(addressModeW)
            , mipLodBias(mipLodBias)
            , minLod(minLod)
            , maxLod(maxLod)
            , maxAnisotropy(maxAnisotropy)
            , compareOp(compareOp) {}

        RenderBackendTextureFilter filter;
        RenderBackendTextureAddressMode addressModeU;
        RenderBackendTextureAddressMode addressModeV;
        RenderBackendTextureAddressMode addressModeW;
        float mipLodBias;
        float minLod;
        float maxLod;
        uint32 maxAnisotropy;
        RenderBackendCompareOp compareOp;
    };

    enum class RenderBackendShaderStage : uint8
    {
        Compute         = 0,
        Vertex          = 1,
        Pixel           = 2,
        Amplification   = 3,
        Mesh            = 4,
        RayGen          = 5,
        Miss            = 6,
        AnyHit          = 7,
        ClosestHit      = 8,
        Intersection    = 9,
        Count           = 10,
    };
    static_assert(uint8(RenderBackendShaderStage::Count) == RenderBackendShaderStageCount);

    struct RenderBackendShaderDesc
    {
        RenderBackendShaderStage stage;
        const void* code;
        uint64 codeSize;
        const char* entryFunctionName;
    };

    enum class RenderBackendIndexType
    {
        UINT32,
        UINT16,
        UINT8,
    };

    struct RenderBackendRasterizationState
    {
        RenderBackendRasterizationCullMode cullMode = RenderBackendRasterizationCullMode::None;
        RenderBackendRasterizationFillMode fillMode = RenderBackendRasterizationFillMode::Solid;
        bool frontFaceCounterClockwise              = true;
        bool depthClampEnable                       = false;
        float depthBiasClamp                        = 0.0f;
        float depthBiasConstantFactor               = 0.0f;
        float depthBiasSlopeFactor                  = 0.0f;
        float lineWidth                             = 1.0f;
    };

    struct RenderBackendDepthStencilState
    {
        bool depthTestEnable                                   = false;
        bool depthWriteEnable                                  = false;
        RenderBackendCompareOp depthCompareFunction            = RenderBackendCompareOp::Never;
        bool stencilTestEnable                                 = false;
        RenderBackendStencilOp frontFaceStencilPassOp          = RenderBackendStencilOp::Keep;
        RenderBackendStencilOp frontFaceStencilDepthFailOp     = RenderBackendStencilOp::Keep;
        RenderBackendStencilOp frontFaceStencilFailOp          = RenderBackendStencilOp::Keep;
        RenderBackendCompareOp frontFaceStencilCompareFunction = RenderBackendCompareOp::Always;
        RenderBackendStencilOp backFaceStencilPassOp           = RenderBackendStencilOp::Keep;
        RenderBackendStencilOp backFaceStencilDepthFailOp      = RenderBackendStencilOp::Keep;
        RenderBackendStencilOp backFaceStencilFailOp           = RenderBackendStencilOp::Keep;
        RenderBackendCompareOp backFaceStencilCompareFunction  = RenderBackendCompareOp::Always;
        uint8 stencilReadMask                                  = 0xFF;
        uint8 stencilWriteMask                                 = 0xFF;
    };

    struct RenderBackendColorBlendAttachmentState
    {
        static const RenderBackendColorBlendAttachmentState Additive;
        static const RenderBackendColorBlendAttachmentState AdditiveRGB;

        bool blendEnable                             = false;
        RenderBackendBlendFactor srcColorBlendFactor = RenderBackendBlendFactor::One;
        RenderBackendBlendFactor dstColorBlendFactor = RenderBackendBlendFactor::Zero;
        RenderBackendBlendOp colorBlendOp            = RenderBackendBlendOp::Add;
        RenderBackendBlendFactor srcAlphaBlendFactor = RenderBackendBlendFactor::One;
        RenderBackendBlendFactor dstAlphaBlendFactor = RenderBackendBlendFactor::Zero;
        RenderBackendBlendOp alphaBlendOp            = RenderBackendBlendOp::Add;
        RenderBackendColorComponentFlags writeMask   = RenderBackendColorComponentFlags::RGBA;
    };

    struct RenderBackendColorBlendState
    {
        RenderBackendColorBlendAttachmentState targetBlends[RenderBackendMaxRenderTargetCount];
    };

    enum class RenderBackendRenderPassLoadOperation
    {
        Load,
        Clear,
        Discard,
        None
    };

    enum class RenderBackendRenderPassStoreOperation
    {
        Store,
        Discard,
        None
    };

    enum class RenderBackendDepthStencilAccessType
    {
        DepthNoAccess_StencilNoAccess,
        DepthNoAccess_StencilReadOnly,
        DepthNoAccess_StencilWrite,
        DepthReadOnly_StencilNoAccess,
        DepthReadOnly_StencilWrite,
        DepthReadOnly_StencilReadOnly,
        DepthWrite_StencilNoAccess,
        DepthWrite_StencilReadOnly,
        DepthWrite_StencilWrite,
    };

    struct RenderBackendViewport
    {
        float x;
        float y;
        float width;
        float height;
        float minDepth;
        float maxDepth;
        RenderBackendViewport() = default;
        RenderBackendViewport(float width, float height)
            : x(0.0f), y(0.0f), width(width), height(height), minDepth(0.0f), maxDepth(1.0f) {}
        RenderBackendViewport(float x, float y, float width, float height)
            : x(x), y(y), width(width), height(height), minDepth(0.0f), maxDepth(1.0f) {}
        RenderBackendViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
            : x(x), y(y), width(width), height(height), minDepth(minDepth), maxDepth(maxDepth) {}
    };

    struct RenderBackendScissor
    {
        int32 left;
        int32 top;
        uint32 width;
        uint32 height;
        RenderBackendScissor() = default;
        RenderBackendScissor(uint32 width, uint32 height)
            : left(0), top(0), width(width), height(height) {}
        RenderBackendScissor(int32 left, int32 top, uint32 width, uint32 height)
            : left(left), top(top), width(width), height(height) {}
    };

    struct RenderBackendStatistics
    {
        uint64 nonIndexedDraws;
        uint64 indexedDraws;
        uint64 nonIndexedIndirectDraws;
        uint64 indexedIndirectDraws;
        uint64 computeDispatches;
        uint64 computeIndirectDispatches;
        uint64 traceRayDispatches;
        uint64 vertices;
        uint64 pipelines;
        uint64 transitions;
        uint64 renderPasses;
        void Add(const RenderBackendStatistics& other)
        {
            nonIndexedDraws += other.nonIndexedDraws;
            indexedDraws += other.indexedDraws;
            nonIndexedIndirectDraws += other.nonIndexedIndirectDraws;
            indexedIndirectDraws += other.indexedIndirectDraws;
            computeDispatches += other.computeDispatches;
            computeIndirectDispatches += other.computeIndirectDispatches;
            traceRayDispatches += other.traceRayDispatches;
            vertices += other.vertices;
            pipelines += other.pipelines;
            transitions += other.transitions;
            renderPasses += other.renderPasses;
        }
    };

    struct RenderBackendBarrier
    {
        enum class Type
        {
            Global,
            Texture,
            Buffer,
        };
        Type type;
        RenderBackendResourceState stateBefore;
        RenderBackendResourceState stateAfter;
        union
        {
            struct
            {
                RenderBackendTextureHandle texture;
                RenderBackendTextureSubresourceRange textureRange;
            };
            struct
            {
                RenderBackendBufferHandle buffer;
                RenderBackendBufferSubresourceRange bufferRange;
            };
        };
        RenderBackendBarrier(RenderBackendTextureHandle texture, RenderBackendTextureSubresourceRange range, RenderBackendResourceState srcState, RenderBackendResourceState dstState)
            : type(Type::Texture), texture(texture), textureRange(range), stateBefore(srcState), stateAfter(dstState) {}
        RenderBackendBarrier(RenderBackendBufferHandle buffer, RenderBackendBufferSubresourceRange range, RenderBackendResourceState srcState, RenderBackendResourceState dstState)
            : type(Type::Buffer), buffer(buffer), bufferRange(range), stateBefore(srcState), stateAfter(dstState) {}
    };

    enum class RenderBackendQueryType
    {
        Occlusion,
        Timestamp,
    };

    struct RenderBackendTimingQueryHeapDesc
    {
        uint32 maxRegions;
        RenderBackendTimingQueryHeapDesc() = default;
        RenderBackendTimingQueryHeapDesc(uint32 maxRegions) : maxRegions(maxRegions) {}
    };

    struct RenderBackendGraphicsPipelineState
    {
        RenderBackendRasterizationState rasterizationState;
        RenderBackendDepthStencilState depthStencilState;
        RenderBackendColorBlendState colorBlendState;
    };

    struct RenderBackendShaderConstants
    {
        enum class Type : int8
        {
            SamplerState          = 0,
            TextureSRV            = 1,
            TextureUAV            = 2,
            BufferCBV             = 3,
            BufferSRV             = 4,
            BufferUAV             = 5,
            AccelerationStructure = 6,
            Scalar                = 7,
            Count                 = 8
        };

        struct SlotData
        {
            union
            {
                int             descriptorIndex;
                int             scalarTypeInt;
                unsigned int    scalarTypeUint;
                float           scalarTypeFloat;
            };
        };
        static_assert(sizeof(SlotData) == 4);

        void BindSamplerState(uint8 slot, int descriptorIndex)
        {
            //types[slot] = int8(Type::SamplerState);
            data[slot].descriptorIndex = descriptorIndex;
        }

        void BindTextureSRV(uint8 slot, int descriptorIndex)
        {
            //types[slot] = int8(Type::TextureSRV);
            data[slot].descriptorIndex = descriptorIndex;
        }

        void BindTextureUAV(uint8 slot, int descriptorIndex)
        {
            //types[slot] = int8(Type::TextureUAV);
            data[slot].descriptorIndex = descriptorIndex;
        }

        void BindBufferCBV(uint8 slot, int descriptorIndex)
        {
            //types[slot] = int8(Type::BufferCBV);
            data[slot].descriptorIndex = descriptorIndex;
        }

        void BindBufferSRV(uint8 slot, int descriptorIndex)
        {
            //types[slot] = int8(Type::BufferSRV);
            data[slot].descriptorIndex = descriptorIndex;
        }

        void BindBufferUAV(uint8 slot, int descriptorIndex)
        {
            //types[slot] = int8(Type::BufferUAV);
            data[slot].descriptorIndex = descriptorIndex;
        }

        void BindAccelerationStructure(uint8 slot, int descriptorIndex)
        {
            //types[slot] = int8(Type::RayTracingAccelerationStructure);
            data[slot].descriptorIndex = descriptorIndex;
        }

        void BindScalar(uint8 slot, int32 value)
        {
            //types[slot] = int8(Type::Scalar);
            data[slot].scalarTypeInt = value;
        }

        void BindScalar(uint8 slot, uint32 value)
        {
            //types[slot] = int8(Type::Scalar);
            data[slot].scalarTypeUint = value;
        }

        void BindScalar(uint8 slot, float value)
        {
            //types[slot] = int8(Type::Scalar);
            data[slot].scalarTypeFloat = value;
        }

        //int8 types[RenderBackendPushConstantsSlotCount];
        SlotData data[RenderBackendPushConstantsSlotCount];
    };

#if 0
    struct RenderBackendRenderTargetBinding
    {
        RenderBackendTextureViewHandle renderTargetView;
        RenderBackendRenderPassLoadOperation loadOperation;
        RenderBackendRenderPassStoreOperation storeOperation;
    };

    struct RenderBackendDepthStencilBinding
    {
        RenderBackendTextureViewHandle depthStencilView;
        RenderBackendRenderPassLoadOperation depthLoadOperation;
        RenderBackendRenderPassStoreOperation depthStoreOperation;
        RenderBackendRenderPassLoadOperation stencilLoadOperation;
        RenderBackendRenderPassStoreOperation stencilStoreOperation;
        RenderBackendDepthStencilAccessType depthStencilAccessType;
    };
#endif

    struct alignas(64) RenderBackendRenderPassInfo
    {
        struct RenderTargetBinding
        {
            RenderBackendTextureHandle texture;
            uint32 mipLevel;
            RenderBackendRenderPassLoadOperation loadOp;
            RenderBackendRenderPassStoreOperation storeOp;
        };
        struct DepthStencilBinding
        {
            RenderBackendTextureHandle texture;
            uint32 mipLevel;
            RenderBackendRenderPassLoadOperation depthLoadOp;
            RenderBackendRenderPassStoreOperation depthStoreOp;
            RenderBackendRenderPassLoadOperation stencilLoadOp;
            RenderBackendRenderPassStoreOperation stencilStoreOp;
            RenderBackendDepthStencilAccessType depthStencilAccessType;
        };
        Rect renderArea;
        bool allowUAVWrites; //uint32 renderPassFlags;
        RenderTargetBinding renderTargets[RenderBackendMaxRenderTargetCount];
        DepthStencilBinding depthStencil;
    };

    enum class RenderBackendRayTracingAccelerationStructureBuildFlags
    {
        None = 0,
        AllowUpdate = 1 << 0,
        AllowCompaction = 1 << 1,
        PreferFastTrace = 1 << 2,
        PreferFastBuild = 1 << 3,
        MinimizeMemory = 1 << 4,
    };
    HORIZON_OVERLOAD_ENUM_CLASS_OPERATORS(RenderBackendRayTracingAccelerationStructureBuildFlags);

    enum class RenderBackendRayTracingInstanceFlags
    {
        None = 0,
        TriangleFacingCullDisable = 1 << 0,
        TriangleFrontCounterclockwise = 1 << 1,
        ForceOpaque = 1 << 2,
        ForceNoOpaque = 1 << 3,
    };
    HORIZON_OVERLOAD_ENUM_CLASS_OPERATORS(RenderBackendRayTracingInstanceFlags);

    struct RenderBackendRayTracingInstance
    {
        Matrix4x4f transformMatrix;
        uint32 instanceID;
        uint32 instanceMask;
        uint32 instanceContributionToHitGroupIndex;
        RenderBackendRayTracingInstanceFlags flags;
        RenderBackendRayTracingAccelerationStructureHandle blas;
    };

    enum class RenderBackendRayTracingGeometryType
    {
        Triangles = 0,
        AABBs = 1,
    };

    enum class RenderBackendRayTracingGeometryFlags
    {
        None = 0,
        Opaque = 1 << 0,
        NoDuplicateAnyHitInvocation = 1 << 1,
    };
    HORIZON_OVERLOAD_ENUM_CLASS_OPERATORS(RenderBackendRayTracingGeometryFlags);

    struct RenderBackendRayTracingGeometryTriangleDesc
    {
        uint32 indexCount;
        uint32 vertexCount;
        uint32 vertexStride;
        RenderBackendBufferHandle vertexBuffer;
        uint32 vertexOffset;
        RenderBackendBufferHandle indexBuffer;
        uint32 indexOffset;
        RenderBackendBufferHandle transformBuffer;
        uint32 transformOffset;
    };

    struct RenderBackendRayTracingGeometryAABBDesc
    {
        RenderBackendBufferHandle buffer;
        uint32 offset;
    };

    struct RenderBackendRayTracingGeometryDesc
    {
        RenderBackendRayTracingGeometryType type;
        RenderBackendRayTracingGeometryFlags flags;
        //union
        //{
        RenderBackendRayTracingGeometryTriangleDesc triangleDesc;
        RenderBackendRayTracingGeometryAABBDesc aabbDesc;
        //};
    };

    struct RenderBackendRayTracingBottomLevelAccelerationStructureDesc
    {
        RenderBackendRayTracingAccelerationStructureBuildFlags buildFlags;
        uint32 geometryCount;
        RenderBackendRayTracingGeometryDesc* geometryDescs;
    };

    struct RenderBackendRayTracingTopLevelAccelerationStructureDesc
    {
        RenderBackendRayTracingAccelerationStructureBuildFlags buildFlags;
        RenderBackendRayTracingGeometryFlags geometryFlags;
        uint32 instanceCount;
        RenderBackendRayTracingInstance* instances;
    };

    struct RenderBackendRayTracingAccelerationStructureRange
    {
        uint32 numPrimitives;
        uint32 primitiveOffset;
        uint32 firstVertex;
        uint32 transformOffset;
    };

    struct RenderBackendShaderBindingTable
    {
        RenderBackendBufferHandle buffer;
        uint64 offset;
        uint64 size;
        uint64 stride;
    };

    enum class RenderBackendRayTracingShaderGroupType
    {
        RayGen,
        Miss,
        TrianglesHitGroup,
        ProceduralHitGroup,
    };

    struct RenderBackendRayTracingShaderDesc
    {
        RenderBackendShaderStage stage;
        std::string entry;
        RenderBackendShaderBlob code;
    };

    struct RenderBackendRayTracingShaderGroupDesc
    {
        static const uint32 ShaderUnused = ~0u;

        static RenderBackendRayTracingShaderGroupDesc CreateRayGen(uint32 rayGenerationShader)
        {
            return RenderBackendRayTracingShaderGroupDesc(
                RenderBackendRayTracingShaderGroupType::RayGen,
                rayGenerationShader,
                ShaderUnused,
                ShaderUnused,
                ShaderUnused,
                ShaderUnused);
        }

        static RenderBackendRayTracingShaderGroupDesc CreateMiss(uint32 missShader)
        {
            return RenderBackendRayTracingShaderGroupDesc(
                RenderBackendRayTracingShaderGroupType::Miss,
                ShaderUnused,
                missShader,
                ShaderUnused,
                ShaderUnused,
                ShaderUnused);
        }

        static RenderBackendRayTracingShaderGroupDesc CreateTrianglesHitGroup(uint32 closestHitShader, uint32 anyHitShader, uint32 intersectionShader)
        {
            return RenderBackendRayTracingShaderGroupDesc(
                RenderBackendRayTracingShaderGroupType::TrianglesHitGroup,
                ShaderUnused,
                ShaderUnused,
                closestHitShader,
                anyHitShader,
                intersectionShader);
        }

        RenderBackendRayTracingShaderGroupDesc()
            : type(RenderBackendRayTracingShaderGroupType::RayGen)
            , rayGenerationShader(ShaderUnused)
            , missShader(ShaderUnused)
            , closestHitShader(ShaderUnused)
            , anyHitShader(ShaderUnused)
            , intersectionShader(ShaderUnused)
        {

        }

        RenderBackendRayTracingShaderGroupDesc(
            RenderBackendRayTracingShaderGroupType type,
            uint32 rayGenerationShader,
            uint32 missShader,
            uint32 closestHitShader,
            uint32 anyHitShader,
            uint32 intersectionShader)
            : type(type)
            , rayGenerationShader(rayGenerationShader)
            , missShader(missShader)
            , closestHitShader(closestHitShader)
            , anyHitShader(anyHitShader)
            , intersectionShader(intersectionShader)
        {

        }

        RenderBackendRayTracingShaderGroupType type;
        uint32 rayGenerationShader;
        uint32 missShader;
        uint32 closestHitShader;
        uint32 anyHitShader;
        uint32 intersectionShader;
    };

    struct RenderBackendRayTracingPipelineStateDesc
    {
        uint32 maxRayRecursionDepth;
        std::vector<RenderBackendShaderHandle> shaders;
        std::vector<RenderBackendRayTracingShaderGroupDesc> shaderGroupDescs;
    };

    struct RenderBackendRayTracingShaderBindingTableDesc
    {
        RenderBackendRayTracingPipelineStateHandle rayTracingPipelineState;
        uint32 shaderRecordCount;
        std::vector<uint32> shaderGroupIndices;
        std::vector<uint32> shaderRecordSizes;
        std::vector<void*>  shaderRecordValues;
    };

    enum class RenderBackendSwapChainPresentMode
    {
        Immediate,
        Maibox,
        FIFO,
        FIFORelaxed,
    };

    struct RenderBackendSwapChainDesc
    {
        uint32 width;
        uint32 height;
        uint64 windowHandle;
        uint32 numBuffers;
        bool vsync;
        bool fullscreen;
        RenderBackendTextureFormat format;
        RenderBackendSwapChainPresentMode presentMode;
    };

    struct RenderBackendDrawIndirectArguments
    {
        uint32 numVertices;
        uint32 numInstances;
        uint32 firstVertex;
        uint32 firstInstance;
    };

    struct RenderBackendDrawIndexedIndirectArguments
    {
        uint32 numIndices;
        uint32 numInstances;
        uint32 firstIndex;
        int32 vertexOffset;
        uint32 firstInstance;
    };

    struct RenderBackendDispatchIndirectArguments
    {
        uint32 threadGroupCountX;
        uint32 threadGroupCountY;
        uint32 threadGroupCountZ;
    };

    struct RenderBackendDispatchRaysIndirectArguments
    {
        uint32 width;
        uint32 height;
        uint32 depth;
    };

    struct RenderBackendDispatchMeshIndirectArguments
    {
        uint32 threadGroupCountX;
        uint32 threadGroupCountY;
        uint32 threadGroupCountZ;
    };

    struct RenderBackendDevice
    {
        void* device;
        void* physicalDevice;
    };

    struct RenderBackendTextureResource
    {
        /** VkImage or ID3D12Resource */
        void* texture;

        /** vkImageCreateInfo or nullptr */
        void* info;

        /** vkDeviceMemory or nullptr */
        void* memory;

        /** VkImageView or nullptr */
        void* view;

        /** Width in pixels */
        uint32 width;

        /** Height in pixels */
        uint32 height;

        /** Number of mip-map levels */
        uint32 mipLevels;

        /** Number of arrays */
        uint32 arrayLayers;

        /** Native format */
        uint32 format;

        /** VkImageLayout or D3D12_RESOURCE_STATES */
        uint32 state;

        /** VkImageCreateFlags */
        uint32 flags;

        /** VkImageUsageFlags */
        uint32 usage;
    };

    typedef bool(*RenderBackendDispatchSuperSamplingCallback)(
        void* commandList,
        void* temporalSuperSamplingInterface,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure);
}