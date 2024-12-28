#include "ImageBasedLighting.h"
#include "RenderUtility.h"
#include "ShaderLibrary.h"

namespace Horizon
{
    uint32 GEnvironmentBrdfLutTextureSize = 256;

    uint32 GIrradianceEnvironmentMapSize = 32;

    void RenderEnvironmentBrdfLut(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentBrdfLutTexture)
    {
        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::EnvironmentBRDFIntegration);

        RenderBackendBarrier transition(environmentBrdfLutTexture, RenderBackendTextureSubresourceRange::All, RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Transitions(&transition, 1);

        uint32 threadGroupCountX = CeilDiv(GEnvironmentBrdfLutTextureSize, 8);
        uint32 threadGroupCountY = CeilDiv(GEnvironmentBrdfLutTextureSize, 8);
        uint32 threadGroupCountZ = 1;

        RenderBackendShaderConstants shaderConstants = {};
        shaderConstants.BindTextureUAV(0, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(environmentBrdfLutTexture, 0));

        commandList.Dispatch(
            computeShader,
            shaderConstants,
            threadGroupCountX,
            threadGroupCountY,
            threadGroupCountZ);

        transition = RenderBackendBarrier(environmentBrdfLutTexture, RenderBackendTextureSubresourceRange::All, RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Transitions(&transition, 1);
    }

    void GenerateCubemapMips(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle cubemapTexture, uint32 mipLevelCount)
    {
        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::DownsampleCubemap);

        for (uint32 mipLevel = 1; mipLevel < mipLevelCount; mipLevel++)
        {
            RenderBackendBarrier transitions[] =
            {
                RenderBackendBarrier(cubemapTexture, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
            };
            commandList.Transitions(transitions, 1);

            uint32 threadGroupCountX = CeilDiv(1 << (mipLevelCount - mipLevel - 1), 8);
            uint32 threadGroupCountY = CeilDiv(1 << (mipLevelCount - mipLevel - 1), 8);
            uint32 threadGroupCountZ = 1;

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(cubemapTexture));
            shaderConstants.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(cubemapTexture, mipLevel));
            shaderConstants.BindScalar(2, mipLevel - 1u);

            commandList.Dispatch(
                computeShader,
                shaderConstants,
                threadGroupCountX,
                threadGroupCountY,
                threadGroupCountZ);
        }

        RenderBackendBarrier transition = RenderBackendBarrier(cubemapTexture, RenderBackendTextureSubresourceRange(mipLevelCount - 1, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Transitions(&transition, 1);
    }

    void ComputeEnvironmentIrradianceMap(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 mipLevel, RenderBackendTextureHandle irradianceEnvironmentMap)
    {
        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::IrradianceEnvironmentMapReference);

        RenderBackendBarrier transition(irradianceEnvironmentMap, RenderBackendTextureSubresourceRange(0, 1, 0, 6), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Transitions(&transition, 1);

        uint32 threadGroupCountX = CeilDiv(GIrradianceEnvironmentMapSize, 8);
        uint32 threadGroupCountY = CeilDiv(GIrradianceEnvironmentMapSize, 8);
        uint32 threadGroupCountZ = 1;

        RenderBackendShaderConstants shaderConstants = {};
        shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
        shaderConstants.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMap, 0));
        shaderConstants.BindScalar(2, mipLevel);

        commandList.Dispatch(
            computeShader,
            shaderConstants,
            threadGroupCountX,
            threadGroupCountY,
            threadGroupCountZ);

        transition = RenderBackendBarrier(irradianceEnvironmentMap, RenderBackendTextureSubresourceRange(0, 1, 0, 6), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Transitions(&transition, 1);
    }

    void ComputeEnvironmentIrradianceMapSH(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 environmentMapSize, RenderBackendBufferHandle irradianceEnvironmentMapBuffer)
    {
        /*static const uint32 log2_16 = 4;
        uint32 sourceMipLevel = uint32(std::log2(float(environmentMapSize))) - log2_16;

        {
            RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::IrradianceEnvironmentMapSHSampling);

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
            shaderConstants.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapSHCoefficientsRGBBuffer));
            shaderConstants.BindScalar(2, sourceMipLevel);

            commandList.Dispatch(
                computeShader,
                shaderConstants,
                1,
                1,
                1);
        }

        {
            RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::IrradianceEnvironmentMapSHIntegration);

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(irradianceEnvironmentMapSHCoefficientsRGBBuffer));
            shaderConstants.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapBuffer));
            shaderConstants.BindScalar(2, shCoefficientsRGBCount);

            commandList.Dispatch(
                computeShader,
                shaderConstants,
                1,
                1,
                1);
        }*/
    }

