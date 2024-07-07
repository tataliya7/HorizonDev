#include "ImageBasedLighting.h"
#include "RenderUtils.h"
#include "ShaderLibrary.h"

namespace Horizon
{
    uint32 GPreIntegratedBrdfLutSize = 256;
    uint32 GIrradianceEnvironmentMapSize = 32;

    void RenderPreIntegratedBrdfLut(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle preIntegratedBrdfLut)
    {
        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::PreIntegratedBRDF);

        RenderBackendBarrier transition(preIntegratedBrdfLut, RenderBackendTextureSubresourceRange::All, RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Transitions(&transition, 1);

        uint32 threadGroupCountX = CeilDiv(GPreIntegratedBrdfLutSize, 8);
        uint32 threadGroupCountY = CeilDiv(GPreIntegratedBrdfLutSize, 8);
        uint32 threadGroupCountZ = 1;

        RenderBackendShaderConstants shaderConstants = {};
        shaderConstants.BindTextureUAV(0, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(preIntegratedBrdfLut, 0));

        commandList.Dispatch(
            computeShader,
            shaderConstants,
            threadGroupCountX,
            threadGroupCountY,
            threadGroupCountZ);

        transition = RenderBackendBarrier(preIntegratedBrdfLut, RenderBackendTextureSubresourceRange::All, RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Transitions(&transition, 1);
    }

    void GenerateCubemapMips(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle cubemap, uint32 numMipLevels)
    {
        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::DownsampleCubemap);

        for (uint32 mipLevel = 1; mipLevel < numMipLevels; mipLevel++)
        {
            RenderBackendBarrier transitions[] =
            {
                RenderBackendBarrier(cubemap, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                RenderBackendBarrier(cubemap, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
            };
            commandList.Transitions(transitions, 2);

            uint32 threadGroupCountX = CeilDiv(1 << (numMipLevels - mipLevel - 1), 8);
            uint32 threadGroupCountY = CeilDiv(1 << (numMipLevels - mipLevel - 1), 8);
            uint32 threadGroupCountZ = 1;

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(cubemap));
            shaderConstants.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(cubemap, mipLevel));
            shaderConstants.BindScalar(2, mipLevel - 1u);

            commandList.Dispatch(
                computeShader,
                shaderConstants,
                threadGroupCountX,
                threadGroupCountY,
                threadGroupCountZ);
        }

        RenderBackendBarrier transition = RenderBackendBarrier(cubemap, RenderBackendTextureSubresourceRange(numMipLevels - 1, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Transitions(&transition, 1);
    }

    void ComputeEnvironmentIrradiance(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 mipLevel, RenderBackendTextureHandle irradianceEnvironmentMap)
    {
        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ComputeEnvironmentIrradiance);

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

    void ComputeEnvironmentIrradianceSH(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 environmentMapSize, RenderBackendBufferHandle irradianceEnvironmentMapSH)
    {
        static const uint32 log2_16 = 4;
        uint32 sourceMipLevel = uint32(std::log2(float(environmentMapSize))) - log2_16;

        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ComputeEnvironmentIrradianceSH);

        RenderBackendShaderConstants shaderConstants = {};
        shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
        shaderConstants.BindBufferUAV(1, renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapSH));
        shaderConstants.BindScalar(2, sourceMipLevel);

        commandList.Dispatch(
            computeShader,
            shaderConstants,
            1,
            1,
            1);
    }

    void FilterEnvironmentMap(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 numMipLevels, RenderBackendTextureHandle filteredEnvironmentMap)
    {
        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::FilterEnvironmentMap);

        RenderBackendBarrier transition(filteredEnvironmentMap, RenderBackendTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
        commandList.Transitions(&transition, 1);

        for (uint32 mipLevel = 0; mipLevel < numMipLevels; mipLevel++)
        {
            uint32 threadGroupCountX = CeilDiv(1 << (numMipLevels - mipLevel - 1), 8);
            uint32 threadGroupCountY = CeilDiv(1 << (numMipLevels - mipLevel - 1), 8);
            uint32 threadGroupCountZ = 1;

            float roughness = (float)mipLevel / (float)(numMipLevels - 1);

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentMap));
            shaderConstants.BindTextureUAV(1, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(filteredEnvironmentMap, mipLevel));
            shaderConstants.BindScalar(2, roughness);

