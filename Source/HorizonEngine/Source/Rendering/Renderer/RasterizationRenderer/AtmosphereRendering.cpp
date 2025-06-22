#include "AtmosphereRendering.h"
#include "RasterizationRenderer.h"

namespace Horizon
{
    // Configurations
    // TODO: Make it changeable in flight.
    uint32 TransmittanceLutWidth = 256;
    uint32 TransmittanceLutHeight = 64;
    uint32 MultipleScatteringLutWidth = 32;
    uint32 MultipleScatteringLutHeight = 32;
    uint32 SkyViewLutWidth = 192;
    uint32 SkyViewLutHeight = 104;
    uint32 AerialPerspectiveVolumeSize = 32;
    uint32 TransmittanceLutSampleCount = 40; // Can go a low as 10 sample but energy lost starts to be visible
    uint32 MultipleScatteringLutSampleCount = 20; // a minimum set of step is required for accuracy unfortunately
    uint32 RayMarchingMinSampleCount = 4;
    uint32 RayMarchingMaxSampleCount = 32;

    void SetupSkyAtmosphereShaderParameters(SkyAtmosphereShaderParameters& outParameters, const SkyAtmosphereRenderObject& renderObject)
    {
        outParameters.transmittanceLutSize = GetSizeAndInverseSize(TransmittanceLutWidth, TransmittanceLutHeight);
        outParameters.multipleScatteringLutSize = GetSizeAndInverseSize(MultipleScatteringLutWidth, MultipleScatteringLutHeight);
        outParameters.skyViewLutSize = GetSizeAndInverseSize(SkyViewLutWidth, SkyViewLutHeight);
        outParameters.aerialPerspectiveVolumeSize = Vector2f(float(AerialPerspectiveVolumeSize), 1.0f / float(AerialPerspectiveVolumeSize));
        outParameters.transmittanceLutSampleCount = float(TransmittanceLutSampleCount);
        outParameters.multipleScatteringLutSampleCount = float(MultipleScatteringLutSampleCount);
        outParameters.rayMarchingMinSampleCount = float(RayMarchingMinSampleCount);
        outParameters.rayMarchingMaxSampleCount = float(RayMarchingMaxSampleCount);

        const AtmosphereParameters& atmosphereParameters = renderObject.GetAtmosphereParameters();
        outParameters.bottomRadius = atmosphereParameters.bottomRadius;
        outParameters.topRadius = atmosphereParameters.topRadius;
        outParameters.groundAlbedo = atmosphereParameters.groundAlbedo;
        outParameters.rayleighScattering = atmosphereParameters.rayleighScattering;
        outParameters.rayleighDensityExpScale = atmosphereParameters.rayleighDensityExpScale;
        outParameters.mieScattering = atmosphereParameters.mieScattering;
        outParameters.mieAbsorption = atmosphereParameters.mieAbsorption;
        outParameters.mieExtinction = atmosphereParameters.mieExtinction;
        outParameters.miePhaseG = atmosphereParameters.miePhaseG;
        outParameters.mieDensityExpScale = atmosphereParameters.mieDensityExpScale;
        outParameters.absorptionDensity0LayerWidth = atmosphereParameters.absorptionDensity0LayerWidth;
        outParameters.absorptionDensity0ConstantTerm = atmosphereParameters.absorptionDensity0ConstantTerm;
        outParameters.absorptionDensity0LinearTerm = atmosphereParameters.absorptionDensity0LinearTerm;
        outParameters.absorptionDensity1ConstantTerm = atmosphereParameters.absorptionDensity1ConstantTerm;
        outParameters.absorptionDensity1LinearTerm = atmosphereParameters.absorptionDensity1LinearTerm;
        outParameters.absorptionExtinction = atmosphereParameters.absorptionExtinction;
    }

