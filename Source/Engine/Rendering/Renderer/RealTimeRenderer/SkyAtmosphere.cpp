#include "RealTimeRenderer.h"
#include "SkyAtmosphere.h"

#define HIGH_QUALITY_SKY_ATMOSPHERE

#ifdef HIGH_QUALITY_SKY_ATMOSPHERE
#define SKY_ATMOSPHERE_DEFAULT_LUT_FORMAT RenderBackendTextureFormat::RGBA32Float
#else
#define SKY_ATMOSPHERE_DEFAULT_LUT_FORMAT RenderBackendTextureFormat::RGBA16Float
#endif

namespace HE
{
    struct SkyAtmosphereConstants
    {
        float bottomRadius;
        float topRadius;
        Vector3 groundAlbedo;
        Vector3 rayleighScattering;
        Vector3 mieScattering;
        Vector3 mieExtinction;
        Vector3 mieAbsorption;
        float miePhaseG;
        Vector3 absorptionExtinction;
        float rayleighDensity[12];
        float mieDensity[12];
        float absorptionDensity[12];
        float cosMaxSunZenithAngle;
        float multipleScatteringFactor;
        float rayMarchMinSPP;
        float rayMarchMaxSPP;
        Vector4 transmittanceLutSizeAndInvSize;
        Vector4 multipleScatteringLutSizeAndInvSize;
        Vector4 skyViewLutSizeAndInvSize;
        float aerialPerspectiveVolumeSize;
    };

    struct SkyAtmosphereShaderParameters
    {
        Vector4 transmittanceLutSizeAndInvSize;
        Vector4 multipleScatteringLutSizeAndInvSize;
        Vector4 skyViewLutSizeAndInvSize;
        float aerialPerspectiveVolumeSize;
        float transmittanceLutSampleCount;
        float multipleScatteringLutSampleCount;
        float rayMarchMinSPP;
        float rayMarchMaxSPP;

        // Atmosphere
        float bottomRadius;
        float topRadius;
        Vector3 groundAlbedo;
        Vector3 rayleighScattering;
        float rayleighDensityExpScale;
        Vector3 mieScattering;
        Vector3 mieExtinction;
        Vector3 mieAbsorption;
        float miePhaseG;
        float mieDensityExpScale;
        float absorptionDensity0LayerWidth;
        float absorptionDensity0ConstantTerm;
        float absorptionDensity0LinearTerm;
        float absorptionDensity1ConstantTerm;
        float absorptionDensity1LinearTerm;
        Vector3 absorptionExtinction;
    };

    void SetupEarthAtmosphere(SkyAtmosphereComponent* component)
    {
        // Values shown here are the result of integration over wavelength power spectrum integrated with paricular function.
        // Refer to https://github.com/ebruneton/precomputed_atmospheric_scattering for details.

        // All units in kilometers
        const float earthRadius = 6360.0f;
        //const float earthAtmosphereHeight = 60.0f;   // 100km atmosphere radius, less edge visible and it contain 99.99% of the atmosphere medium https://en.wikipedia.org/wiki/K%C3%A1rm%C3%A1n_line
        const float earthAtmosphereHeight = 100.0;
        const float earthRayleighScaleHeight = 8.0f;
        const float earthMieScaleHeight = 1.2f;

        const double maxSunZenithAngle = M_PI * 120.0 / 180.0;

        // Earth
        component->groundRadius = earthRadius;
        component->groundAlbedo = { 0.401978f, 0.401978f, 0.401978f };
        component->atmosphereHeight = earthAtmosphereHeight;
        component->multipleScatteringFactor = 1.0f;

        // Raleigh
        component->rayleighScattering = { 0.005802f, 0.013558f, 0.033100f }; // 1/km
        component->rayleighScaleHeight = earthRayleighScaleHeight;

        // Mie
        component->mieScattering = { 0.003996f, 0.003996f, 0.003996f }; // 1/km
        component->mieExtinction = { 0.004440f, 0.004440f, 0.004440f }; // 1/km
        component->mieAnisotropy = 0.8f;
        component->mieScaleHeight = earthMieScaleHeight;

        // Absorption
        component->absorptionDensity[0] = { 25.0f, 0.0f, 0.0f, 1.0f / 15.0f, -2.0f / 3.0f };
        component->absorptionDensity[1] = { 0.0f, 0.0f, 0.0f, -1.0f / 15.0f, 8.0f / 3.0f };
        component->absorptionExtinction = { 0.000650f, 0.001881f, 0.000085f }; // 1/km
        component->cosMaxSunZenithAngle = (float)Math::Cos(maxSunZenithAngle);
    }

