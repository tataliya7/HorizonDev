#include "RasterizationRenderer.h"

namespace Horizon
{
    Matrix4x4f ComputeRotationMatrixForCubeFace(uint32 faceIndex)
    {
        // Basis
        Vector3f up = Vector3f(0.0f, 0.0f, 1.0f);
        Vector3f right = Vector3f(1.0f, 0.0f, 0.0f);
        Vector3f forward = Vector3f(1.0f, 0.0f, 0.0f);

        switch (faceIndex)
        {
        case 0: // X+
            up = Vector3f(0.0f, 0.0f, 1.0f);
            right = Vector3f(1.0f, 0.0f, 0.0f);
            forward = Vector3f(1.0f, 0.0f, 0.0f);
            break;
        case 1: // X-
            up = Vector3f(0.0f, 0.0f, 1.0f);
            right = Vector3f(1.0f, 0.0f, 0.0f);
            forward = Vector3f(-1.0f, 0.0f, 0.0f);
            break;
        case 2: // Y+
            up = Vector3f(0.0f, 0.0f, 1.0f);
            right = Vector3f(1.0f, 0.0f, 0.0f);
            forward = Vector3f(0.0f, 1.0f, 0.0f);
            break;
        case 3: // Y-
            up = Vector3f(0.0f, 0.0f, 1.0f);
            right = Vector3f(1.0f, 0.0f, 0.0f);
            forward = Vector3f(0.0f, -1.0f, 0.0f);
            break;
        case 4: // Z+
            up = Vector3f(0.0f, 0.0f, 1.0f);
            right = Vector3f(1.0f, 0.0f, 0.0f);
            forward = Vector3f(0.0f, -1.0f, 0.0f);
            break;
        case 5: // Z-
            up = Vector3f(0.0f, 0.0f, 1.0f);
            right = Vector3f(1.0f, 0.0f, 0.0f);
            forward = Vector3f(0.0f, -1.0f, 0.0f);
            break;
        default:
            std::unreachable();
            break;
        }

        // RH
        Matrix4x4f result;
        result[0][0] = right.x;
        result[1][0] = right.y;
        result[2][0] = right.z;
        result[3][0] = 0.0f;
        result[0][1] = up.x;
        result[1][1] = up.y;
        result[2][1] = up.z;
        result[3][1] = 0.0f;
        result[0][2] = -forward.x;
        result[1][2] = -forward.y;
        result[2][2] = -forward.z;
        result[3][2] = 0.0f;
        result[3][0] = 0.0f;
        result[3][1] = 0.0f;
        result[3][2] = 0.0f;
        result[3][3] = 1.0f;

        return result;
    }

