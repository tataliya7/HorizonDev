#pragma once

#include "Core/CoreDefinitions.h"
#include "Core/CoreTypes.h"
#include "Core/StdHeaders.h"
#include "Core/Math/Math.h"
#include "Core/JobSystem/JobSystem.h"
#include "Core/Logging/Logging.h"
#include "Core/Misc/Misc.h"
#include "Core/Platform/Platform.h"
#include "Core/Memory/Memory.h"
#include "Core/ColorManagement.h"

namespace HE
{
    struct PostProcessingSettings
    {
        // Auto Exposure
        float fixedExposureValue;
        float autoExposureExposureCompensation;
        float autoExposureMinExposureValue;
        float autoExposureMaxExposureValue;
        float autoExposureHistogramLowPercent;
        float autoExposureHistogramHighPercent;
        float autoExposureHistogramMinEV100;
        float autoExposureHistogramMaxEV100;
        // Exposure value decrease speed, TODO: use physical units, like f-stops per second
        float autoExposureSpeedDarkToBright;
        // Exposure value rising speed, TODO: use physical units, like f-stops per second
        float autoExposureSpeedBrightToDark;
        int autoExposureUseTargetExposure;

        // Depth of Filed
        float dofScale;
        float dofFocalDistance;
        float dofFocalRegion;
        float dofNearTransitionRegion;
        float dofFarTransitionRegion;
        float dofNearRegionBlurSize;
        float dofFarRegionBlurSize;

        float localExposureShadows;
        float localExposureHighlights;
        int32 localExposureCoarsestMipLevel;
        int32 localExposureDisplayMipLevel;
        float localExposurePreferenceSigma;

        // Bloom
        float bloomIntensity;
        float bloomRadius;

        float lensDirtIntensity;
        Vector4 lensDirtTint;

        float lensFlaresIntensity;

        // Chrmatic Aberration
        float chromaticAberrationIntensity;
        float chromaticAberrationOffset;

        // Motion Blur

        // Tone Mapping

        // Color Correction
        Vector4 colorCorrectionSaturation;
        Vector4 colorCorrectionContrast;
        Vector4 colorCorrectionGamma;
        Vector4 colorCorrectionGain;
        Vector4 colorCorrectionOffset;

        // Color Grading
        float colorGradingWhiteBalanceColorTemperature;

        bool localExposureEnabled;
    };
}