            commandList.Dispatch(
                computeShader,
                shaderConstants,
                threadGroupCountX,
                threadGroupCountY,
                threadGroupCountZ);
        }

        transition = RenderBackendBarrier(filteredEnvironmentMap, RenderBackendTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
        commandList.Transitions(&transition, 1);
    }

    void ComputeEnvironmentCubemaps(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle environmentMap, uint32 cubemapSize, RenderBackendTextureHandle irradianceEnvironmentMap, RenderBackendBufferHandle irradianceEnvironmentMapSH, RenderBackendTextureHandle filteredEnvironmentMap)
    {
        const uint32 numMipLevels = Math::MaxNumMipLevels(cubemapSize);

        GenerateCubemapMips(renderBackend, shaderLibrary, commandList, environmentMap, numMipLevels);

        const uint32 numIrradianceEnvironmentMapMipLevels = Math::MaxNumMipLevels(GIrradianceEnvironmentMapSize);
        const uint32 sourceMipLevel = Math::Max<uint32>(0, numMipLevels - numIrradianceEnvironmentMapMipLevels);

        ComputeEnvironmentIrradiance(renderBackend, shaderLibrary, commandList, environmentMap, sourceMipLevel, irradianceEnvironmentMap);

        ComputeEnvironmentIrradianceSH(renderBackend, shaderLibrary, commandList, environmentMap, cubemapSize, irradianceEnvironmentMapSH);

        FilterEnvironmentMap(renderBackend, shaderLibrary, commandList, environmentMap, numMipLevels, filteredEnvironmentMap);
    }

    void ConvertLatLongToCubemap(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle latLongTexture, RenderBackendTextureHandle cubemapTexture, uint32 cubemapTextureSize)
    {
        RenderBackendBarrier transition(cubemapTexture, RenderBackendTextureSubresourceRange(0, 1, 0, 6), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess);
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

    // void RenderSystem::UpdateSkyLight(EnvironmentLightComponent& skyLight)
    // {
    //     uint32 deviceMask = ~0u;
    //     uint32 cubemapSize = skyLight.GetCubemapResolution();
    //     RenderBackendTextureHandle equirectangular = LoadTextureFromHDRFile(renderBackend, skyLight.GetCubemap().c_str());
    //     RenderBackendTextureDesc cubemapDesc = RenderBackendTextureDesc::CreateCube(
    //         cubemapSize,
    //         RenderBackendTextureFormat::RGBA16Float,
    //         RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource,
    //         Math::MaxNumMipLevels(cubemapSize));
    //     skyLight.environmentMap = renderBackend->CreateTexture(deviceMask, &cubemapDesc, nullptr, "EnvironmentMap");
    //
    //     RenderBackendTextureDesc irradianceEnvironmentMapDesc = RenderBackendTextureDesc::CreateCube(
    //         GIrradianceEnvironmentMapSize,
    //         RenderBackendTextureFormat::RGBA16Float,
    //         RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
    //     skyLight.irradianceEnvironmentMap = renderBackend->CreateTexture(deviceMask, &irradianceEnvironmentMapDesc, nullptr, "IrradianceEnvironmentMap");
    //
    //     RenderBackendBufferDesc irradianceEnvironmentMapSHDesc = RenderBackendBufferDesc::CreateByteAddress(sizeof(float) * 27);
    //     skyLight.irradianceEnvironmentMapSH = renderBackend->CreateBuffer(deviceMask, &irradianceEnvironmentMapSHDesc, nullptr, "IrradianceEnvironmentMapSH");
    //
    //     skyLight.filteredEnvironmentMap = renderBackend->CreateTexture(deviceMask, &cubemapDesc, nullptr, "FilteredEnvironmentMap");
    //
    //     RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);
    //
    //     ConvertLatLongToCubemap(shaderLibrary, *commandList, equirectangular, skyLight.environmentMap, cubemapSize);
    //
    //     ComputeEnvironmentCubemaps(shaderLibrary, *commandList, skyLight.environmentMap, cubemapSize, skyLight.irradianceEnvironmentMap, skyLight.irradianceEnvironmentMapSH, skyLight.filteredEnvironmentMap);
    //
    //     renderBackend->SubmitCommandLists(&commandList, 1, RenderBackendSwapChainHandle::Null);
    //
    //     delete commandList;
    //
    //     skyLight.SetDirty(false);
    // }
}