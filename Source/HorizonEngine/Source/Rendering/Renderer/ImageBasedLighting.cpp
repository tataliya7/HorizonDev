#include "ImageBasedLighting.h"
#include "RenderUtility.h"
#include "ShaderRepository.h"

namespace Horizon
{
    void ConvertLatLongToCubemap(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        RenderBackendTextureHandle latLongTexture,
        RenderBackendTextureHandle cubemapTexture,
        uint32 cubemapTextureSize)
    {
        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::LatLongToCubemap);
        if (computeShader)
        {
            RenderBackendBarrier transitionBefore = RenderBackendBarrier(
                cubemapTexture,
                RenderBackendTextureSubresourceRange::All,
                RenderBackendResourceState::Undefined,
                RenderBackendResourceState::UnorderedAccess);
            commandList->Barriers(&transitionBefore, 1);

            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(cubemapTextureSize, 8);
            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(cubemapTextureSize, 8);
            uint32 threadGroupCountZ = 1;

            RenderBackendPushConstantValues pushConstantValues = {};
            pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(latLongTexture));
            pushConstantValues.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(cubemapTexture, 0));

            commandList->Dispatch(
                computeShader,
                pushConstantValues,
                threadGroupCountX,
                threadGroupCountY,
                threadGroupCountZ);

            // RenderBackendBarrier transitionAfter = RenderBackendBarrier(
            //     cubemapTexture,
            //     RenderBackendTextureSubresourceRange::All,
            //     RenderBackendResourceState::UnorderedAccess,
            //     RenderBackendResourceState::ShaderResource);
            // commandList->Barriers(&transitionAfter, 1);
        }
    }

    void GenerateCubemapMips(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        RenderBackendTextureHandle cubemapTexture,
        uint32 mipLevelCount)
    {
        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::DownsampleCubemap);
        if (computeShader)
        {
            for (uint32 mipLevel = 1; mipLevel < mipLevelCount; mipLevel++)
            {
                RenderBackendBarrier transitions[] =
                {
                    RenderBackendBarrier(cubemapTexture, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                };
                commandList->Barriers(transitions, 1);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(1 << (mipLevelCount - mipLevel - 1), 8);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(1 << (mipLevelCount - mipLevel - 1), 8);
                uint32 threadGroupCountZ = 1;

                RenderBackendPushConstantValues pushConstantValues = {};
                pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(cubemapTexture));
                pushConstantValues.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(cubemapTexture, mipLevel));
                pushConstantValues.BindShaderConstant(2, mipLevel - 1u);

                commandList->Dispatch(
                    computeShader,
                    pushConstantValues,
                    threadGroupCountX,
                    threadGroupCountY,
                    threadGroupCountZ);
            }

            RenderBackendBarrier transition = RenderBackendBarrier(cubemapTexture, RenderBackendTextureSubresourceRange(mipLevelCount - 1, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
            commandList->Barriers(&transition, 1);
        }
    }

    uint32 EnvironmentBrdfLutTextureSize = 256;

    void PrecomputeEnvironmentBRDFLookupTable(RenderBackend* renderBackend, ShaderRepository* shaderRepository, RenderBackendCommandList* commandList, RenderBackendTextureHandle environmentBrdfLutTexture)
    {
        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::EnvironmentBRDFIntegration);
        if (computeShader)
        {
            RenderBackendBarrier transitionBefore = RenderBackendBarrier(
                environmentBrdfLutTexture,
                RenderBackendTextureSubresourceRange::All,
                RenderBackendResourceState::Undefined,
                RenderBackendResourceState::UnorderedAccess);
            commandList->Barriers(&transitionBefore, 1);

            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(EnvironmentBrdfLutTextureSize, 8);
            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(EnvironmentBrdfLutTextureSize, 8);
            uint32 threadGroupCountZ = 1;

            RenderBackendPushConstantValues pushConstantValues = {};
            pushConstantValues.BindTextureUAV(0, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(environmentBrdfLutTexture, 0));

            commandList->Dispatch(
                computeShader,
                pushConstantValues,
                threadGroupCountX,
                threadGroupCountY,
                threadGroupCountZ);

            RenderBackendBarrier transitionAfter = RenderBackendBarrier(
                environmentBrdfLutTexture,
                RenderBackendTextureSubresourceRange::All,
                RenderBackendResourceState::UnorderedAccess,
                RenderBackendResourceState::ShaderResource);
            commandList->Barriers(&transitionAfter, 1);
        }
    }

    static void ComputeEnvironmentIrradianceMap(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        RenderBackendTextureHandle environmentMapTexture,
        uint32 environmentMapTextureMipLevel,
        uint32 irradianceEnvironmentMapTextureSize,
        RenderBackendTextureHandle irradianceEnvironmentMapTexture)
    {
        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::IrradianceEnvironmentMapReference);
        if (computeShader)
        {
            RenderBackendBarrier transitionBefore = RenderBackendBarrier(
                irradianceEnvironmentMapTexture,
                RenderBackendTextureSubresourceRange(0, 1, 0, 6),
                RenderBackendResourceState::Undefined,
                RenderBackendResourceState::UnorderedAccess);
            commandList->Barriers(&transitionBefore, 1);

            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(irradianceEnvironmentMapTextureSize, 8);
            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(irradianceEnvironmentMapTextureSize, 8);
            uint32 threadGroupCountZ = 1;

            RenderBackendPushConstantValues pushConstantValues = {};
            pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMapTexture));
            pushConstantValues.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapTexture, 0));
            pushConstantValues.BindShaderConstant(2, environmentMapTextureMipLevel);

            commandList->Dispatch(
                computeShader,
                pushConstantValues,
                threadGroupCountX,
                threadGroupCountY,
                threadGroupCountZ);

            RenderBackendBarrier transitionAfter = RenderBackendBarrier(
                irradianceEnvironmentMapTexture,
                RenderBackendTextureSubresourceRange(0, 1, 0, 6),
                RenderBackendResourceState::UnorderedAccess,
                RenderBackendResourceState::ShaderResource);
            commandList->Barriers(&transitionAfter, 1);
        }
    }

    static void ComputeEnvironmentIrradianceMapSH(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        RenderBackendTextureHandle environmentMapTexture,
        uint32 environmentMapTextureSize,
        RenderBackendBufferHandle irradianceEnvironmentMapBuffer)
    {
        static constexpr uint32 log2_16 = 4;
        uint32 sourceMipLevel = static_cast<uint32>(std::log2(static_cast<float>(environmentMapTextureSize))) - log2_16;

        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::IrradianceEnvironmentMapSHOnePass);
        if (computeShader)
        {
            RenderBackendPushConstantValues pushConstantValues = {};
            pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMapTexture));
            pushConstantValues.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapBuffer));
            pushConstantValues.OverrideShaderConstantValue(2, sourceMipLevel);

            commandList->Dispatch(
                computeShader,
                pushConstantValues,
                1,
                1,
                1);

        }
        // {
        //     RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::IrradianceEnvironmentMapSHSampling);
        //     if (computeShader)
        //     {
        //         RenderBackendPushConstantValues pushConstantValues = {};
        //         pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMapTexture));
        //         pushConstantValues.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(intermediateIrradianceEnvironmentMapBuffer));
        //         pushConstantValues.BindShaderConstant(2, sourceMipLevel);
        //
        //         commandList->Dispatch(
        //             computeShader,
        //             pushConstantValues,
        //             1,
        //             1,
        //             1);
        //     }
        // }
        // {
        //     RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::IrradianceEnvironmentMapSHIntegration);
        //     if (computeShader)
        //     {
        //         RenderBackendPushConstantValues pushConstantValues = {};
        //         pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(intermediateIrradianceEnvironmentMapBuffer));
        //         pushConstantValues.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapBuffer));
        //         pushConstantValues.BindShaderConstant(2, shCoefficientsRGBCount);
        //
        //         commandList->Dispatch(
        //             computeShader,
        //             pushConstantValues,
        //             1,
        //             1,
        //             1);
        //     }
        // }
    }

    static void ConvolveEnvironmentMap(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        RenderBackendTextureHandle environmentMapTexture,
        uint32 environmentMapTextureMipLevelCount,
        RenderBackendTextureHandle convolvedEnvironmentMapTexture)
    {
        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::EnvironmentMapConvolution);
        if (computeShader)
        {
            RenderBackendBarrier transitionBefore = RenderBackendBarrier(
                convolvedEnvironmentMapTexture,
                RenderBackendTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers),
                RenderBackendResourceState::Undefined,
                RenderBackendResourceState::UnorderedAccess);
            commandList->Barriers(&transitionBefore, 1);

            for (uint32 mipLevel = 0; mipLevel < environmentMapTextureMipLevelCount; mipLevel++)
            {
                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(1 << (environmentMapTextureMipLevelCount - mipLevel - 1), 8);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(1 << (environmentMapTextureMipLevelCount - mipLevel - 1), 8);
                uint32 threadGroupCountZ = 1;

                float roughness = static_cast<float>(mipLevel) / static_cast<float>(environmentMapTextureMipLevelCount - 1);

                RenderBackendPushConstantValues pushConstantValues = {};
                pushConstantValues.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMapTexture));
                pushConstantValues.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(convolvedEnvironmentMapTexture, mipLevel));
                pushConstantValues.OverrideShaderConstantValue(2, roughness);

                commandList->Dispatch(
                    computeShader,
                    pushConstantValues,
                    threadGroupCountX,
                    threadGroupCountY,
                    threadGroupCountZ);
            }

            RenderBackendBarrier transitionAfter = RenderBackendBarrier(
                convolvedEnvironmentMapTexture,
                RenderBackendTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers),
                RenderBackendResourceState::UnorderedAccess,
                RenderBackendResourceState::ShaderResource);
            commandList->Barriers(&transitionAfter, 1);
        }
    }

    void PrecomputeEnvironmentMaps(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        uint32 environmentMapTextureSize,
        RenderBackendTextureHandle environmentMapTexture,
        uint32 irradianceEnvironmentMapTextureSize,
        RenderBackendTextureHandle convolvedEnvironmentMapTexture,
        RenderBackendTextureHandle irradianceEnvironmentMapTexture)
    {
        const uint32 environmentMapTextureMipLevelCount = Math::MaxMipLevelCount(environmentMapTextureSize);
        const uint32 irradianceEnvironmentMapMipLevelCount = std::min<uint32>(Math::MaxMipLevelCount(irradianceEnvironmentMapTextureSize), environmentMapTextureMipLevelCount);
        const uint32 sourceMipLevel = std::max<uint32>(0, environmentMapTextureMipLevelCount - irradianceEnvironmentMapMipLevelCount);

        ComputeEnvironmentIrradianceMap(renderBackend, shaderRepository, commandList, environmentMapTexture, sourceMipLevel, irradianceEnvironmentMapTextureSize, irradianceEnvironmentMapTexture);

        ConvolveEnvironmentMap(renderBackend, shaderRepository, commandList, environmentMapTexture, environmentMapTextureMipLevelCount, convolvedEnvironmentMapTexture);
    }

    void PrecomputeEnvironmentMaps(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        uint32 environmentMapTextureSize,
        RenderBackendTextureHandle environmentMapTexture,
        RenderBackendTextureHandle convolvedEnvironmentMapTexture,
        RenderBackendBufferHandle irradianceEnvironmentMapBuffer)
    {
        ComputeEnvironmentIrradianceMapSH(renderBackend, shaderRepository, commandList, environmentMapTexture, environmentMapTextureSize, irradianceEnvironmentMapBuffer);

        const uint32 environmentMapTextureMipLevelCount = Math::MaxMipLevelCount(environmentMapTextureSize);
        ConvolveEnvironmentMap(renderBackend, shaderRepository, commandList, environmentMapTexture, environmentMapTextureMipLevelCount, convolvedEnvironmentMapTexture);
    }
}