    void ComputeEnvironmentIrradianceMapSHFast(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 environmentMapSize, RenderBackendBufferHandle irradianceEnvironmentMapBuffer)
    {
        static const uint32 log2_16 = 4;
        uint32 sourceMipLevel = uint32(std::log2(float(environmentMapSize))) - log2_16;

        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::IrradianceEnvironmentMapSHOnePass);

        RenderBackendShaderConstants shaderConstants = {};
        shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
        shaderConstants.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapBuffer));
        shaderConstants.BindScalar(2, sourceMipLevel);

        commandList.Dispatch(
            computeShader,
            shaderConstants,
            1,
            1,
            1);
    }

    void ConvolveEnvironmentMap(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 numMipLevels, RenderBackendTextureHandle convolvedEnvironmentMap)
    {
        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::EnvironmentMapConvolution);

        RenderBackendBarrier transition(convolvedEnvironmentMap, RenderBackendTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Transitions(&transition, 1);

        for (uint32 mipLevel = 0; mipLevel < numMipLevels; mipLevel++)
        {
            uint32 threadGroupCountX = CeilDiv(1 << (numMipLevels - mipLevel - 1), 8);
            uint32 threadGroupCountY = CeilDiv(1 << (numMipLevels - mipLevel - 1), 8);
            uint32 threadGroupCountZ = 1;

            float roughness = (float)mipLevel / (float)(numMipLevels - 1);

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
            shaderConstants.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(convolvedEnvironmentMap, mipLevel));
            shaderConstants.BindScalar(2, roughness);

            commandList.Dispatch(
                computeShader,
                shaderConstants,
                threadGroupCountX,
                threadGroupCountY,
                threadGroupCountZ);
        }

        transition = RenderBackendBarrier(convolvedEnvironmentMap, RenderBackendTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Transitions(&transition, 1);
    }

    void PrecomputeEnvironmentMaps(
        RenderBackend* renderBackend,
        ShaderLibrary* shaderLibrary,
        RenderBackendCommandList& commandList,
        uint32 cubemapSize,
        RenderBackendTextureHandle environmentMapTexture,
        RenderBackendTextureHandle convolvedEnvironmentMap,
        RenderBackendTextureHandle irradianceEnvironmentMapTexture,
        RenderBackendBufferHandle irradianceEnvironmentMapBuffer,
        RenderBackendBufferHandle irradianceEnvironmentMapBufferFast)
    {
        const uint32 mipLevelCount = Math::MaxMipLevelCount(cubemapSize);

        //GenerateCubemapMips(renderBackend, shaderLibrary, commandList, environmentMapTexture, mipLevelCount);

        const uint32 irradianceEnvironmentMapMipLevelCount = Math::MaxMipLevelCount(GIrradianceEnvironmentMapSize);
        const uint32 sourceMipLevel = Math::Max<uint32>(0, mipLevelCount - irradianceEnvironmentMapMipLevelCount);

        ComputeEnvironmentIrradianceMap(renderBackend, shaderLibrary, commandList, environmentMapTexture, sourceMipLevel, irradianceEnvironmentMapTexture);

        ComputeEnvironmentIrradianceMapSH(renderBackend, shaderLibrary, commandList, environmentMapTexture, cubemapSize, irradianceEnvironmentMapBuffer);
        ComputeEnvironmentIrradianceMapSHFast(renderBackend, shaderLibrary, commandList, environmentMapTexture, cubemapSize, irradianceEnvironmentMapBufferFast);

        ConvolveEnvironmentMap(renderBackend, shaderLibrary, commandList, environmentMapTexture, mipLevelCount, convolvedEnvironmentMap);
    }

    void ConvertLatLongToCubemap(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle latLongTexture, RenderBackendTextureHandle cubemapTexture, uint32 cubemapTextureSize)
    {
        RenderBackendBarrier transition(cubemapTexture, RenderBackendTextureSubresourceRange::All, RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Transitions(&transition, 1);

        uint32 threadGroupCountX = CeilDiv(cubemapTextureSize, 8);
        uint32 threadGroupCountY = CeilDiv(cubemapTextureSize, 8);
        uint32 threadGroupCountZ = 1;

        RenderBackendShaderConstants shaderConstants = {};
        shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(latLongTexture));
        shaderConstants.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(cubemapTexture, 0));

        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LatLongToCubemap);

        commandList.Dispatch(
            computeShader,
            shaderConstants,
            threadGroupCountX,
            threadGroupCountY,
            threadGroupCountZ);
    }
}