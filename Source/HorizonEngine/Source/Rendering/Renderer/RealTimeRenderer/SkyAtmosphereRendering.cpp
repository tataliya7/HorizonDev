#include "SkyAtmosphereRendering.h"
#include "RealTimeRenderer.h"

namespace Horizon
{
    // Sky Atmosphere Rendering Configs
    // TODO: Make it changeable in flight.
    uint32 TransmittanceLutWidth_Deprecated = 256;
    uint32 TransmittanceLutHeight_Deprecated = 64;
    uint32 MultipleScatteringLutWidth_Deprecated = 32;
    uint32 MultipleScatteringLutHeight_Deprecated = 32;
    uint32 SkyViewLutWidth_Deprecated = 192;
    uint32 SkyViewLutHeight_Deprecated = 104;
    uint32 AerialPerspectiveVolumeSize_Deprecated = 32;
    uint32 TransmittanceLutSampleCount_Deprecated = 40; // Can go a low as 10 sample but energy lost starts to be visible
    uint32 MultipleScatteringLutSampleCount_Deprecated = 20; // a minimum set of step is required for accuracy unfortunately
    uint32 RayMarchingMinSampleCount_Deprecated = 4;
    uint32 RayMarchingMaxSampleCount_Deprecated = 32;

    void SetupSkyAtmosphereShaderParameters(SkyAtmosphereShaderParameters& outParameters, const SkyAtmosphereRenderProxy& renderProxy)
    {
        outParameters.transmittanceLutSize = GetSizeAndInverseSize(TransmittanceLutWidth_Deprecated, TransmittanceLutHeight_Deprecated);
        outParameters.multipleScatteringLutSize = GetSizeAndInverseSize(MultipleScatteringLutWidth_Deprecated, MultipleScatteringLutHeight_Deprecated);
        outParameters.skyViewLutSize = GetSizeAndInverseSize(SkyViewLutWidth_Deprecated, SkyViewLutHeight_Deprecated);
        outParameters.aerialPerspectiveVolumeSize = Vector2(float(AerialPerspectiveVolumeSize_Deprecated), 1.0f / float(AerialPerspectiveVolumeSize_Deprecated));
        outParameters.transmittanceLutSampleCount = float(TransmittanceLutSampleCount_Deprecated);
        outParameters.multipleScatteringLutSampleCount = float(MultipleScatteringLutSampleCount_Deprecated);
        outParameters.rayMarchingMinSampleCount = float(RayMarchingMinSampleCount_Deprecated);
        outParameters.rayMarchingMaxSampleCount = float(RayMarchingMaxSampleCount_Deprecated);

        const AtmosphereParameters& atmosphereParameters = renderProxy.GetAtmosphereParameters();
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
        b2 = Vector3f(b, sign + n.y * n.y* a, -n.y);
    }

    void SetupSkyAtmosphereViewRelatedParameters(SkyAtmosphereViewRelatedParameters& outParameters, const SkyAtmosphereRenderProxy& renderProxy, const Vector3& worldSpaceCameraPosition, const Vector3& cameraForwardVector)
    {
        const AtmosphereParameters& atmosphereParameters = renderProxy.GetAtmosphereParameters();
        float bottomRadiusKm = atmosphereParameters.bottomRadius;

        Vector3 worldSpacePlanetCenterKm = Vector3(0.0f, 0.0f, -bottomRadiusKm);
        Vector3 worldSpacePlanetCenter = worldSpacePlanetCenterKm * KilometersToMeters;

        Vector3 planetCenterToCameraPositionKm = (worldSpaceCameraPosition - worldSpacePlanetCenter) * MetersToKilometers;

        Vector3 upVector = Math::Normalize(planetCenterToCameraPositionKm);
        Vector3 forwardVector = cameraForwardVector;
        Vector3 leftVector = Math::Normalize(Math::CrossProduct(forwardVector, upVector));

        if (std::abs(Math::DotProduct(upVector, forwardVector)) > 0.999f)
        {
            BuildOrthonormalBasisBranchless(upVector, forwardVector, leftVector);
        }
        else
        {
            forwardVector = Math::Normalize(Math::CrossProduct(upVector, leftVector));
        }

        outParameters.skyViewLutReferential = Matrix3x3(forwardVector, leftVector, upVector);
    }

    bool RealTimeRenderer::IsSkyAtmosphereRenderingEnabled() const
    {
        if (scene && scene->HasSkyAtmosphere())
        {
            return true;
        }
        return false;
    }

