#pragma once

#ifdef __cplusplus
#include "Foundation/FoundationModule.h"
namespace Horizon {
#define uint Horizon::uint32
#define float2 Horizon::Vector2
#define float3 Horizon::Vector3
#define float4 Horizon::Vector4
#define float3x3 Horizon::Matrix3x3
#define float4x4 Horizon::Matrix4x4
#endif // __cplusplus

struct PerFrameShaderParameters
{
    uint frameIndex;
    uint frameIndexMod8;
    uint renderWidth;
    uint renderHeight;

    uint targetWidth;
    uint targetHeight;
    uint displayWidth;
    uint displayHeight;

    float4 renderResolution;
    float4 targetResolution;
    float4 displayResolution;

    float3 cameraPosition;
    float deltaTimeInSeconds;

    float3 previousCameraPosition;
    float padding0;

    float2 cameraJitterOffset;
    float2 previousCameraJitterOffset;

    float3 cameraUpVector;
    float padding1;

    float3 cameraRightVector;
    float padding2;

    float3 cameraForwardVector;
    float padding3;

    float halfFovInRadians;
    float aspectRatio;
    float nearClippingPlane;
    float farClippingPlane;
    //float4 frustumPlanes[6];

    float materialTextureMipLodBias;
    float preExposure;
    float inversePreExposure;
    float preExposureCorrection;

    float2 motionVectorScale;
    float2 viewSpaceDepthToNDCSpaceDepthTransform;

    float4x4 worldToViewMatrix;
    float4x4 viewToWorldMatrix;
    float4x4 viewToClipMatrix;
    float4x4 clipToViewMatrix;
    float4x4 worldToClipMatrix;
    float4x4 clipToWorldMatrix;
    float4x4 nonJitteredWorldToClipMatrix;
    float4x4 nonJitteredClipToWorldMatrix;

    float4x4 previousWorldToViewMatrix;
    float4x4 previousViewToWorldMatrix;
    float4x4 previousViewToClipMatrix;
    float4x4 previousClipToViewMatrix;
    float4x4 previousWorldToClipMatrix;
    float4x4 previousClipToWorldMatrix;
    float4x4 previousNonJitteredWorldToClipMatrix;

    float4x4 currentClipToPreviousClipMatrix;
    float4x4 previousClipToCurrentClipMatrix;

    float3 indirectLightingMultiplier;
    float padding6;

    float3 atmosphericLightDirection;
    float padding7;

    float3 atmosphericLightOuterSpaceIlluminance;
    float padding8;

    float3 atmosphericLightDiskLuminance;
    float atmosphericLightDiskCosHalfApexAngle;

    float4 skyAtmosphereTransmittanceLutSize;
    float4 skyAtmosphereMultipleScatteringLutSize;
    float4 skyAtmosphereSkyViewLutSize;

    float2 skyAtmosphereAerialPerspectiveVolumeSize;
    float2 padding9;

    float skyAtmosphereTransmittanceLutSampleCount;
    float skyAtmosphereMultipleScatteringLutSampleCount;
    float skyAtmosphereRayMarchingMinSampleCount;
    float skyAtmosphereRayMarchingMaxSampleCount;

    float skyAtmosphereBottomRadiusInKilometers;
    float skyAtmosphereTopRadiusInKilometers;
    float padding19;
    float padding20;

    float3 skyAtmosphereGroundAlbedo;
    float padding21;

    float3 skyAtmosphereRayleighScattering;
    float padding22;

    float skyAtmosphereRayleighDensityExpScale;
    float padding23;
    float padding24;
    float padding25;

    float3 skyAtmosphereMieScattering;
    float padding26;

    float3 skyAtmosphereMieAbsorption;
    float padding27;

    float3 skyAtmosphereMieExtinction;
    float skyAtmosphereMiePhaseG;

    float skyAtmosphereMieDensityExpScale;
    float skyAtmosphereAbsorptionDensity0LayerWidth;
    float skyAtmosphereAbsorptionDensity0ConstantTerm;
    float skyAtmosphereAbsorptionDensity0LinearTerm;

    float skyAtmosphereAbsorptionDensity1ConstantTerm;
    float skyAtmosphereAbsorptionDensity1LinearTerm;
    float padding17;
    float padding18;

    float3 skyAtmosphereAbsorptionExtinction;
    float padding16;

    float3 skyAtmosphereSkyLuminanceFactor;
    float padding15;

    float4x4 skyAtmosphereSkyViewLutReferential;

    // TODO: move post processing settings to a separate struct
    uint motionBlurMaxSampleCount;
    float motionBlurIntensity;
    float motionBlurMaxVelocityLengthInPixels;
    float padding10;

    float autoExposureUseTargetExposure;
    float autoExposureExposureCompensation;
    float autoExposureMinExposureValue;
    float autoExposureMaxExposureValue;

    float autoExposureSpeedDarkToBright;
    float autoExposureSpeedBrightToDark;
    float autoExposureHistogramLowerPercentage;
    float autoExposureHistogramHigherPercentage;

    float autoExposureHistogramMinEV100;
    float autoExposureHistogramMaxEV100;
    float padding11;
    float padding12;

    float bloomIntensity;
    float bloomRadius;

    float lensFlareIntensity;
    float lensFlareHaloIntensity;
    float lensFlareHaloWidth;
    float lensFlareHaloChromaticAberrationOffset;

    float chromaticAberrationIntensity;
    float chromaticAberrationOffset;

    float whiteBalance;
    float vignetteIntensity;
    float padding13;
    float padding14;

    //float3 lensDirtScaleFactor;
    //float4 colorCorrectionSaturation;
    //float4 colorCorrectionContrast;
    //float4 colorCorrectionGamma;
    //float4 colorCorrectionGain;
    //float4 colorCorrectionOffset;
};

#ifndef __cplusplus
//DECLARE_ALIASED_ARRAY_CONSTANT_BUFFER_CBV(PerFrameShaderParameters);
DECLARE_ALIASED_ARRAY_STRUCTURED_BUFFER_SRV(PerFrameShaderParameters);
#endif

#ifdef __cplusplus
static_assert(sizeof(PerFrameShaderParameters) % 16 == 0);
#endif

#ifdef __cplusplus
#undef uint
#undef float2
#undef float3
#undef float4
#undef float3x3
#undef float4x4
}
#endif // __cplusplus