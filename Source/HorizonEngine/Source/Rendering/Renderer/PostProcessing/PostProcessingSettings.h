#pragma once

#include "Rendering/Renderer/RendererCommon.h"

namespace Horizon
{
    enum class ExposureMethod
    {
        FixedExposure,
        AutoExposure,
    };

    enum class ToneMappingOperatorType
    {
        Linear,
        ACES,
    };

    enum class LocalToneMappingMethod
    {
        None,
        BilateralGrid,
        ExposureFusion,
    };

    struct PostProcessingSettings
    {
        float motionBlurIntensity = 1.0f;

        float motionBlurMaxVelocityLength = 100.0f;

        ExposureMethod exposureMethod = ExposureMethod::AutoExposure;

        float fixedExposureValue = 0.0f;

        float autoExposureExposureCompensation = 0.0f;

        float autoExposureMinExposureValue = -10.0f;

        float autoExposureMaxExposureValue = 20.0f;

        float autoExposureSpeedDarkToBright = 3.0f;

        float autoExposureSpeedBrightToDark = 1.0f;

        float autoExposureHistogramLowerPercentage = 0.0f;

        float autoExposureHistogramHigherPercentage = 100.0f;

        float autoExposureHistogramMinEV100 = -10.0f;

        float autoExposureHistogramMaxEV100 = 20.0f;

        LocalToneMappingMethod localToneMappingMethod = LocalToneMappingMethod::None;

        float bilateralGridLocalToneMappingShadows = 1.0f;

        float bilateralGridLocalToneMappingHighlights = 1.0f;

        float bilateralGridLocalToneMappingDetailStrength = 1.0f;

        float bilateralGridLocalToneMappingGaussianFilterWeight = 0.6f;

        float exposureFusionLocalToneMappingShadows = 1.0f;

        float exposureFusionLocalToneMappingHighlights = 1.0f;

        float bloomIntensity = 1.0f;

        float bloomRadius = 0.5f;

        float lensFlareIntensity = 1.0f;

        float lensFlareHaloIntensity = 1.0f;

        float lensFlareHaloWidth = 0.45f;

        float lensFlareHaloChromaticAberrationOffset = 0.01f;

        float chromaticAberrationIntensity = 0.0f;

        float chromaticAberrationOffset;

        float whiteBalance = 6500.0f;

        float vignetteIntensity = 0.25f;

        ToneMappingOperatorType toneMappingOperator;
    };
}