    /**
     * Building an orthonormal basis from a 3D unit vector without branch.
     *
     * @see Tom Duff, James Burgess, Per Christensen, Christophe Hery, Andrew Kensler, Max Liani, and Ryusuke Villemin, Building an Orthonormal Basis, Revisited, Journal of Computer Graphics Techniques (JCGT), vol. 6, no. 1, 1-8, 2017
     */
    static void BuildOrthonormalBasisBranchless(const Vector3f& n, Vector3f& b1, Vector3f& b2)
    {
        const float sign = copysignf(1.0f, n.z); // Using copysignf to eliminate the test
        //const float sign = n.z >= 0.0f ? 1.0f : -1.0f; // Explicitly writing
        const float a = -1.0f / (sign + n.z);
        const float b = n.x * n.y * a;
        b1 = Vector3f(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
        b2 = Vector3f(b, sign + n.y * n.y * a, -n.y);
    }

    void SetupSkyAtmosphereViewRelatedParameters(SkyAtmosphereViewRelatedParameters& outParameters, const SkyAtmosphereRenderObject& renderObject, const Vector3f& worldSpaceCameraPosition, const Vector3f& cameraForwardVector)
    {
        const AtmosphereParameters& atmosphereParameters = renderObject.GetAtmosphereParameters();
        float bottomRadiusKm = atmosphereParameters.bottomRadius;

        Vector3f worldSpacePlanetCenterKm = Vector3f(0.0f, 0.0f, -bottomRadiusKm);
        Vector3f worldSpacePlanetCenter = worldSpacePlanetCenterKm * KilometersToMeters;

        Vector3f planetCenterToCameraPositionKm = (worldSpaceCameraPosition - worldSpacePlanetCenter) * MetersToKilometers;

        Vector3f forwardVector = cameraForwardVector;
        Vector3f upVector = Math::Normalize(planetCenterToCameraPositionKm);
        Vector3f rightVector = Math::Normalize(Math::CrossProduct(forwardVector, upVector));

        if (std::abs(Math::DotProduct(upVector, forwardVector)) > 0.999f)
        {
            BuildOrthonormalBasisBranchless(upVector, forwardVector, rightVector);
        }
        else
        {
            forwardVector = Math::Normalize(Math::CrossProduct(upVector, rightVector));
        }

        outParameters.skyViewLutReferential = Matrix3x3f(forwardVector, rightVector, upVector);
    }

    bool RasterizationRenderer::IsSkyAtmosphereRenderingEnabled() const
    {
        return renderFeatures.enableSkyAtmosphereRendering;
    }

    void RasterizationRenderer::RenderSkyAtmosphereLUTs(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "RenderSkyAtmosphereLUTs");

        const uint32 transmittanceLutWidth = TransmittanceLutWidth;
        const uint32 transmittanceLutHeight = TransmittanceLutHeight;
        const uint32 multipleScatteringLutWidth = MultipleScatteringLutWidth;
        const uint32 multipleScatteringLutHeight = MultipleScatteringLutHeight;
        const uint32 skyViewLutWidth = SkyViewLutWidth;
        const uint32 skyViewLutHeight = SkyViewLutHeight;
        const uint32 aerialPerspectiveVolumeSize = AerialPerspectiveVolumeSize;

        RenderBackendTextureDesc transmittanceLutDesc = RenderBackendTextureDesc::Create2D(
            transmittanceLutWidth,
            transmittanceLutHeight,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        RenderGraphTextureHandle transmittanceLut = renderGraph.CreateTexture(transmittanceLutDesc, "SkyAtmosphereTransmittanceLut");

        RenderBackendTextureDesc multipleScatteringLutDesc = RenderBackendTextureDesc::Create2D(
            multipleScatteringLutWidth,
            multipleScatteringLutHeight,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        RenderGraphTextureHandle multipleScatteringLut = renderGraph.CreateTexture(multipleScatteringLutDesc, "SkyAtmosphereMultipleScatteringLut");

        RenderBackendTextureDesc skyViewLutDesc = RenderBackendTextureDesc::Create2D(
            skyViewLutWidth,
            skyViewLutHeight,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        RenderGraphTextureHandle skyViewLut = renderGraph.CreateTexture(skyViewLutDesc, "SkyAtmosphereSkyViewLut");

        RenderBackendTextureDesc aerialPerspectiveVolumeDesc = RenderBackendTextureDesc::Create3D(
            aerialPerspectiveVolumeSize,
            aerialPerspectiveVolumeSize,
            aerialPerspectiveVolumeSize,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        RenderGraphTextureHandle aerialPerspectiveVolume = renderGraph.CreateTexture(aerialPerspectiveVolumeDesc, "SkyAtmosphereAerialPerspectiveVolume");

        renderGraph.AddPass(
            std::format("SkyAtmosphereTransmittanceLut (Compute, {}x{})", transmittanceLutWidth, transmittanceLutHeight),
            RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                transmittanceLut = builder.WriteTexture(transmittanceLut, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(transmittanceLutWidth, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(transmittanceLutHeight, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(transmittanceLut, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SkyAtmosphereTransmittanceLut);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("SkyAtmosphereMultipleScatteringLut (Compute, {}x{})", multipleScatteringLutWidth, multipleScatteringLutHeight),
            RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                transmittanceLut = builder.ReadTexture(transmittanceLut, RenderBackendResourceState::ShaderResource);
                multipleScatteringLut = builder.WriteTexture(multipleScatteringLut, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = multipleScatteringLutWidth;
                    uint32 threadGroupCountY = multipleScatteringLutHeight;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(transmittanceLut));
                    shaderConstants.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(multipleScatteringLut, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SkyAtmosphereMultipleScatteringLut);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("SkyAtmosphereSkyViewLut (Compute, {}x{})", skyViewLutWidth, skyViewLutHeight),
            RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                transmittanceLut = builder.ReadTexture(transmittanceLut, RenderBackendResourceState::ShaderResource);
                multipleScatteringLut = builder.ReadTexture(multipleScatteringLut, RenderBackendResourceState::ShaderResource);
                skyViewLut = builder.WriteTexture(skyViewLut, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(skyViewLutWidth, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(skyViewLutHeight, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(transmittanceLut));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(multipleScatteringLut));
                    shaderConstants.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(skyViewLut, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SkyAtmosphereSkyViewLut);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        assert(aerialPerspectiveVolumeSize % 4 == 0);

        renderGraph.AddPass(
            std::format("SkyAtmosphereAerialPerspectiveVolume (Compute, {}x{}x{})", aerialPerspectiveVolumeSize, aerialPerspectiveVolumeSize, aerialPerspectiveVolumeSize),
            RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                transmittanceLut = builder.ReadTexture(transmittanceLut, RenderBackendResourceState::ShaderResource);
                multipleScatteringLut = builder.ReadTexture(multipleScatteringLut, RenderBackendResourceState::ShaderResource);
                aerialPerspectiveVolume = builder.WriteTexture(aerialPerspectiveVolume, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = aerialPerspectiveVolumeSize / 4;
                    uint32 threadGroupCountY = aerialPerspectiveVolumeSize / 4;
                    uint32 threadGroupCountZ = aerialPerspectiveVolumeSize / 4;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(transmittanceLut));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(multipleScatteringLut));
                    shaderConstants.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(aerialPerspectiveVolume, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SkyAtmosphereAerialPerspectiveVolume);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RasterizationRendererSkyAtmosphereLUTs& skyAtmosphereLUTs = renderGraph.blackboard.Create<RasterizationRendererSkyAtmosphereLUTs>();
        skyAtmosphereLUTs.transmittanceLut = transmittanceLut;
        skyAtmosphereLUTs.multipleScatteringLut = multipleScatteringLut;
        skyAtmosphereLUTs.skyViewLut = skyViewLut;
        skyAtmosphereLUTs.aerialPerspectiveVolume = aerialPerspectiveVolume;
    }

    void RasterizationRenderer::RenderSkyAtmosphere(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        renderGraph.AddPass(
            std::format("SkyAtmosphereRayMarching (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();
                RasterizationRendererSkyAtmosphereLUTs& skyAtmosphereLUTs = renderGraph.blackboard.Get<RasterizationRendererSkyAtmosphereLUTs>();

                RenderGraphTextureHandle skyViewLut = builder.ReadTexture(skyAtmosphereLUTs.skyViewLut, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle transmittanceLut = builder.ReadTexture(skyAtmosphereLUTs.transmittanceLut, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle aerialPerspectiveVolume = builder.ReadTexture(skyAtmosphereLUTs.aerialPerspectiveVolume, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::DepthStencilReadOnly);
                RenderGraphTextureHandle sceneColorTexture = intermediateResources.colorTexture = builder.WriteTexture(intermediateResources.colorTexture, RenderBackendResourceState::RenderTarget);

                builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);
                builder.SetDepthStencilBinding(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Load,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthReadOnly_StencilNoAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::Equal;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::SrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGB;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(transmittanceLut));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(skyViewLut));
                    shaderConstants.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(aerialPerspectiveVolume));
                    shaderConstants.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::SkyAtmosphereRayMarching);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }

    bool RasterizationRenderer::IsSkyAtmosphereDebugVisualizationEnabled() const
    {
        // TODO
        return false;
    }

    void RasterizationRenderer::AddSkyAtmosphereDebugVisualizationPass()
    {
        // TODO
    }
}