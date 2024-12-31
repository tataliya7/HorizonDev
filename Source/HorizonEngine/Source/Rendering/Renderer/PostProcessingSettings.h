#pragma once

#include "RendererCommon.h"

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
        Hable,
        Custom,
    };

    /**
     * A collection of configurable parameters for all post-processing effects.
     */
    struct PostProcessingSettings
    {
        /** The strength of the motion blur effect. */
        float motionBlurIntensity = 1.0f;

        /** The maximum length of velocity in screen percentage. */
        float motionBlurMaxVelocityLength = 100.0f;

        /** Exposure computation method. */
        ExposureMethod exposureMethod = ExposureMethod::AutoExposure;

        /** User-defined fixed exposure value (EV100) of the scene, only takes effect when the exposure method is 'FixedExposure'. */
        float fixedExposureValue = 0.0f;

        /** Exposure computation method. */
        float autoExposureExposureCompensation = 0.0f;

        /** Auto-exposure minimum exposure value (EV100). Used to limit the exposure value calculated by the auto-exposure algorithm. */
        float autoExposureMinExposureValue = -10.0f;

        /** Auto-exposure maximum exposure value (EV100). Used to limit the exposure value calculated by the auto-exposure algorithm. */
        float autoExposureMaxExposureValue = 20.0f;

        /** The adaptation speed from a dark to a light environment, i.e., exposure value decrease speed. */
        float autoExposureSpeedDarkToBright = 3.0f;

        /** The adaptation speed from a light to a dark environment, i.e., exposure value rising speed. */
        float autoExposureSpeedBrightToDark = 1.0f;

        /** The lower percentage of the histogram. Values below this threshold will be discarded and won't contribute to the scene average luminance. */
        float autoExposureHistogramLowerPercentage = 0.0f;

        /** The upper percentage of the histogram. Values beyond this threshold will be discarded and won't contribute to the scene average luminance. */
        float autoExposureHistogramHigherPercentage = 100.0f;

        /** Histogram minimum exposure value. This property is expressed in EV100. */
        float autoExposureHistogramMinEV100 = -10.0f;

        /** Histogram maximum exposure value. This property is expressed in EV100. */
        float autoExposureHistogramMaxEV100 = 20.0f;

        /** Overall intensity of the bloom effect. */
        float bloomIntensity = 1.0f;

        /** The radius of the bloom effect, controls how far the bloom extends out from the highlight. */
        float bloomRadius = 0.5f;

        float lensFlareIntensity = 1.0f;

        float lensFlareHaloIntensity = 1.0f;

        float lensFlareHaloWidth = 0.45f;

        float lensFlareHaloChromaticAberrationOffset = 0.01f;

        /** The strength of the chromatic aberration effect. */
        float chromaticAberrationIntensity = 0.0f;

        /** TBD. */
        float chromaticAberrationOffset;

        /** The color temperature at which white objects on film actually look white. This property is expressed in Kelvin (K). */
        float whiteBalance = 6500.0f;

        // @todo Investigate what the appropriate default value should be.
        float vignetteIntensity = 0.25f;

        ToneMappingOperatorType toneMappingOperator;
    };
}