    void RasterizationRenderer::CaptureEnvironmentMap(
            RenderGraph& renderGraph,
            const SceneView& view)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "CaptureEnvironmentMap");

        SkyLightRenderObject* skyLight = view.scene->GetActiveSkyLight();

        if (!skyLight)
        {
            return;
        }

        const uint32 environmentMapTextureSize = skyLight->GetCubemapSize();
        const uint32 environmentMapTextureMipLevelCount = Math::MaxMipLevelCount(environmentMapTextureSize);

        RenderGraphTextureDescription capturedEnvironmentMapTextureDesc = RenderGraphTextureDescription::CreateCube(
            environmentMapTextureSize,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            environmentMapTextureMipLevelCount);
        RenderGraphTextureHandle capturedEnvironmentMapTexture = renderGraph.CreateTexture(capturedEnvironmentMapTextureDesc, "CapturedEnvironmentMapTexture");

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        if (renderFeatures.enableSkyAtmosphereRendering)
        {
            for (uint32 faceIndex = 0; faceIndex < 6; faceIndex++)
            {
                const Matrix4x4f rotationMatrix = ComputeRotationMatrixForCubeFace(faceIndex);

                renderGraph.AddPass(
                    std::format("CaptureEnvironmentMap (Compute, Size={}, Face={})", environmentMapTextureSize, faceIndex),
                    RenderGraphPassFlags::Compute,
                    [&](RenderGraphBuilder& builder)
                    {
                        capturedEnvironmentMapTexture = builder.WriteTexture(capturedEnvironmentMapTexture, RenderBackendResourceState::UnorderedAccess);

                        return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                        {
                            //RenderBackendTextureUAVDesc capturedEnvironmentMapTextureUAV = RenderBackendTextureUAVDesc::Create(resourceRegistry.GetRenderBackendTextureHandle(capturedEnvironmentMapTexture), 0);
                            //commandList.ClearTextureUAV(capturedEnvironmentMapTextureUAV, RenderBackendTextureClearValue::Black);
                        };
                    });
            }
        }
        else
        {
            renderGraph.AddPass(
                std::format("ClearEnvironmentMap (Compute)"),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    capturedEnvironmentMapTexture = builder.WriteTexture(capturedEnvironmentMapTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendTextureUAVDesc capturedEnvironmentMapTextureUAV = RenderBackendTextureUAVDesc::Create(resourceRegistry.GetRenderBackendTextureHandle(capturedEnvironmentMapTexture), 0);
                        commandList.ClearTextureUAV(capturedEnvironmentMapTextureUAV, RenderBackendTextureClearValue::Black);
                    };
                });
        }

        renderGraph.AddPass(
            std::format("GenerateEnvironmentMipmaps (Compute, MipLevelCount={})", environmentMapTextureMipLevelCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                capturedEnvironmentMapTexture = builder.WriteTexture(capturedEnvironmentMapTexture, RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::DownsampleCubemap);

                    for (uint32 mipLevel = 1; mipLevel < environmentMapTextureMipLevelCount; mipLevel++)
                    {
                        RenderBackendBarrier transitions = RenderBackendBarrier(
                            resourceRegistry.GetRenderBackendTextureHandle(capturedEnvironmentMapTexture),
                            RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers),
                            RenderBackendResourceState::UnorderedAccess,
                            RenderBackendResourceState::ShaderResource);
                        commandList.Barriers(&transitions, 1);

                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(1 << (environmentMapTextureMipLevelCount - mipLevel - 1), 8);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(1 << (environmentMapTextureMipLevelCount - mipLevel - 1), 8);
                        uint32 threadGroupCountZ = 1;

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(capturedEnvironmentMapTexture));
                        pushConstantValues.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(capturedEnvironmentMapTexture, mipLevel));
                        pushConstantValues.OverrideShaderConstantValue(2, mipLevel - 1);

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }

                    RenderBackendBarrier transition = RenderBackendBarrier(
                        resourceRegistry.GetRenderBackendTextureHandle(capturedEnvironmentMapTexture),
                        RenderBackendTextureSubresourceRange(environmentMapTextureMipLevelCount - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers),
                        RenderBackendResourceState::UnorderedAccess,
                        RenderBackendResourceState::ShaderResource);
                    commandList.Barriers(&transition, 1);
                };
            });

        // test
        capturedEnvironmentMapTexture = renderGraph.ImportExternalTexture(skyLight->environmentMapTexture, "TestEnvironmentMapTexture");

        intermediateResources.environmentMapTexture = capturedEnvironmentMapTexture;

        uint32 sampleCount = 64;

        RenderGraphBufferDescription irradianceEnvironmentMapBufferDesc = RenderGraphBufferDescription::CreateByteAddress(sizeof(float) * 9 * 3);
        RenderGraphBufferHandle irradianceEnvironmentMapBuffer = renderGraph.CreateBuffer(irradianceEnvironmentMapBufferDesc, "IrradianceEnvironmentMapBuffer");

        renderGraph.AddPass(
            std::format("GenerateIrradianceEnvironmentMap (Compute, SampleCount={})", sampleCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                capturedEnvironmentMapTexture = builder.ReadTexture(capturedEnvironmentMapTexture, RenderBackendResourceState::ShaderResource);
                irradianceEnvironmentMapBuffer = builder.WriteBuffer(irradianceEnvironmentMapBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    constexpr uint32 log2_16 = 4;
                    uint32 sourceMipLevel = uint32(std::log2(float(environmentMapTextureSize))) - log2_16;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(capturedEnvironmentMapTexture));
                    pushConstantValues.BindBufferUAV(1, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(irradianceEnvironmentMapBuffer));
                    pushConstantValues.OverrideShaderConstantValue(2, sourceMipLevel);

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::IrradianceEnvironmentMapSHOnePass);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        1,
                        1,
                        1);
                };
            });

        RenderGraphTextureHandle convolvedEnvironmentMapTexture = renderGraph.CreateTexture(capturedEnvironmentMapTextureDesc, "ConvolvedEnvironmentMapTexture");

        renderGraph.AddPass(
            std::format("ConvolveEnvironmentMap (Compute, SampleCount={})", sampleCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                capturedEnvironmentMapTexture = builder.ReadTexture(capturedEnvironmentMapTexture, RenderBackendResourceState::ShaderResource);
                convolvedEnvironmentMapTexture = builder.WriteTexture(convolvedEnvironmentMapTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::EnvironmentMapConvolution);

                    for (uint32 targetMipLevel = 0; targetMipLevel < environmentMapTextureMipLevelCount; targetMipLevel++)
                    {
                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(1 << (environmentMapTextureMipLevelCount - targetMipLevel - 1), 8);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(1 << (environmentMapTextureMipLevelCount - targetMipLevel - 1), 8);
                        uint32 threadGroupCountZ = 1;

                        float roughness = float(targetMipLevel) / float(environmentMapTextureMipLevelCount - 1);

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(capturedEnvironmentMapTexture));
                        pushConstantValues.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(convolvedEnvironmentMapTexture, targetMipLevel));
                        pushConstantValues.OverrideShaderConstantValue(2, roughness);

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }
                };
            });

        intermediateResources.irradianceEnvironmentMapBuffer = irradianceEnvironmentMapBuffer;
        intermediateResources.convolvedEnvironmentMapTexture = convolvedEnvironmentMapTexture;
    }
}