    static inline Vector4 GetSizeAndInvSize(uint32 width, uint32 height)
    {
        float fWidth = float(width);
        float fHeight = float(height);
        return Vector4(fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight);
    };

    static void UpdateSkyAtmosphereConstants(const SkyAtmosphere& skyAtmosphere, const SkyAtmosphereComponent& component)
    {
        const SkyAtmosphereConfig& config = skyAtmosphere.config;
        const float rayleighScatteringScale = 1.0f;
        RenderBackend* renderBackend = skyAtmosphere.renderBackend;

        SkyAtmosphereConstants constants;
        memset(&constants, 0xBA, sizeof(SkyAtmosphereConstants));
        constants.bottomRadius = component.groundRadius;
        constants.topRadius = component.groundRadius + component.atmosphereHeight;
        constants.groundAlbedo = component.groundAlbedo;
        constants.rayleighScattering = rayleighScatteringScale * component.rayleighScattering;
        constants.mieScattering = component.mieScattering;
        constants.mieExtinction = component.mieExtinction;
        constants.mieAbsorption = component.mieExtinction - component.mieScattering;
        constants.mieAbsorption.x = Math::Max(constants.mieAbsorption.x, 0.0f);
        constants.mieAbsorption.y = Math::Max(constants.mieAbsorption.y, 0.0f);
        constants.mieAbsorption.z = Math::Max(constants.mieAbsorption.z, 0.0f);
        constants.miePhaseG = component.mieAnisotropy;
        constants.absorptionExtinction = component.absorptionExtinction;
        constants.rayleighDensity[7] = -1.0f / component.rayleighScaleHeight;
        constants.mieDensity[7] = -1.0f / component.mieScaleHeight;
        memcpy(constants.absorptionDensity, &component.absorptionDensity, sizeof(component.absorptionDensity));
        constants.cosMaxSunZenithAngle = component.cosMaxSunZenithAngle;
        constants.multipleScatteringFactor = component.multipleScatteringFactor;
        constants.transmittanceLutSizeAndInvSize = GetSizeAndInvSize(config.transmittanceLutWidth, config.transmittanceLutHeight);
        constants.multipleScatteringLutSizeAndInvSize = GetSizeAndInvSize(config.multipleScatteringLutWidth, config.multipleScatteringLutHeight);
        constants.skyViewLutSizeAndInvSize = GetSizeAndInvSize(config.skyViewLutWidth, config.skyViewLutHeight);
        constants.aerialPerspectiveVolumeSize = (float)config.aerialPerspectiveVolumeSize;
        constants.rayMarchMinSPP = (float)config.rayMarchMinSPP;
        constants.rayMarchMaxSPP = (float)config.rayMarchMaxSPP;

        renderBackend->UpdateBuffer(skyAtmosphere.skyAtmosphereConstantsUploadBuffer, 0, &constants, sizeof(SkyAtmosphereConstants));

        SkyAtmosphereShaderParameters parameters;
        parameters.transmittanceLutSizeAndInvSize = GetSizeAndInvSize(config.transmittanceLutWidth, config.transmittanceLutHeight);
        parameters.multipleScatteringLutSizeAndInvSize = GetSizeAndInvSize(config.multipleScatteringLutWidth, config.multipleScatteringLutHeight);
        parameters.skyViewLutSizeAndInvSize = GetSizeAndInvSize(config.skyViewLutWidth, config.skyViewLutHeight);
        parameters.transmittanceLutSampleCount = (float)config.transmittanceLutSampleCount;
        parameters.multipleScatteringLutSampleCount = (float)config.multipleScatteringLutSampleCount;
        parameters.rayMarchMinSPP = (float)config.rayMarchMinSPP;
        parameters.rayMarchMaxSPP = (float)config.rayMarchMaxSPP;
        parameters.bottomRadius = component.groundRadius;
        parameters.topRadius = component.groundRadius + component.atmosphereHeight;
        parameters.groundAlbedo = component.groundAlbedo;
        parameters.rayleighScattering = rayleighScatteringScale * component.rayleighScattering;
        parameters.rayleighDensityExpScale = -1.0f / component.rayleighScaleHeight;
        parameters.mieScattering = component.mieScattering;
        parameters.mieExtinction = component.mieExtinction;
        parameters.mieAbsorption = component.mieExtinction - component.mieScattering;
        parameters.mieAbsorption.x = std::max(constants.mieAbsorption.x, 0.0f);
        parameters.mieAbsorption.y = std::max(constants.mieAbsorption.y, 0.0f);
        parameters.mieAbsorption.z = std::max(constants.mieAbsorption.z, 0.0f);
        parameters.miePhaseG = component.mieAnisotropy;
        parameters.mieDensityExpScale = -1.0f / component.mieScaleHeight;
        parameters.absorptionDensity0LayerWidth = component.absorptionDensity[0].width;
        parameters.absorptionDensity0ConstantTerm = component.absorptionDensity[0].constantTerm;
        parameters.absorptionDensity0LinearTerm = component.absorptionDensity[0].linearTerm;
        parameters.absorptionDensity1ConstantTerm = component.absorptionDensity[1].constantTerm;
        parameters.absorptionDensity1LinearTerm = component.absorptionDensity[1].linearTerm;
        parameters.absorptionExtinction = component.absorptionExtinction;
        renderBackend->UpdateBuffer(skyAtmosphere.skyAtmosphereShaderParametersUploadBuffer, 0, &parameters, sizeof(SkyAtmosphereShaderParameters));
    }

