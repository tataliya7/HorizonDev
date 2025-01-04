#pragma once

#include "Direct3D12RenderBackendCommon.h"

#include <d3dx12/d3dx12.h>

namespace Horizon
{

#define D3D12_CHECK(function) { HRESULT hr = function; if (FAILED(hr)) { VerifyD3D12Result(hr, #function, __FILE__, __LINE__); } }

#define SIZEOF_32BIT(x) ((sizeof(x) - 1) / sizeof(uint32) + 1)

    namespace D3D12Utils
    {
        inline std::wstring Widen(const std::string& input)
        {
            std::wstring result = {};
            if (input.length() > 0)
            {
                int length = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), int(input.size()), NULL, 0);
                if (length > 0)
                {
                    result.resize(length);
                    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), int(input.size()), result.data(), int(result.size()));
                }
            }
            return result;
        }

        inline std::string Narrow(const std::wstring& input)
        {
            std::string result = {};
            if (input.length() > 0)
            {
                int length = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), int(input.size()), NULL, 0, NULL, NULL);
                if (length > 0)
                {
                    result.resize(length);
                    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), int(input.size()), result.data(), int(result.size()), NULL, NULL);
                }
            }
            return result;
        }
    }

    static inline uint32 AlignUp(uint32 size, uint32 alignment)
    {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    static inline void VerifyD3D12Result(HRESULT result, const char* function, const char* filename, uint32 line)
    {
        LogError(GLogger, std::format("D3D12 function returns a runtime error. Code: 0x{:X}. Function: {}. File: {}. Line: {}.", (uint32)result, function, filename, line));
    }

    static inline uint32 GetNumSubresources(ID3D12Device* device, uint32 mipLevels, uint32 arrayLayers, DXGI_FORMAT format)
    {
        return mipLevels * arrayLayers * D3D12GetFormatPlaneCount(device, format);
    }

    static inline const WCHAR* GetD3DFeatureLevelWCHAR(D3D_FEATURE_LEVEL featureLevel)
    {
        switch (featureLevel)
        {
        case D3D_FEATURE_LEVEL_11_0:
            return TEXT("D3D_FEATURE_LEVEL_11_0");
        case D3D_FEATURE_LEVEL_11_1:
            return TEXT("D3D_FEATURE_LEVEL_11_1");
        case D3D_FEATURE_LEVEL_12_0:
            return TEXT("D3D_FEATURE_LEVEL_12_0");
        case D3D_FEATURE_LEVEL_12_1:
            return TEXT("D3D_FEATURE_LEVEL_12_1");
        case D3D_FEATURE_LEVEL_12_2:
            return TEXT("D3D_FEATURE_LEVEL_12_2");
        default:
            std::unreachable();
            break;
        }
        return TEXT("D3D_FEATURE_LEVEL_INVALID");
    }

    static inline const char* GetD3D12RenderPassesTierName(D3D12_RENDER_PASS_TIER tier)
    {
        switch (tier)
        {
        case D3D12_RENDER_PASS_TIER_0:
            return "D3D12_RENDER_PASS_TIER_0";
        case D3D12_RENDER_PASS_TIER_1:
            return "D3D12_RENDER_PASS_TIER_1";
        case D3D12_RENDER_PASS_TIER_2:
            return "D3D12_RENDER_PASS_TIER_2";
        default:
            std::unreachable();
            break;
        }
        return "Unknown";
    }

    static inline const WCHAR* GetD3DShaderModelWCHAR(D3D_SHADER_MODEL shaderModel)
    {
        switch (shaderModel)
        {
        case D3D_SHADER_MODEL_6_8:
            return TEXT("D3D_SHADER_MODEL_6_8");
        case D3D_SHADER_MODEL_6_7:
            return TEXT("D3D_SHADER_MODEL_6_7");
        case D3D_SHADER_MODEL_6_6:
            return TEXT("D3D_SHADER_MODEL_6_6");
        case D3D_SHADER_MODEL_6_5:
            return TEXT("D3D_SHADER_MODEL_6_5");
        case D3D_SHADER_MODEL_6_4:
            return TEXT("D3D_SHADER_MODEL_6_4");
        case D3D_SHADER_MODEL_6_3:
            return TEXT("D3D_SHADER_MODEL_6_3");
        case D3D_SHADER_MODEL_6_2:
            return TEXT("D3D_SHADER_MODEL_6_2");
        case D3D_SHADER_MODEL_6_1:
            return TEXT("D3D_SHADER_MODEL_6_1");
        case D3D_SHADER_MODEL_6_0:
            return TEXT("D3D_SHADER_MODEL_6_0");
        default:
            std::unreachable();
            break;
        }
        return TEXT("D3D_SHADER_MODEL_INVALID");
    }

    static inline D3D12_FILTER ConvertToD3D12Filter(RenderBackendTextureFilter filter)
    {
        switch (filter)
        {
        case RenderBackendTextureFilter::MinMagMipPoint:
            return D3D12_FILTER_MIN_MAG_MIP_POINT;
        case RenderBackendTextureFilter::MinMagPointMipLinear:
            return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        case RenderBackendTextureFilter::MinPointMagLinearMipPoint:
            return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case RenderBackendTextureFilter::MinPointMagMipLinear:
            return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        case RenderBackendTextureFilter::MinLinearMagMipPoint:
            return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        case RenderBackendTextureFilter::MinLinearMagPointMipLinear:
            return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case RenderBackendTextureFilter::MinMagLinearMipPoint:
            return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        case RenderBackendTextureFilter::MinMagMipLinear:
            return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        case RenderBackendTextureFilter::Anisotropic:
            return D3D12_FILTER_ANISOTROPIC;
        case RenderBackendTextureFilter::ComparisonMinMagMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        case RenderBackendTextureFilter::ComparisonMinMagPointMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
        case RenderBackendTextureFilter::ComparisonMinPointMagLinearMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case RenderBackendTextureFilter::ComparisonMinPointMagMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
        case RenderBackendTextureFilter::ComparisonMinLinearMagMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
        case RenderBackendTextureFilter::ComparisonMinLinearMagPointMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case RenderBackendTextureFilter::ComparisonMinMagLinearMipPoint:
            return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        case RenderBackendTextureFilter::ComparisonMinMagMipLinear:
            return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        case RenderBackendTextureFilter::ComparisonAnisotropic:
            return D3D12_FILTER_COMPARISON_ANISOTROPIC;
        case RenderBackendTextureFilter::MinimumMinMagMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
        case RenderBackendTextureFilter::MinimumMinMagPointMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
        case RenderBackendTextureFilter::MinimumMinPointMagLinearMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case RenderBackendTextureFilter::MinimumMinPointMagMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
        case RenderBackendTextureFilter::MinimumMinLinearMagMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
        case RenderBackendTextureFilter::MinimumMinLinearMagPointMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case RenderBackendTextureFilter::MinimumMinMagLinearMipPoint:
            return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
        case RenderBackendTextureFilter::MinimumMinMagMipLinear:
            return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
        case RenderBackendTextureFilter::MinimumAnisotropic:
            return D3D12_FILTER_MINIMUM_ANISOTROPIC;
        case RenderBackendTextureFilter::MaximumMinMagMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
        case RenderBackendTextureFilter::MaximumMinMagPointMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
        case RenderBackendTextureFilter::MaximumMinPointMagLinearMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
        case RenderBackendTextureFilter::MaximumMinPointMagMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
        case RenderBackendTextureFilter::MaximumMinLinearMagMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
        case RenderBackendTextureFilter::MaximumMinLinearMagPointMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        case RenderBackendTextureFilter::MaximumMinMagLinearMipPoint:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
        case RenderBackendTextureFilter::MaximumMinMagMipLinear:
            return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
        case RenderBackendTextureFilter::MaximumAnisotropic:
            return D3D12_FILTER_MAXIMUM_ANISOTROPIC;
        default:
            std::unreachable();
            return D3D12_FILTER_MIN_MAG_MIP_POINT;
        }
    }

    static inline D3D12_TEXTURE_ADDRESS_MODE ConvertToD3D12TextureAddressMode(RenderBackendTextureAddressMode mode)
    {
        switch (mode)
        {
        case RenderBackendTextureAddressMode::Warp:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case RenderBackendTextureAddressMode::Mirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case RenderBackendTextureAddressMode::Clamp:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case RenderBackendTextureAddressMode::Border:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        default:
            std::unreachable();
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    }

    static inline D3D12_COMPARISON_FUNC ConvertToD3D12ComparisonFunc(RenderBackendCompareOp compareOp)
    {
        switch (compareOp)
        {
        case RenderBackendCompareOp::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case RenderBackendCompareOp::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case RenderBackendCompareOp::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case RenderBackendCompareOp::LessOrEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case RenderBackendCompareOp::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case RenderBackendCompareOp::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case RenderBackendCompareOp::GreaterOrEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case RenderBackendCompareOp::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
        default:
            std::unreachable();
            return D3D12_COMPARISON_FUNC_NEVER;
        }
    }

    static inline D3D12_RESOURCE_DIMENSION GetD3D12ResourceDimension(RenderBackendTextureType type)
    {
        switch (type)
        {
        case RenderBackendTextureType::Texture1D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        case RenderBackendTextureType::Texture2D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        case RenderBackendTextureType::TextureCube:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        case RenderBackendTextureType::Texture3D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        default:
            std::unreachable();
            return D3D12_RESOURCE_DIMENSION_UNKNOWN;
        }
    }

    static inline D3D12_RESOURCE_FLAGS GetD3D12ResourceFlags(RenderBackendBufferCreateFlags flags)
    {
        D3D12_RESOURCE_FLAGS result = D3D12_RESOURCE_FLAG_NONE;
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::UnorderedAccess))
        {
            result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        if (!EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::ShaderResource))
        {
            result |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::RayTracingAccelerationStructure))
        {
            result |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
        }
        return result;
    }

    static inline D3D12_RESOURCE_FLAGS GetD3D12ResourceFlags(RenderBackendTextureCreateFlags flags)
    {
        D3D12_RESOURCE_FLAGS result = D3D12_RESOURCE_FLAG_NONE;
        if (EnumClassHasFlags(flags, RenderBackendTextureCreateFlags::UnorderedAccess))
        {
            result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        if (!EnumClassHasFlags(flags, RenderBackendTextureCreateFlags::ShaderResource))
        {
            result |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
        if (EnumClassHasFlags(flags, RenderBackendTextureCreateFlags::RenderTarget))
        {
            result |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if (EnumClassHasFlags(flags, RenderBackendTextureCreateFlags::DepthStencil))
        {
            result |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        return result;
    }

    static inline D3D12_HEAP_TYPE GetD3D12HeapType(RenderBackendBufferCreateFlags flags)
    {
        D3D12_HEAP_TYPE type = D3D12_HEAP_TYPE_DEFAULT;
        if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::Upload))
        {
            type = D3D12_HEAP_TYPE_UPLOAD;
        }
        else if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::Readback))
        {
            type = D3D12_HEAP_TYPE_READBACK;
        }
        else if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::CpuOnly))
        {
            type = D3D12_HEAP_TYPE_UPLOAD;
        }
        // TODO: investigate this
        //else if (EnumClassHasFlags(flags, RenderBackendBufferCreateFlags::CpuToGpu))
        //{
        //    type = D3D12_HEAP_TYPE_GPU_UPLOAD;
        //}
        return type;
    }

    // TODO: reorder
    static inline D3D12_RESOURCE_STATES ConvertToD3D12ResourceState(RenderBackendResourceState state)
    {
        switch (state)
        {
        case RenderBackendResourceState::Undefined:
            return D3D12_RESOURCE_STATE_COMMON;
        case RenderBackendResourceState::Present:
            return D3D12_RESOURCE_STATE_PRESENT;
        case RenderBackendResourceState::IndirectArgument:
            return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case RenderBackendResourceState::VertexBuffer:
            return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case RenderBackendResourceState::IndexBuffer:
            return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case RenderBackendResourceState::ShaderResource:
            return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        case RenderBackendResourceState::CopySrc:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case RenderBackendResourceState::DepthStencilReadOnly:
            return D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        case RenderBackendResourceState::DepthStencil:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE; // TODO
        case RenderBackendResourceState::RenderTarget:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case RenderBackendResourceState::CopyDst:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case RenderBackendResourceState::UnorderedAccess:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        default:
            std::unreachable();
            return D3D12_RESOURCE_STATE_COMMON;
        }
    }

    static inline D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE ConvertToD3D12RenderPassBeginningAccessType(RenderBackendRenderPassBeginningAccessType accessType)
    {
        switch (accessType)
        {
        case RenderBackendRenderPassBeginningAccessType::Discard:
            return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        case RenderBackendRenderPassBeginningAccessType::Preserve:
            return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
        case RenderBackendRenderPassBeginningAccessType::Clear:
            return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        case RenderBackendRenderPassBeginningAccessType::NoAccess:
            return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
        default:
            std::unreachable();
            return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        }
    }

    static inline D3D12_RENDER_PASS_ENDING_ACCESS_TYPE ConvertToD3D12RenderPassEndingAccessType(RenderBackendRenderPassEndingAccessType accessType)
    {
        switch (accessType)
        {
        case RenderBackendRenderPassEndingAccessType::Discard:
            return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
        case RenderBackendRenderPassEndingAccessType::Preserve:
            return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        case RenderBackendRenderPassEndingAccessType::NoAccess:
            return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
        default:
            std::unreachable();
            return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
        }
    }

    static inline DXGI_FORMAT ConvertToDXGIFormat(RenderBackendTextureFormat format)
    {
        switch (format)
        {
        case RenderBackendTextureFormat::Unknown:
            return DXGI_FORMAT_UNKNOWN;
        case RenderBackendTextureFormat::R8Unorm:
            return DXGI_FORMAT_R8_UNORM;
        case RenderBackendTextureFormat::R8Snorm:
            return DXGI_FORMAT_R8_SNORM;
        case RenderBackendTextureFormat::R16Unorm:
            return DXGI_FORMAT_R16_UNORM;
        case RenderBackendTextureFormat::R16Snorm:
            return DXGI_FORMAT_R16_SNORM;
        case RenderBackendTextureFormat::R8G8Unorm:
            return DXGI_FORMAT_R8G8_UNORM;
        case RenderBackendTextureFormat::R8G8Snorm:
            return DXGI_FORMAT_R8G8_SNORM;
        case RenderBackendTextureFormat::R16G16Unorm:
            return DXGI_FORMAT_R16G16_UNORM;
        case RenderBackendTextureFormat::R16G16Snorm:
            return DXGI_FORMAT_R16G16_SNORM;
        //case RenderBackendTextureFormat::RGB16Unorm:
        //  return DXGI_FORMAT_R16G16B16_UNORM;
        //case RenderBackendTextureFormat::RGB16Snorm:
        //  return DXGI_FORMAT_R16G16B16_SNORM;
        case RenderBackendTextureFormat::R8G8B8A8Unorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case RenderBackendTextureFormat::R8G8B8A8UnormSrgb:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case RenderBackendTextureFormat::R8G8B8A8Snorm:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case RenderBackendTextureFormat::R16G16B16A16Unorm:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case RenderBackendTextureFormat::R16Float:
            return DXGI_FORMAT_R16_FLOAT;
        case RenderBackendTextureFormat::R16G16Float:
            return DXGI_FORMAT_R16G16_FLOAT;
        //case RenderBackendTextureFormat::RGB16Float:
        //    return DXGI_FORMAT_R16G16B16_FLOAT;
        case RenderBackendTextureFormat::R16G16B16A16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case RenderBackendTextureFormat::R32Float:
            return DXGI_FORMAT_R32_FLOAT;
        case RenderBackendTextureFormat::R32G32Float:
            return DXGI_FORMAT_R32G32_FLOAT;
        case RenderBackendTextureFormat::R32G32B32Float:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case RenderBackendTextureFormat::R32G32B32A32Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case RenderBackendTextureFormat::R8Int:
            return DXGI_FORMAT_R8_SINT;
        case RenderBackendTextureFormat::R8Uint:
            return DXGI_FORMAT_R8_UINT;
        case RenderBackendTextureFormat::R16Int:
            return DXGI_FORMAT_R16_SINT;
        case RenderBackendTextureFormat::R16Uint:
            return DXGI_FORMAT_R16_UINT;
        case RenderBackendTextureFormat::R32Int:
            return DXGI_FORMAT_R32_SINT;
        case RenderBackendTextureFormat::R32Uint:
            return DXGI_FORMAT_R32_UINT;
        case RenderBackendTextureFormat::R8G8Int:
            return DXGI_FORMAT_R8G8_SINT;
        case RenderBackendTextureFormat::R8G8Uint:
            return DXGI_FORMAT_R8G8_UINT;
        case RenderBackendTextureFormat::R16G16Int:
            return DXGI_FORMAT_R16G16_SINT;
        case RenderBackendTextureFormat::R16G16Uint:
            return DXGI_FORMAT_R16G16_UINT;
        case RenderBackendTextureFormat::R32G32Int:
            return DXGI_FORMAT_R32G32_SINT;
        case RenderBackendTextureFormat::R32G32Uint:
            return DXGI_FORMAT_R32G32_UINT;
        //case RenderBackendTextureFormat::RGB16Int:
        //    return DXGI_FORMAT_R16G16B16_SINT;
        //case RenderBackendTextureFormat::RGB16Uint:
        //    return DXGI_FORMAT_R16G16B16_UINT;
        case RenderBackendTextureFormat::R32G32B32Int:
            return DXGI_FORMAT_R32G32B32_SINT;
        case RenderBackendTextureFormat::R32G32B32Uint:
            return DXGI_FORMAT_R32G32B32_UINT;
        case RenderBackendTextureFormat::R8G8B8A8Int:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case RenderBackendTextureFormat::R8G8B8A8Uint:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case RenderBackendTextureFormat::R16G16B16A16Int:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case RenderBackendTextureFormat::R16G16B16A16Uint:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case RenderBackendTextureFormat::R32G32B32A32Int:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case RenderBackendTextureFormat::R32G32B32A32Uint:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case RenderBackendTextureFormat::R11G11B10Float:
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case RenderBackendTextureFormat::B8G8R8A8Unorm:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case RenderBackendTextureFormat::B8G8R8A8UnormSrgb:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case RenderBackendTextureFormat::D32Float:
            return DXGI_FORMAT_D32_FLOAT;
        case RenderBackendTextureFormat::D32FloatS8Uint:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        case RenderBackendTextureFormat::D16Unorm:
            return DXGI_FORMAT_D16_UNORM;
        case RenderBackendTextureFormat::D24UnormS8Uint:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case RenderBackendTextureFormat::R10G10B10A2Unorm:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case RenderBackendTextureFormat::BC1Unorm:
            return DXGI_FORMAT_BC1_UNORM;
        case RenderBackendTextureFormat::BC1UnormSrgb:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case RenderBackendTextureFormat::BC2Unorm:
            return DXGI_FORMAT_BC2_UNORM;
        case RenderBackendTextureFormat::BC2UnormSrgb:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case RenderBackendTextureFormat::BC3Unorm:
            return DXGI_FORMAT_BC3_UNORM;
        case RenderBackendTextureFormat::BC3UnormSrgb:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case RenderBackendTextureFormat::BC4Unorm:
            return DXGI_FORMAT_BC4_UNORM;
        case RenderBackendTextureFormat::BC4Snorm:
            return DXGI_FORMAT_BC4_SNORM;
        case RenderBackendTextureFormat::BC5Unorm:
            return DXGI_FORMAT_BC5_UNORM;
        case RenderBackendTextureFormat::BC5Snorm:
            return DXGI_FORMAT_BC5_SNORM;
        case RenderBackendTextureFormat::BC6HUF:
            return DXGI_FORMAT_BC6H_UF16;
        case RenderBackendTextureFormat::BC6HSF:
            return DXGI_FORMAT_BC6H_SF16;
        case RenderBackendTextureFormat::BC7Unorm:
            return DXGI_FORMAT_BC7_UNORM;
        case RenderBackendTextureFormat::BC7UnormSrgb:
            return DXGI_FORMAT_BC7_UNORM_SRGB;
        default:
            std::unreachable();
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    static inline D3D_PRIMITIVE_TOPOLOGY ConvertToD3DPrimitiveTopology(RenderBackendPrimitiveTopology topology)
    {
        switch (topology)
        {
        case RenderBackendPrimitiveTopology::PointList:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case RenderBackendPrimitiveTopology::LineList:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case RenderBackendPrimitiveTopology::LineStrip:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case RenderBackendPrimitiveTopology::TriangleList:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case RenderBackendPrimitiveTopology::TriangleStrip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case RenderBackendPrimitiveTopology::TriangleFan:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN;
        default:
            std::unreachable();
            return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        }
    }

    static inline D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertToD3D12PrimitiveTopologyType(RenderBackendPrimitiveTopology topology)
    {
        switch (topology)
        {
        case RenderBackendPrimitiveTopology::PointList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case RenderBackendPrimitiveTopology::LineList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case RenderBackendPrimitiveTopology::LineStrip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case RenderBackendPrimitiveTopology::TriangleList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case RenderBackendPrimitiveTopology::TriangleStrip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case RenderBackendPrimitiveTopology::TriangleFan:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        default:
            std::unreachable();
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        }
    }

    static inline D3D12_BLEND ConvertToD3D12Blend(RenderBackendBlendFactor blend)
    {
        switch (blend)
        {
        case RenderBackendBlendFactor::Zero:
            return D3D12_BLEND_ZERO;
        case RenderBackendBlendFactor::One:
            return D3D12_BLEND_ONE;
        case RenderBackendBlendFactor::SrcColor:
            return D3D12_BLEND_SRC_COLOR;
        case RenderBackendBlendFactor::OneMinusSrcColor:
            return D3D12_BLEND_INV_SRC_COLOR;
        case RenderBackendBlendFactor::DstColor:
            return D3D12_BLEND_DEST_COLOR;
        case RenderBackendBlendFactor::OneMinusDstColor:
            return D3D12_BLEND_INV_DEST_COLOR;
        case RenderBackendBlendFactor::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case RenderBackendBlendFactor::OneMinusSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case RenderBackendBlendFactor::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
        case RenderBackendBlendFactor::OneMinusDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
        case RenderBackendBlendFactor::ConstantBlendFactor:
            return D3D12_BLEND_BLEND_FACTOR;
        case RenderBackendBlendFactor::OneMinusConstantBlendFactor:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case RenderBackendBlendFactor::SrcAlphaSaturate:
            return D3D12_BLEND_SRC_ALPHA_SAT;
        case RenderBackendBlendFactor::Src1Color:
            return D3D12_BLEND_SRC1_COLOR;
        case RenderBackendBlendFactor::OneMinusSrc1Color:
            return D3D12_BLEND_INV_SRC1_COLOR;
        case RenderBackendBlendFactor::Src1Alpha:
            return D3D12_BLEND_SRC1_ALPHA;
        case RenderBackendBlendFactor::OneMinusSrc1Alpha:
            return D3D12_BLEND_INV_SRC1_ALPHA;
        default:
            std::unreachable();
            return D3D12_BLEND_ZERO;
        }
    }

    static inline D3D12_BLEND_OP ConvertToD3D12BlendOp(RenderBackendBlendOp blendOp)
    {
        switch (blendOp)
        {
        case RenderBackendBlendOp::Add:
            return D3D12_BLEND_OP_ADD;
        case RenderBackendBlendOp::Subtract:
            return D3D12_BLEND_OP_SUBTRACT;
        case RenderBackendBlendOp::ReverseSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case RenderBackendBlendOp::Min:
            return D3D12_BLEND_OP_MIN;
        case RenderBackendBlendOp::Max:
            return D3D12_BLEND_OP_MAX;
        default:
            std::unreachable();
            return D3D12_BLEND_OP_ADD;
        }
    }

    static inline UINT8 ConvertToD3D12RenderTargetWriteMask(RenderBackendColorComponentFlags writeMask)
    {
        UINT8 mask = 0;
        if (writeMask == RenderBackendColorComponentFlags::RGBA)
        {
            return D3D12_COLOR_WRITE_ENABLE_ALL;
        }
        else
        {
            if (EnumClassHasFlags(writeMask, RenderBackendColorComponentFlags::R))
            {
                mask |= D3D12_COLOR_WRITE_ENABLE_RED;
            }
            if (EnumClassHasFlags(writeMask, RenderBackendColorComponentFlags::G))
            {
                mask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
            }
            if (EnumClassHasFlags(writeMask, RenderBackendColorComponentFlags::B))
            {
                mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
            }
            if (EnumClassHasFlags(writeMask, RenderBackendColorComponentFlags::A))
            {
                mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
            }
        }
        return mask;
    }

    static inline D3D12_STENCIL_OP ConvertToD3D12StencilOp(RenderBackendStencilOp stencilOp)
    {
        switch (stencilOp)
        {
        case RenderBackendStencilOp::Keep:
            return D3D12_STENCIL_OP_KEEP;
        case RenderBackendStencilOp::Zero:
            return D3D12_STENCIL_OP_ZERO;
        case RenderBackendStencilOp::Replace:
            return D3D12_STENCIL_OP_REPLACE;
        case RenderBackendStencilOp::IncreaseAndClamp:
            return D3D12_STENCIL_OP_INCR_SAT;
        case RenderBackendStencilOp::DecreaseAndClamp:
            return D3D12_STENCIL_OP_DECR_SAT;
        case RenderBackendStencilOp::Invert:
            return D3D12_STENCIL_OP_INVERT;
        case RenderBackendStencilOp::IncreaseAndWrap:
            return D3D12_STENCIL_OP_INCR;
        case RenderBackendStencilOp::DecreaseAndWrap:
            return D3D12_STENCIL_OP_DECR;
        default:
            std::unreachable();
            return D3D12_STENCIL_OP_KEEP;
        }
    }

    static inline D3D12_CULL_MODE ConvertToD3D12CullMode(RenderBackendRasterizationCullMode cullMode)
    {
        switch (cullMode)
        {
        case RenderBackendRasterizationCullMode::None:
            return D3D12_CULL_MODE_NONE;
        case RenderBackendRasterizationCullMode::Back:
            return D3D12_CULL_MODE_BACK;
        case RenderBackendRasterizationCullMode::Front:
            return D3D12_CULL_MODE_FRONT;
        default:
            std::unreachable();
            return D3D12_CULL_MODE_NONE;
        }
    }

    static inline D3D12_FILL_MODE ConvertToD3D12FillMode(RenderBackendRasterizationFillMode fillMode)
    {
        switch (fillMode)
        {
        case RenderBackendRasterizationFillMode::Wireframe:
            return D3D12_FILL_MODE_WIREFRAME;
        case RenderBackendRasterizationFillMode::Solid:
            return D3D12_FILL_MODE_SOLID;
        default:
            std::unreachable();
            return D3D12_FILL_MODE_SOLID;
        }
    }

    static inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS ConvertToD3D12RayTracingAccelerationStructureBuildFlags(RenderBackendRayTracingAccelerationStructureBuildFlags flags)
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS result = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::AllowUpdate))
        {
            result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        }
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::AllowCompaction))
        {
            result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
        }
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastTrace))
        {
            result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        }
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastBuild))
        {
            result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
        }
        if (EnumClassHasFlags(flags, RenderBackendRayTracingAccelerationStructureBuildFlags::MinimizeMemory))
        {
            result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
        }
        return result;
    }

    static inline D3D12_RAYTRACING_GEOMETRY_TYPE ConvertToD3D12RayTracingGeometryType(RenderBackendRayTracingGeometryType type)
    {
        switch (type)
        {
        case RenderBackendRayTracingGeometryType::Triangles:
            return D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        case RenderBackendRayTracingGeometryType::AABBs:
            return D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
        default:
            std::unreachable();
            return D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        }
    }

    static inline D3D12_RAYTRACING_GEOMETRY_FLAGS ConvertToD3D12RayTracingGeometryFlags(RenderBackendRayTracingGeometryFlags flags)
    {
        D3D12_RAYTRACING_GEOMETRY_FLAGS result = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
        if (EnumClassHasFlags(flags, RenderBackendRayTracingGeometryFlags::Opaque))
        {
            result |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        }
        if (EnumClassHasFlags(flags, RenderBackendRayTracingGeometryFlags::NoDuplicateAnyHitInvocation))
        {
            result |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
        }
        return result;
    }

    inline void ConvertToD3D12TextureBarrier(
        RenderBackendResourceState srcState,
        RenderBackendResourceState dstState,
        D3D12_BARRIER_SYNC& outSyncBefore,
        D3D12_BARRIER_SYNC& outSyncAfter,
        D3D12_BARRIER_ACCESS& outAccessBefore,
        D3D12_BARRIER_ACCESS& outAccessAfter,
        D3D12_BARRIER_LAYOUT& outLayoutBefore,
        D3D12_BARRIER_LAYOUT& outLayoutAfter,
        D3D12_TEXTURE_BARRIER_FLAGS& outFlags)
    {
        switch (srcState)
        {
        case RenderBackendResourceState::Undefined:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED;
            outFlags |= D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
            break;
        case RenderBackendResourceState::VertexBuffer:
        case RenderBackendResourceState::IndexBuffer:
        case RenderBackendResourceState::ShaderResource:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
            break;
        case RenderBackendResourceState::Present:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_COMMON;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_PRESENT;
            break;
        case RenderBackendResourceState::RenderTarget:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
            break;
        case RenderBackendResourceState::DepthStencilReadOnly:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
            break;
        case RenderBackendResourceState::DepthStencil:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
            break;
        case RenderBackendResourceState::CopySrc:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_COPY_SOURCE;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_COPY_SOURCE;
            break;
        case RenderBackendResourceState::CopyDst:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_COPY_DEST;
            break;
        case RenderBackendResourceState::UnorderedAccess:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
            break;
        case RenderBackendResourceState::IndirectArgument:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
            outLayoutBefore = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
            break;
        default: std::unreachable(); break;
        }

        switch (dstState)
        {
        case RenderBackendResourceState::Undefined:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_UNDEFINED;
            break;
        case RenderBackendResourceState::VertexBuffer:
        case RenderBackendResourceState::IndexBuffer:
        case RenderBackendResourceState::ShaderResource:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
            break;
        case RenderBackendResourceState::Present:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_COMMON;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;
            break;
        case RenderBackendResourceState::RenderTarget:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
            break;
        case RenderBackendResourceState::DepthStencilReadOnly:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
            break;
        case RenderBackendResourceState::DepthStencil:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
            break;
        case RenderBackendResourceState::CopySrc:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_COPY_SOURCE;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_COPY_SOURCE;
            break;
        case RenderBackendResourceState::CopyDst:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_COPY_DEST;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_COPY_DEST;
            break;
        case RenderBackendResourceState::UnorderedAccess:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
            break;
        case RenderBackendResourceState::IndirectArgument:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
            outLayoutAfter = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
            break;
        default: std::unreachable(); break;
        }
    }

    inline D3D12_BARRIER_LAYOUT ConvertToD3D12BarrierLayout(RenderBackendResourceState state)
    {
        switch (state)
        {
        case RenderBackendResourceState::Undefined:
            return D3D12_BARRIER_LAYOUT_UNDEFINED;
        case RenderBackendResourceState::Present:
            return D3D12_BARRIER_LAYOUT_PRESENT;
        case RenderBackendResourceState::RenderTarget:
            return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
        case RenderBackendResourceState::UnorderedAccess:
            return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
        case RenderBackendResourceState::DepthStencil:
            return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE; // TODO
        case RenderBackendResourceState::DepthStencilReadOnly:
            return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
        case RenderBackendResourceState::ShaderResource:
        case RenderBackendResourceState::IndirectArgument:
            return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
        case RenderBackendResourceState::VertexBuffer:
            return D3D12_BARRIER_LAYOUT_COMMON;
        case RenderBackendResourceState::IndexBuffer:
            return D3D12_BARRIER_LAYOUT_COMMON;
        case RenderBackendResourceState::CopySrc:
            return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
        case RenderBackendResourceState::CopyDst:
            return D3D12_BARRIER_LAYOUT_COPY_DEST;
        default:
            std::unreachable();
        }
    }

    inline void ConvertToD3D12BufferBarrier(
        RenderBackendResourceState srcState,
        RenderBackendResourceState dstState,
        D3D12_BARRIER_SYNC& outSyncBefore,
        D3D12_BARRIER_SYNC& outSyncAfter,
        D3D12_BARRIER_ACCESS& outAccessBefore,
        D3D12_BARRIER_ACCESS& outAccessAfter)
    {
        switch (srcState)
        {
        case RenderBackendResourceState::Undefined:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
            break;
        case RenderBackendResourceState::VertexBuffer:
        case RenderBackendResourceState::IndexBuffer:
        case RenderBackendResourceState::ShaderResource:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
            break;
        case RenderBackendResourceState::CopySrc:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_COPY_SOURCE;
            break;
        case RenderBackendResourceState::CopyDst:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
            break;
        case RenderBackendResourceState::UnorderedAccess:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            break;
        case RenderBackendResourceState::IndirectArgument:
            outSyncBefore = D3D12_BARRIER_SYNC_ALL;
            outAccessBefore = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
            break;
        default: std::unreachable(); break;
        }

        switch (dstState)
        {
        case RenderBackendResourceState::Undefined:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
            break;
        case RenderBackendResourceState::VertexBuffer:
        case RenderBackendResourceState::IndexBuffer:
        case RenderBackendResourceState::ShaderResource:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
            break;
        case RenderBackendResourceState::CopySrc:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_COPY_SOURCE;
            break;
        case RenderBackendResourceState::CopyDst:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_COPY_DEST;
            break;
        case RenderBackendResourceState::UnorderedAccess:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            break;
        case RenderBackendResourceState::IndirectArgument:
            outSyncAfter = D3D12_BARRIER_SYNC_ALL;
            outAccessAfter = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
            break;
        default: std::unreachable(); break;
        }
    }
}