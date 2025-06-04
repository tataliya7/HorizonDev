#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    struct RealTimeRendererSkyAtmosphereLUTs
    {
        RenderGraphTextureHandle transmittanceLut;
        RenderGraphTextureHandle multipleScatteringLut;
        RenderGraphTextureHandle skyViewLut;
        RenderGraphTextureHandle aerialPerspectiveVolume;
    };

    struct SkyAtmosphereShaderParameters
    {
        Vector4f transmittanceLutSize;
        Vector4f multipleScatteringLutSize;
        Vector4f skyViewLutSize;
        Vector2f aerialPerspectiveVolumeSize;

        float transmittanceLutSampleCount;
        float multipleScatteringLutSampleCount;
        float rayMarchingMinSampleCount;
        float rayMarchingMaxSampleCount;

        float bottomRadius;
        float topRadius;
        Vector3f groundAlbedo;

        Vector3f rayleighScattering;
        float rayleighDensityExpScale;

        Vector3f mieScattering;
        Vector3f mieExtinction;
        Vector3f mieAbsorption;
        float miePhaseG;
        float mieDensityExpScale;

        float absorptionDensity0LayerWidth;
        float absorptionDensity0ConstantTerm;
        float absorptionDensity0LinearTerm;
        float absorptionDensity1ConstantTerm;
        float absorptionDensity1LinearTerm;
        Vector3f absorptionExtinction;
    };

    void SetupSkyAtmosphereShaderParameters(SkyAtmosphereShaderParameters& outParameters, const SkyAtmosphereRenderObject& renderObject);

    struct SkyAtmosphereViewRelatedParameters
    {
        Matrix4x4f skyViewLutReferential;
    };

    void SetupSkyAtmosphereViewRelatedParameters(SkyAtmosphereViewRelatedParameters& outParameters, const SkyAtmosphereRenderObject& renderObject, const Vector3f& worldSpaceCameraPosition, const Vector3f& cameraForwardVector);
}