    static void Update(RenderGraph& renderGraph, SkyAtmosphere& skyAtmosphere, const SkyAtmosphereComponent& component)
    {
        UpdateSkyAtmosphereConstants(skyAtmosphere, component);

        auto& skyAtmosphereData = renderGraph.blackboard.CreateSingleton<RenderGraphSkyAtmosphereData>();

        const RenderBackendTextureDesc transmittanceLutDesc = RenderBackendTextureDesc::Create2D(
            skyAtmosphere.config.transmittanceLutWidth,
            skyAtmosphere.config.transmittanceLutHeight,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        const RenderBackendTextureDesc multipleScatteringLutDesc = RenderBackendTextureDesc::Create2D(
            skyAtmosphere.config.multipleScatteringLutWidth,
            skyAtmosphere.config.multipleScatteringLutHeight,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        const RenderBackendTextureDesc skyViewLutDesc = RenderBackendTextureDesc::Create2D(
            skyAtmosphere.config.skyViewLutWidth,
            skyAtmosphere.config.skyViewLutHeight,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        const RenderBackendTextureDesc aerialPerspectiveVolumeDesc = RenderBackendTextureDesc::Create3D(
            skyAtmosphere.config.aerialPerspectiveVolumeSize,
            skyAtmosphere.config.aerialPerspectiveVolumeSize,
            skyAtmosphere.config.aerialPerspectiveVolumeSize,
            SKY_ATMOSPHERE_DEFAULT_LUT_FORMAT,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);

        skyAtmosphereData.transmittanceLut        = renderGraph.CreateTexture(transmittanceLutDesc, "SkyAtmosphereTransmittanceLut");
        skyAtmosphereData.multipleScatteringLut   = renderGraph.CreateTexture(multipleScatteringLutDesc, "SkyAtmosphereMultipleScatteringLut");
        skyAtmosphereData.skyViewLut              = renderGraph.CreateTexture(skyViewLutDesc, "SkyAtmosphereSkyViewLut");
        skyAtmosphereData.aerialPerspectiveVolume = renderGraph.CreateTexture(aerialPerspectiveVolumeDesc, "SkyAtmosphereAerialPerspectiveVolume");

        skyAtmosphere.transmittanceLutCS = skyAtmosphere.shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereTransmittanceLut);
        skyAtmosphere.multipleScatteringLutCS = skyAtmosphere.shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereMultipleScatteringLut);
        skyAtmosphere.skyViewLutCS = skyAtmosphere.shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereSkyViewLut);
        skyAtmosphere.aerialPerspectiveVolumeCS = skyAtmosphere.shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereAerialPerspectiveVolume);
        skyAtmosphere.rayMarchingCS = skyAtmosphere.shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereRayMarching);
    }

    void RenderSkyAtmosphereLUTs(RenderGraph& renderGraph, SkyAtmosphere& skyAtmosphere, const SkyAtmosphereComponent& component, uint32 renderResolutionX, uint32 renderResolutionY, RenderBackendBufferHandle sceneViewShaderParametersBuffer)
    {
        Update(renderGraph, skyAtmosphere, component);

        // Temp
        renderGraph.AddPass("UploadSkyAtmosphereShaderParameters", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.CopyBuffer(
                        skyAtmosphere.skyAtmosphereConstantsUploadBuffer,
                        0,
                        skyAtmosphere.skyAtmosphereConstants,
                        0,
                        sizeof(SkyAtmosphereConstants));

                    commandList.CopyBuffer(
                        skyAtmosphere.skyAtmosphereShaderParametersUploadBuffer,
                        0,
                        skyAtmosphere.skyAtmosphereShaderParametersBuffer,
                        0,
                        sizeof(SkyAtmosphereShaderParameters));
                };
            });

        auto& skyAtmosphereData = renderGraph.blackboard.Get<RenderGraphSkyAtmosphereData>();

        uint32 transmittanceLutWidth = skyAtmosphere.config.transmittanceLutWidth;
        uint32 transmittanceLutHeight = skyAtmosphere.config.transmittanceLutHeight;
        uint32 multipleScatteringLutWidth = skyAtmosphere.config.multipleScatteringLutWidth;
        uint32 multipleScatteringLutHeight = skyAtmosphere.config.multipleScatteringLutHeight;
        uint32 skyViewLutWidth = skyAtmosphere.config.skyViewLutWidth;
        uint32 skyViewLutHeight = skyAtmosphere.config.skyViewLutHeight;
        uint32 aerialPerspectiveVolumeSize = skyAtmosphere.config.aerialPerspectiveVolumeSize;

        renderGraph.AddPass(std::format("SkyAtmosphereTransmittanceLUT (Compute, {}x{})", transmittanceLutWidth, transmittanceLutHeight), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                auto transmittanceLut = skyAtmosphereData.transmittanceLut = builder.WriteTexture(skyAtmosphereData.transmittanceLut, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 dispatchX = Math::CeilDiv(transmittanceLutWidth, 8);
                        uint32 dispatchY = Math::CeilDiv(transmittanceLutHeight, 8);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                        shaderArguments.BindBuffer(1, skyAtmosphere.skyAtmosphereShaderParametersBuffer, 0);
                        shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(transmittanceLut)));

                        RenderBackendShaderHandle computeShader = skyAtmosphere.transmittanceLutCS;

                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            dispatchX,
                            dispatchY);
                    };
            });

        renderGraph.AddPass(std::format("SkyAtmosphereMultipleScatteringLUT (Compute, {}x{})", multipleScatteringLutWidth, multipleScatteringLutHeight), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                auto transmittanceLut = builder.ReadTexture(skyAtmosphereData.transmittanceLut, RenderBackendResourceState::ShaderResource);
                auto multipleScatteringLut = skyAtmosphereData.multipleScatteringLut = builder.WriteTexture(skyAtmosphereData.multipleScatteringLut, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = multipleScatteringLutWidth;
                    uint32 dispatchY = multipleScatteringLutHeight;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindBuffer(1, skyAtmosphere.skyAtmosphereShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(transmittanceLut)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(multipleScatteringLut)));

                    RenderBackendShaderHandle computeShader = skyAtmosphere.multipleScatteringLutCS;

                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });

        renderGraph.AddPass(std::format("SkyAtmosphereSkyViewLUT (Compute, {}x{})", skyViewLutWidth, skyViewLutHeight), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                auto transmittanceLut = builder.ReadTexture(skyAtmosphereData.transmittanceLut, RenderBackendResourceState::ShaderResource);
                auto multipleScatteringLut = builder.ReadTexture(skyAtmosphereData.multipleScatteringLut, RenderBackendResourceState::ShaderResource);

                auto skyViewLut = skyAtmosphereData.skyViewLut = builder.WriteTexture(skyAtmosphereData.skyViewLut, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = Math::CeilDiv(skyViewLutWidth, 8);
                    uint32 dispatchY = Math::CeilDiv(skyViewLutHeight, 8);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindBuffer(1, skyAtmosphere.skyAtmosphereShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(transmittanceLut)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(multipleScatteringLut)));
                    shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(skyViewLut)));

                    RenderBackendShaderHandle computeShader = skyAtmosphere.skyViewLutCS;

                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });

        renderGraph.AddPass(std::format("SkyAtmosphereAerialPerspectiveVolume (Compute, {}x{}x{})", aerialPerspectiveVolumeSize, aerialPerspectiveVolumeSize, aerialPerspectiveVolumeSize), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                auto transmittanceLut = builder.ReadTexture(skyAtmosphereData.transmittanceLut, RenderBackendResourceState::ShaderResource);
                auto multipleScatteringLut = builder.ReadTexture(skyAtmosphereData.multipleScatteringLut, RenderBackendResourceState::ShaderResource);
                auto aerialPerspectiveVolume = skyAtmosphereData.aerialPerspectiveVolume = builder.WriteTexture(skyAtmosphereData.aerialPerspectiveVolume, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = aerialPerspectiveVolumeSize / 4;
                    uint32 dispatchY = aerialPerspectiveVolumeSize / 4;
                    uint32 dispatchZ = aerialPerspectiveVolumeSize / 4;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindBuffer(1, skyAtmosphere.skyAtmosphereShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(transmittanceLut)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(multipleScatteringLut)));
                    shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(aerialPerspectiveVolume)));

                    commandList.Dispatch(
                        skyAtmosphere.aerialPerspectiveVolumeCS,
                        shaderArguments,
                        dispatchX,
                        dispatchY,
                        dispatchZ);
                };
            });
    }

    void RenderSkyAtmosphere(RenderGraph& renderGraph, SkyAtmosphere& skyAtmosphere, const SkyAtmosphereComponent& component, uint32 renderResolutionX, uint32 renderResolutionY, RenderBackendBufferHandle sceneViewShaderParametersBuffer)
    {
        renderGraph.AddPass(std::format("SkyAtmosphereRayMarching (Graphics, {}x{})", renderResolutionX, renderResolutionY), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                auto& skyAtmosphereData = renderGraph.blackboard.Get<RenderGraphSkyAtmosphereData>();
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                auto skyViewLut = builder.ReadTexture(skyAtmosphereData.skyViewLut, RenderBackendResourceState::ShaderResource);
                auto transmittanceLut = builder.ReadTexture(skyAtmosphereData.transmittanceLut, RenderBackendResourceState::ShaderResource);
                auto aerialPerspectiveVolume = builder.ReadTexture(skyAtmosphereData.aerialPerspectiveVolume, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                auto sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);
                builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);

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
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::Zero;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorWriteMask = RenderBackendColorComponentFlags::RGB;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindBuffer(1, skyAtmosphere.skyAtmosphereShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(transmittanceLut)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(skyViewLut)));
                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(aerialPerspectiveVolume)));
                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepthTexture)));

                    RenderBackendShaderHandle graphicsShader = skyAtmosphere.rayMarchingCS;
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }

    SkyAtmosphere* CreateSkyAtmosphere(RenderBackend* renderBackend, ShaderLibrary_Deprecated* shaderLibrary, SkyAtmosphereConfig* config)
    {
        SkyAtmosphere* skyAtmosphere = new SkyAtmosphere();
        skyAtmosphere->renderBackend = renderBackend;
        skyAtmosphere->config = *config;

        uint32 deviceMask = ~0u;

        RenderBackendBufferDesc skyAtmosphereConstantBufferDesc = RenderBackendBufferDesc::CreateByteAddress(sizeof(SkyAtmosphereConstants));
        skyAtmosphere->skyAtmosphereConstants = renderBackend->CreateBuffer(deviceMask, &skyAtmosphereConstantBufferDesc, nullptr, "SkyAtmosphereConstants");

        RenderBackendBufferDesc skyAtmosphereConstantUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(SkyAtmosphereConstants));
        skyAtmosphere->skyAtmosphereConstantsUploadBuffer = renderBackend->CreateBuffer(deviceMask, &skyAtmosphereConstantUploadBufferDesc, nullptr, "SkyAtmosphereConstantsUploadBuffer");

        RenderBackendBufferDesc skyAtmosphereShaderParametersBufferDesc = RenderBackendBufferDesc::CreateByteAddress(sizeof(SkyAtmosphereShaderParameters));
        skyAtmosphere->skyAtmosphereShaderParametersBuffer = renderBackend->CreateBuffer(deviceMask, &skyAtmosphereShaderParametersBufferDesc, nullptr, "SkyAtmosphereShaderParametersBuffer");

        RenderBackendBufferDesc skyAtmosphereShaderParametersUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(SkyAtmosphereShaderParameters));
        skyAtmosphere->skyAtmosphereShaderParametersUploadBuffer = renderBackend->CreateBuffer(deviceMask, &skyAtmosphereShaderParametersUploadBufferDesc, nullptr, "SkyAtmosphereShaderParametersUploadBuffer");

        bool result = true;
        ShaderDesc shaderDesc;
        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SkyAtmosphereTransmittanceLut.hsf", "SkyAtmosphereTransmittanceLutCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereTransmittanceLut, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SkyAtmosphereMultipleScatteringLut.hsf", "SkyAtmosphereMultipleScatteringLutCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereMultipleScatteringLut, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SkyAtmosphereSkyViewLut.hsf", "SkyAtmosphereSkyViewLutCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereSkyViewLut, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SkyAtmosphereAerialPerspectiveVolume.hsf", "SkyAtmosphereAerialPerspectiveVolumeCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereAerialPerspectiveVolume, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/SkyAtmosphereRayMarching.hsf", "SkyAtmosphereRayMarchingVS", "SkyAtmosphereRayMarchingPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereRayMarching, shaderDesc);

        skyAtmosphere->shaderLibrary = shaderLibrary;
        //skyAtmosphere->transmittanceLutShader        = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereTransmittanceLut);
        //skyAtmosphere->multipleScatteringLutShader   = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereMultipleScatteringLut);
        //skyAtmosphere->skyViewLutShader              = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereSkyViewLut);
        //skyAtmosphere->aerialPerspectiveVolumeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereAerialPerspectiveVolume);
        //skyAtmosphere->renderSkyShader               = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyAtmosphereRenderSky);

        return skyAtmosphere;
    }

    void DestroySkyAtmosphere(SkyAtmosphere* skyAtmosphere)
    {
        RenderBackend* renderBackend = skyAtmosphere->renderBackend;
        renderBackend->DestroyBuffer(skyAtmosphere->skyAtmosphereConstants);
        renderBackend->DestroyShader(skyAtmosphere->transmittanceLutCS);
        renderBackend->DestroyShader(skyAtmosphere->multipleScatteringLutCS);
        renderBackend->DestroyShader(skyAtmosphere->skyViewLutCS);
        renderBackend->DestroyShader(skyAtmosphere->aerialPerspectiveVolumeCS);
        renderBackend->DestroyShader(skyAtmosphere->rayMarchingCS);
        delete skyAtmosphere;
    }
}