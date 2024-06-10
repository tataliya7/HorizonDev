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
    uint renderWidth;
    uint renderHeight;
    uint targetWidth;
    uint targetHeight;
    uint displayWidth;
    uint displayHeight;
    uint padding1;

    float4 renderResolution;
    float4 targetResolution;
    float4 displayResolution;

    float deltaTimeInSeconds;
    float3 cameraPosition;
    float3 previousCameraPosition;
    float2 cameraJitterOffset;
    float2 previousCameraJitterOffset;
    float3 cameraUpVector;
    float3 cameraRightVector;
    float3 cameraForwardVector;
    float halfFovInRadians;
    float aspectRatio;
    float nearClippingPlane;
    float farClippingPlane;
    //float4 frustumPlanes[6];

    float materialTextureMipLodBias;
    float preExposure;
    float oneOverPreExposure;
    float preExposureCorrection;

    // TODO: move post processing settings to a separate struct
    float autoExposureExposureCompensation;
    float autoExposureMinExposureValue;
    float autoExposureMaxExposureValue;
    float autoExposureSpeedDarkToBright;
    float autoExposureSpeedBrightToDark;
    float autoExposureHistogramLowerPercentage;
    float autoExposureHistogramHigherPercentage;
    float autoExposureHistogramMinEV100;
    float autoExposureHistogramMaxEV100;
    int autoExposureUseTargetExposure;
    float bloomIntensity;
    float bloomRadius;
    float chromaticAberrationIntensity;
    float chromaticAberrationOffset;
    float whiteBalance;
    //float3 lensDirtScaleFactor;
    //float4 colorCorrectionSaturation;
    //float4 colorCorrectionContrast;
    //float4 colorCorrectionGamma;
    //float4 colorCorrectionGain;
    //float4 colorCorrectionOffset;

    float3 indirectLightingMultiplier;

    float3 atmosphericLightDirection;
    float3 atmosphericLightOuterSpaceIlluminance;
    float3 atmosphericLightDiskLuminance;
    float atmosphericLightDiskCosHalfApexAngle;

    float4 skyAtmosphereTransmittanceLutSize;
    float4 skyAtmosphereMultipleScatteringLutSize;
    float4 skyAtmosphereSkyViewLutSize;

    float2 skyAtmosphereAerialPerspectiveVolumeSize;
    float2 padding2;

    float skyAtmosphereTransmittanceLutSampleCount;
    float skyAtmosphereMultipleScatteringLutSampleCount;
    float skyAtmosphereRayMarchingMinSampleCount;
    float skyAtmosphereRayMarchingMaxSampleCount;
    float skyAtmosphereBottomRadiusInKilometers;
    float skyAtmosphereTopRadiusInKilometers;
    float3 skyAtmosphereGroundAlbedo;
    float3 skyAtmosphereRayleighScattering;
    float skyAtmosphereRayleighDensityExpScale;
    float3 skyAtmosphereMieScattering;
    float3 skyAtmosphereMieAbsorption;
    float3 skyAtmosphereMieExtinction;
    float skyAtmosphereMiePhaseG;
    float skyAtmosphereMieDensityExpScale;
    float skyAtmosphereAbsorptionDensity0LayerWidth;
    float skyAtmosphereAbsorptionDensity0ConstantTerm;
    float skyAtmosphereAbsorptionDensity0LinearTerm;
    float skyAtmosphereAbsorptionDensity1ConstantTerm;
    float skyAtmosphereAbsorptionDensity1LinearTerm;
    float3 skyAtmosphereAbsorptionExtinction;
    float3 skyAtmosphereSkyLuminanceFactor;
    float3x3 skyAtmosphereSkyViewLutReferential;

    float4x4 worldToViewMatrix;
    float4x4 viewToWorldMatrix;
    float4x4 viewToClipMatrix;
    float4x4 clipToViewMatrix;
    float4x4 worldToClipMatrix;
    float4x4 clipToWorldMatrix;
    float4x4 nonJitteredWorldToClipMatrix;

    float4x4 previousWorldToViewMatrix;
    float4x4 previousViewToWorldMatrix;
    float4x4 previousViewToClipMatrix;
    float4x4 previousClipToViewMatrix;
    float4x4 previousWorldToClipMatrix;
    float4x4 previousClipToWorldMatrix;
    float4x4 previousNonJitteredWorldToClipMatrix;
};

#ifndef __cplusplus
DECLARE_CONSTANT_BUFFER_TYPE(PerFrameShaderParameters);
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