    void RealTimeRenderer::RenderSkyAtmosphereLUTs(RenderGraph& renderGraph)
    {
        const uint32 transmittanceLutWidth = TransmittanceLutWidth_Deprecated;
        const uint32 transmittanceLutHeight = TransmittanceLutHeight_Deprecated;
        const uint32 multipleScatteringLutWidth = MultipleScatteringLutWidth_Deprecated;
        const uint32 multipleScatteringLutHeight = MultipleScatteringLutHeight_Deprecated;
        const uint32 skyViewLutWidth = SkyViewLutWidth_Deprecated;
        const uint32 skyViewLutHeight = SkyViewLutHeight_Deprecated;
        const uint32 aerialPerspectiveVolumeSize = AerialPerspectiveVolumeSize_Deprecated;

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
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        RenderGraphTextureHandle aerialPerspectiveVolume = renderGraph.CreateTexture(aerialPerspectiveVolumeDesc, "SkyAtmosphereAerialPerspectiveVolume");

        renderGraph.AddPass(std::format("SkyAtmosphereTransmittanceLut (Compute, {}x{})", transmittanceLutWidth, transmittanceLutHeight), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                transmittanceLut = builder.WriteTexture(transmittanceLut, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 groupCountX = ComputeWorkGroupCount(transmittanceLutWidth, 8);
                        uint32 groupCountY = ComputeWorkGroupCount(transmittanceLutHeight, 8);
                        uint32 groupCountZ = 1;

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBufferSRV(0, GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(transmittanceLut)));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SkyAtmosphereTransmittanceLut);
                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader<SkyAtmosphereTransmittanceLutCS>();

                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            groupCountX,
                            groupCountY,
                            groupCountZ);
                    };
            });

        renderGraph.AddPass(std::format("SkyAtmosphereMultipleScatteringLut (Compute, {}x{})", multipleScatteringLutWidth, multipleScatteringLutHeight), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                transmittanceLut = builder.ReadTexture(transmittanceLut, RenderBackendResourceState::ShaderResource);
                multipleScatteringLut = builder.WriteTexture(multipleScatteringLut, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 groupCountX = multipleScatteringLutWidth;
                        uint32 groupCountY = multipleScatteringLutHeight;
                        uint32 groupCountZ = 1;

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBufferSRV(0, GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(transmittanceLut)));
                        shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(multipleScatteringLut)));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SkyAtmosphereMultipleScatteringLut);

                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            groupCountX,
                            groupCountY,
                            groupCountZ);
                    };
            });

        renderGraph.AddPass(std::format("SkyAtmosphereSkyViewLut (Compute, {}x{})", skyViewLutWidth, skyViewLutHeight), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
                {
                    transmittanceLut = builder.ReadTexture(transmittanceLut, RenderBackendResourceState::ShaderResource);
                    multipleScatteringLut = builder.ReadTexture(multipleScatteringLut, RenderBackendResourceState::ShaderResource);
                    skyViewLut = builder.WriteTexture(skyViewLut, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {
                            uint32 groupCountX = ComputeWorkGroupCount(skyViewLutWidth, 8);
                            uint32 groupCountY = ComputeWorkGroupCount(skyViewLutHeight, 8);
                            uint32 groupCountZ = 1;

                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindBufferSRV(0, GetCurrentPerFrameDataBuffer());
                            shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(transmittanceLut)));
                            shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(multipleScatteringLut)));
                            shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(skyViewLut)));

                            RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SkyAtmosphereSkyViewLut);

                            commandList.Dispatch(
                                computeShader,
                                shaderArguments,
                                groupCountX,
                                groupCountY,
                                groupCountZ);
                        };
                });

        renderGraph.AddPass(std::format("SkyAtmosphereAerialPerspectiveVolume (Compute, {}x{}x{})", aerialPerspectiveVolumeSize, aerialPerspectiveVolumeSize, aerialPerspectiveVolumeSize), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                transmittanceLut = builder.ReadTexture(transmittanceLut, RenderBackendResourceState::ShaderResource);
                multipleScatteringLut = builder.ReadTexture(multipleScatteringLut, RenderBackendResourceState::ShaderResource);
                aerialPerspectiveVolume = builder.WriteTexture(aerialPerspectiveVolume, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 groupCountX = aerialPerspectiveVolumeSize / 4;
                        uint32 groupCountY = aerialPerspectiveVolumeSize / 4;
                        uint32 groupCountZ = aerialPerspectiveVolumeSize / 4;

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBufferSRV(0, GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(transmittanceLut)));
                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(multipleScatteringLut)));
                        shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(aerialPerspectiveVolume)));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SkyAtmosphereAerialPerspectiveVolume);

                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            groupCountX,
                            groupCountY,
                            groupCountZ);
                    };
            });

        RealTimeRendererSkyAtmosphereLUTs& skyAtmosphereLUTs = renderGraph.blackboard.Create<RealTimeRendererSkyAtmosphereLUTs>();
        skyAtmosphereLUTs.transmittanceLut = transmittanceLut;
        skyAtmosphereLUTs.multipleScatteringLut = multipleScatteringLut;
        skyAtmosphereLUTs.skyViewLut = skyViewLut;
        skyAtmosphereLUTs.aerialPerspectiveVolume = aerialPerspectiveVolume;
    }

    void RealTimeRenderer::RenderSkyAtmosphere(RenderGraph& renderGraph)
    {
        renderGraph.AddPass(std::format("SkyAtmosphereRayMarching (Graphics, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                RealTimeRendererSkyAtmosphereLUTs& skyAtmosphereLUTs = renderGraph.blackboard.Get<RealTimeRendererSkyAtmosphereLUTs>();

                RenderGraphTextureHandle skyViewLut = builder.ReadTexture(skyAtmosphereLUTs.skyViewLut, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle transmittanceLut = builder.ReadTexture(skyAtmosphereLUTs.transmittanceLut, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle aerialPerspectiveVolume = builder.ReadTexture(skyAtmosphereLUTs.aerialPerspectiveVolume, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
                RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
                builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                        graphicsPipelineState.depthStencilState.depthTestEnable = true;
                        graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::Equal;
                        graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                        graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                        graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
                        graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::SrcAlpha;
                        graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                        graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGB;

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBufferSRV(0, GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(transmittanceLut)));
                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(skyViewLut)));
                        shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(aerialPerspectiveVolume)));
                        shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));

                        RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::SkyAtmosphereRayMarching);

                        commandList.Draw(
                            graphicsShader,
                            graphicsPipelineState,
                            shaderArguments,
                            3, 1, 0, 0,
                            RenderBackendPrimitiveTopology::TriangleList);
                    };
            });
    }

    bool RealTimeRenderer::IsSkyAtmosphereDebugVisualizationEnabled() const
    {
        // TODO
        return false;
    }

    void RealTimeRenderer::AddSkyAtmosphereDebugVisualizationPass()
    {
        // TODO
    }
}