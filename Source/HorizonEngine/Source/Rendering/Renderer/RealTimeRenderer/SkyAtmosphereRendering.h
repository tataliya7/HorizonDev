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
        Vector4 transmittanceLutSize;
        Vector4 multipleScatteringLutSize;
        Vector4 skyViewLutSize;
        Vector2 aerialPerspectiveVolumeSize;

        float transmittanceLutSampleCount;
        float multipleScatteringLutSampleCount;
        float rayMarchingMinSampleCount;
        float rayMarchingMaxSampleCount;

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

    void SetupSkyAtmosphereShaderParameters(SkyAtmosphereShaderParameters& outParameters, const SkyAtmosphereRenderObject& renderObject);

    struct SkyAtmosphereViewRelatedParameters
    {
        Matrix3x3 skyViewLutReferential;
    };

    void SetupSkyAtmosphereViewRelatedParameters(SkyAtmosphereViewRelatedParameters& outParameters, const SkyAtmosphereRenderObject& renderObject, const Vector3& worldSpaceCameraPosition, const Vector3& cameraForwardVector);
}