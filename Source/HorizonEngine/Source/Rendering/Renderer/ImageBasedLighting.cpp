#include "ImageBasedLighting.h"
#include "RenderUtility.h"
#include "ShaderRepository.h"

namespace Horizon
{
    uint32 GEnvironmentBrdfLutTextureSize = 256;

    uint32 GIrradianceEnvironmentMapSize = 32;

    void RenderEnvironmentBrdfLut(RenderBackend* renderBackend, ShaderRepository* shaderRepository, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentBrdfLutTexture)
    {
        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::EnvironmentBRDFIntegration);

        RenderBackendBarrier transition(environmentBrdfLutTexture, RenderBackendTextureSubresourceRange::All, RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Barriers(&transition, 1);

        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(GEnvironmentBrdfLutTextureSize, 8);
        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(GEnvironmentBrdfLutTextureSize, 8);
        uint32 threadGroupCountZ = 1;

        RenderBackendPushConstantValues pushConstantValues = {};
        pushConstantValues.BindTextureUAV(0, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(environmentBrdfLutTexture, 0));

        commandList.Dispatch(
            computeShader,
            pushConstantValues,
            threadGroupCountX,
            threadGroupCountY,
            threadGroupCountZ);

        transition = RenderBackendBarrier(environmentBrdfLutTexture, RenderBackendTextureSubresourceRange::All, RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Barriers(&transition, 1);
    }

    void GenerateCubemapMips(RenderBackend* renderBackend, ShaderRepository* shaderRepository, RenderBackendCommandList& commandList, RenderBackendTextureHandle cubemapTexture, uint32 mipLevelCount)
    {
        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::DownsampleCubemap);

        for (uint32 mipLevel = 1; mipLevel < mipLevelCount; mipLevel++)
        {
            RenderBackendBarrier transitions[] =
            {
                RenderBackendBarrier(cubemapTexture, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
            };
            commandList.Barriers(transitions, 1);

            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(1 << (mipLevelCount - mipLevel - 1), 8);
            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(1 << (mipLevelCount - mipLevel - 1), 8);
            uint32 threadGroupCountZ = 1;

            RenderBackendPushConstantValues pushConstantValues = {};
            pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(cubemapTexture));
            pushConstantValues.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(cubemapTexture, mipLevel));
            pushConstantValues.OverrideShaderConstantValue(2, mipLevel - 1u);

            commandList.Dispatch(
                computeShader,
                pushConstantValues,
                threadGroupCountX,
                threadGroupCountY,
                threadGroupCountZ);
        }

        RenderBackendBarrier transition = RenderBackendBarrier(cubemapTexture, RenderBackendTextureSubresourceRange(mipLevelCount - 1, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Barriers(&transition, 1);
    }

    void ComputeEnvironmentIrradianceMap(RenderBackend* renderBackend, ShaderRepository* shaderRepository, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 mipLevel, RenderBackendTextureHandle irradianceEnvironmentMap)
    {
        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::IrradianceEnvironmentMapReference);

        RenderBackendBarrier transition(irradianceEnvironmentMap, RenderBackendTextureSubresourceRange(0, 1, 0, 6), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Barriers(&transition, 1);

        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(GIrradianceEnvironmentMapSize, 8);
        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(GIrradianceEnvironmentMapSize, 8);
        uint32 threadGroupCountZ = 1;

        RenderBackendPushConstantValues pushConstantValues = {};
        pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
        pushConstantValues.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMap, 0));
        pushConstantValues.OverrideShaderConstantValue(2, mipLevel);

        commandList.Dispatch(
            computeShader,
            pushConstantValues,
            threadGroupCountX,
            threadGroupCountY,
            threadGroupCountZ);

        transition = RenderBackendBarrier(irradianceEnvironmentMap, RenderBackendTextureSubresourceRange(0, 1, 0, 6), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Barriers(&transition, 1);
    }

    void ComputeEnvironmentIrradianceMapSH(RenderBackend* renderBackend, ShaderRepository* shaderRepository, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 environmentMapSize, RenderBackendBufferHandle irradianceEnvironmentMapBuffer)
    {
        /*static const uint32 log2_16 = 4;
        uint32 sourceMipLevel = uint32(std::log2(float(environmentMapSize))) - log2_16;

        {
            RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::IrradianceEnvironmentMapSHSampling);

            RenderBackendPushConstantValues pushConstantValues = {};
            pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
            pushConstantValues.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapSHCoefficientsRGBBuffer));
            pushConstantValues.BindScalar(2, sourceMipLevel);

            commandList.Dispatch(
                computeShader,
                pushConstantValues,
                1,
                1,
                1);
        }

        {
            RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::IrradianceEnvironmentMapSHIntegration);

            RenderBackendPushConstantValues pushConstantValues = {};
            pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(irradianceEnvironmentMapSHCoefficientsRGBBuffer));
            pushConstantValues.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapBuffer));
            pushConstantValues.BindScalar(2, shCoefficientsRGBCount);

            commandList.Dispatch(
                computeShader,
                pushConstantValues,
                1,
                1,
                1);
        }*/
    }

    void ComputeEnvironmentIrradianceMapSHFast(RenderBackend* renderBackend, ShaderRepository* shaderRepository, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 environmentMapSize, RenderBackendBufferHandle irradianceEnvironmentMapBuffer)
    {
        static const uint32 log2_16 = 4;
        uint32 sourceMipLevel = uint32(std::log2(float(environmentMapSize))) - log2_16;

        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::IrradianceEnvironmentMapSHOnePass);

        RenderBackendPushConstantValues pushConstantValues = {};
        pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
        pushConstantValues.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapBuffer));
        pushConstantValues.OverrideShaderConstantValue(2, sourceMipLevel);

        commandList.Dispatch(
            computeShader,
            pushConstantValues,
            1,
            1,
            1);
    }

    void ConvolveEnvironmentMap(RenderBackend* renderBackend, ShaderRepository* shaderRepository, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 numMipLevels, RenderBackendTextureHandle convolvedEnvironmentMap)
    {
        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::EnvironmentMapConvolution);

        RenderBackendBarrier transition(convolvedEnvironmentMap, RenderBackendTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Barriers(&transition, 1);

        for (uint32 mipLevel = 0; mipLevel < numMipLevels; mipLevel++)
        {
            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(1 << (numMipLevels - mipLevel - 1), 8);
            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(1 << (numMipLevels - mipLevel - 1), 8);
            uint32 threadGroupCountZ = 1;

            float roughness = (float)mipLevel / (float)(numMipLevels - 1);

            RenderBackendPushConstantValues pushConstantValues = {};
            pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
            pushConstantValues.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(convolvedEnvironmentMap, mipLevel));
            pushConstantValues.OverrideShaderConstantValue(2, roughness);

            commandList.Dispatch(
                computeShader,
                pushConstantValues,
                threadGroupCountX,
                threadGroupCountY,
                threadGroupCountZ);
        }

        transition = RenderBackendBarrier(convolvedEnvironmentMap, RenderBackendTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Barriers(&transition, 1);
    }

    void PrecomputeEnvironmentMaps(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList& commandList,
        uint32 cubemapSize,
        RenderBackendTextureHandle environmentMapTexture,
        RenderBackendTextureHandle convolvedEnvironmentMap,
        RenderBackendTextureHandle irradianceEnvironmentMapTexture,
        RenderBackendBufferHandle irradianceEnvironmentMapBuffer,
        RenderBackendBufferHandle irradianceEnvironmentMapBufferFast)
    {
        const uint32 mipLevelCount = Math::MaxMipLevelCount(cubemapSize);

        //GenerateCubemapMips(renderBackend, shaderRepository, commandList, environmentMapTexture, mipLevelCount);

        const uint32 irradianceEnvironmentMapMipLevelCount = Math::MaxMipLevelCount(GIrradianceEnvironmentMapSize);
        const uint32 sourceMipLevel = Math::Max<uint32>(0, mipLevelCount - irradianceEnvironmentMapMipLevelCount);

        ComputeEnvironmentIrradianceMap(renderBackend, shaderRepository, commandList, environmentMapTexture, sourceMipLevel, irradianceEnvironmentMapTexture);

        ComputeEnvironmentIrradianceMapSH(renderBackend, shaderRepository, commandList, environmentMapTexture, cubemapSize, irradianceEnvironmentMapBuffer);
        ComputeEnvironmentIrradianceMapSHFast(renderBackend, shaderRepository, commandList, environmentMapTexture, cubemapSize, irradianceEnvironmentMapBufferFast);

        ConvolveEnvironmentMap(renderBackend, shaderRepository, commandList, environmentMapTexture, mipLevelCount, convolvedEnvironmentMap);
    }

    void ConvertLatLongToCubemap(RenderBackend* renderBackend, ShaderRepository* shaderRepository, RenderBackendCommandList& commandList, RenderBackendTextureHandle latLongTexture, RenderBackendTextureHandle cubemapTexture, uint32 cubemapTextureSize)
    {
        RenderBackendBarrier transition(cubemapTexture, RenderBackendTextureSubresourceRange::All, RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Barriers(&transition, 1);

        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(cubemapTextureSize, 8);
        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(cubemapTextureSize, 8);
        uint32 threadGroupCountZ = 1;

        RenderBackendPushConstantValues pushConstantValues = {};
        pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(latLongTexture));
        pushConstantValues.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(cubemapTexture, 0));

        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::LatLongToCubemap);

        commandList.Dispatch(
            computeShader,
            pushConstantValues,
            threadGroupCountX,
            threadGroupCountY,
            threadGroupCountZ);
    }
}